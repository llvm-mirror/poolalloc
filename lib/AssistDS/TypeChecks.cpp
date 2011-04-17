//===------------ TypeChecks.cpp - Insert runtime type checks -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass inserts checks to enforce type safety during runtime.
//
//===----------------------------------------------------------------------===//

#include "assistDS/TypeChecks.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

char TypeChecks::ID = 0;
static RegisterPass<TypeChecks> TC("typechecks", "Insert runtime type checks", false, true);

static const Type *VoidTy = 0;
static const Type *Int8Ty = 0;
static const Type *Int32Ty = 0;
static const PointerType *VoidPtrTy = 0;

// Incorporate one type and all of its subtypes into the collection of used types.
void TypeChecks::IncorporateType(const Type *Ty) {
  // If Ty doesn't already exist in the used types map, add it now. Otherwise, return.
  if (UsedTypes[Ty] != 0) {
    return;
  }

  UsedTypes[Ty] = maxType;
  ++maxType;

  // Make sure to add any types this type references now.
  for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I) {
    IncorporateType(*I);
  }
}

// Incorporate all of the types used by this value.
void TypeChecks::IncorporateValue(const Value *V) {
  IncorporateType(V->getType());
  UsedValues[V] = V->getType();

  // If this is a constant, it could be using other types.
  if (const Constant *C = dyn_cast<Constant>(V)) {
    if (!isa<GlobalValue>(C)) {
      for (User::const_op_iterator OI = C->op_begin(), OE = C->op_end(); OI != OE; ++OI) {
        IncorporateValue(*OI);
      }
    }
  }
}

bool TypeChecks::runOnModule(Module &M) {
  bool modified = false; // Flags whether we modified the module.

  TD = &getAnalysis<TargetData>();
  TypeAnalysis &TA = getAnalysis<TypeAnalysis>();

  VoidTy = IntegerType::getVoidTy(M.getContext());
  Int8Ty = IntegerType::getInt8Ty(M.getContext());
  Int32Ty = IntegerType::getInt32Ty(M.getContext());
  VoidPtrTy = PointerType::getUnqual(Int8Ty);

  UsedTypes.clear(); // Reset if run multiple times.
  maxType = 1;

  // Loop over global variables, incorporating their types.
  for (Module::const_global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
    IncorporateType(I->getType());
    if (I->hasInitializer()) {
      IncorporateValue(I->getInitializer());
    }
  }

  // Insert the shadow initialization function at the entry to main.
  Function *MainF = M.getFunction("main");
  if (MainF == 0 || MainF->isDeclaration())
    return false;

  inst_iterator MainI = inst_begin(MainF);
  modified |= initShadow(M, *MainI);

  // record all globals
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E; ++I) {
    if(!I->getNumUses() == 1)
      continue;
    if(!I->hasInitializer())
      continue;
    modified |= visitGlobal(M, *I, *MainI);
  }

  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    IncorporateType(MI->getType());
    Function &F = *MI;

    // Loop over all of the instructions in the function, adding their return type as well as the types of their operands.
    for (inst_iterator II = inst_begin(F), IE = inst_end(F); II != IE; ++II) {
      Instruction &I = *II;

      IncorporateType(I.getType()); // Incorporate the type of the instruction.
      for (User::op_iterator OI = I.op_begin(), OE = I.op_end(); OI != OE; ++OI) {
        IncorporateValue(*OI); // Insert instruction operand types.
      }

      if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
        if (TA.isCopyingStore(SI)) {
          Value *SS = TA.getStoreSource(SI);
          if (SS != NULL) {
            modified |= visitCopyingStoreInst(M, *SI, SS);
          }
        } else {
          modified |= visitStoreInst(M, *SI);
        }
      } else if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
        if (!TA.isCopyingLoad(LI)) {
          modified |= visitLoadInst(M, *LI);
        }
      }
    }
  }

  return modified;
}

// Print the types found in the module. If the optional Module parameter is
// passed in, then the types are printed symbolically if possible, using the
// symbol table from the module.
void TypeChecks::print(raw_ostream &OS, const Module *M) const {
  OS << "Types in use by this module:\n";
  for (std::map<const Type *, unsigned int>::const_iterator I = UsedTypes.begin(), E = UsedTypes.end(); I != E; ++I) {
    OS << "  ";
    WriteTypeSymbolic(OS, I->first, M);
    OS << '\n';
  }

  OS << "\nValues in use by this module:\n";
  for (std::map<const Value *, const Type *>::const_iterator I = UsedValues.begin(), E = UsedValues.end(); I != E; ++I) {
    OS << "  " << I->first << " = ";
    WriteTypeSymbolic(OS, I->second, M);
    OS << '\n';
  }

  OS << "\nNumber of types: " << maxType << '\n';
}

// Initialize the shadow memory which contains the 1:1 mapping.
bool TypeChecks::initShadow(Module &M, Instruction &I) {
  // Create the call to the runtime initialization function and place it before the store instruction.
  Constant *F = M.getOrInsertFunction("shadowInit", VoidTy, NULL);
  CallInst::Create(F, "", &I);

  return true;
}

// Initialize the shadow memory which contains the 1:1 mapping.
bool TypeChecks::unmapShadow(Module &M, Instruction &I) {
  // Create the call to the runtime shadow memory unmap function and place it before any exiting instruction.
  Constant *F = M.getOrInsertFunction("shadowUnmap", VoidTy, NULL);
  CallInst::Create(F, "", &I);

  return true;
}

bool TypeChecks::visitGlobal(Module &M, GlobalVariable &GV, Instruction &I) {
  
  CastInst *BCI = BitCastInst::CreatePointerCast(&GV, VoidPtrTy, "", &I);
  std::vector<Value *> Args;
  Args.push_back(BCI);
  const PointerType *PTy = GV.getType();
  Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[PTy->getElementType()]));
  Constant *F = M.getOrInsertFunction("trackStoreInst", VoidTy, VoidPtrTy, Int8Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &I);

  return true;
}
// Insert runtime checks before all load instructions.
bool TypeChecks::visitLoadInst(Module &M, LoadInst &LI) {
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(LI.getPointerOperand(), VoidPtrTy, "", &LI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[LI.getType()]));

  // Create the call to the runtime check and place it before the load instruction.
  Constant *F = M.getOrInsertFunction("trackLoadInst", VoidTy, VoidPtrTy, Int8Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &LI);

  return true;
}

// Insert runtime checks before all store instructions.
bool TypeChecks::visitStoreInst(Module &M, StoreInst &SI) {
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(SI.getPointerOperand(), VoidPtrTy, "", &SI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[SI.getOperand(0)->getType()])); // SI.getValueOperand()

  // Create the call to the runtime check and place it before the store instruction.
  Constant *F = M.getOrInsertFunction("trackStoreInst", VoidTy, VoidPtrTy, Int8Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &SI);

  return true;
}

// Insert runtime checks before copying store instructions.
bool TypeChecks::visitCopyingStoreInst(Module &M, StoreInst &SI, Value *SS) {
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(SI.getPointerOperand(), VoidPtrTy, "", &SI);
  CastInst *BCI_Src = BitCastInst::CreatePointerCast(SS, VoidPtrTy, "", &SI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(BCI_Src);
  Args.push_back(ConstantInt::get(Int8Ty, TD->getTypeStoreSize(SI.getOperand(0)->getType())));

  // Create the call to the runtime check and place it before the store instruction.
  Constant *F = M.getOrInsertFunction("copyTypeInfo", VoidTy, VoidPtrTy, VoidPtrTy, Int8Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &SI);

  return true;
}
