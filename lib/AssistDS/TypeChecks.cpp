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

#include "TypeChecks.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

char TypeChecks::ID = 0;
static RegisterPass<TypeChecks> TC("typechecks", "Insert runtime type checks", false, true);

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
  // Flags whether we modified the module.
  bool modified = false;

  UsedTypes.clear(); // Reset if run multiple times.
  maxType = 1;

  // Loop over global variables, incorporating their types.
  for (Module::const_global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
    IncorporateType(I->getType());
    if (I->hasInitializer()) {
      IncorporateValue(I->getInitializer());
    }
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
        modified |= visitStoreInst(M, *SI);
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

// Insert runtime checks before all store instructions.
bool TypeChecks::visitStoreInst(Module &M, StoreInst &SI) {
  const Type *Int8Ty = IntegerType::getInt8Ty(M.getContext());
  const Type *Int32Ty = IntegerType::getInt32Ty(M.getContext());
  PointerType *VoidPtrTy = PointerType::getUnqual(Int8Ty);

  CastInst *BCI = BitCastInst::CreatePointerCast(SI.getPointerOperand(), VoidPtrTy, "", &SI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(ConstantInt::get(Int32Ty, UsedTypes[SI.getOperand(0)->getType()])); // SI.getValueOperand()

  // Create the call to the runtime check, and place it before the store instruction.
  Constant *F = M.getOrInsertFunction("trackStoreInst", Int8Ty, VoidPtrTy, Int32Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &SI);

  return true;
}
