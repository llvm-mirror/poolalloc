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
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Intrinsics.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"

#include <set>
#include <vector>

using namespace llvm;

char TypeChecks::ID = 0;

static RegisterPass<TypeChecks> 
TC("typechecks", "Insert runtime type checks", false, true);

// Pass statistics
STATISTIC(numLoadChecks,  "Number of Load Insts that need type checks");
STATISTIC(numStoreChecks, "Number of Store Insts that need type checks");

namespace {
  static cl::opt<bool> EnableTypeSafeOpt("enable-type-safe-opt",
         cl::desc("Use DSA pass"),
         cl::Hidden,
         cl::init(false));
}

static int tagCounter = 0;
static const Type *VoidTy = 0;
static const Type *Int8Ty = 0;
static const Type *Int32Ty = 0;
static const Type *Int64Ty = 0;
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
  TA = &getAnalysis<TypeAnalysis>();
  if(EnableTypeSafeOpt)
    TS = &getAnalysis<dsa::TypeSafety<TDDataStructures> >();

  VoidTy = IntegerType::getVoidTy(M.getContext());
  Int8Ty = IntegerType::getInt8Ty(M.getContext());
  Int32Ty = IntegerType::getInt32Ty(M.getContext());
  Int64Ty = IntegerType::getInt64Ty(M.getContext());
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

  Function *MainF = M.getFunction("main");
  if (MainF == 0 || MainF->isDeclaration()) {
    assert(0 && "No main function found");
    return false;
  }

  // Insert the shadow initialization function.
  modified |= initShadow(M);

  inst_iterator MainI = inst_begin(MainF);
  // record all globals
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E; ++I) {
    if(!I->getNumUses() == 1)
      continue;
    if(!I->hasInitializer())
      continue;
    modified |= visitGlobal(M, *I, I->getInitializer(), *MainI, 0);
  }

  std::vector<Function *> toProcess;
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    IncorporateType(MI->getType());
    Function &F = *MI;
    toProcess.push_back(&F);

    // Loop over all of the instructions in the function, 
    // adding their return type as well as the types of their operands.
    for (inst_iterator II = inst_begin(F), IE = inst_end(F); II != IE; ++II) {
      Instruction &I = *II;

      IncorporateType(I.getType()); // Incorporate the type of the instruction.
      for (User::op_iterator OI = I.op_begin(), OE = I.op_end(); OI != OE; ++OI) {
        IncorporateValue(*OI); // Insert instruction operand types.
      }

      if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
        if (TA->isCopyingStore(SI)) {
          Value *SS = TA->getStoreSource(SI);
          if (SS != NULL) {
            modified |= visitCopyingStoreInst(M, *SI, SS);
          }
        } else {
          modified |= visitStoreInst(M, *SI);
        }
      } else if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
        if (!TA->isCopyingLoad(LI)) {
          modified |= visitLoadInst(M, *LI);
        }
      } else if (CallInst *CI = dyn_cast<CallInst>(&I)) {
        modified |= visitCallInst(M, *CI);
      } else if (InvokeInst *II = dyn_cast<InvokeInst>(&I)) {
        modified |= visitInvokeInst(M, *II);
      } else if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
        modified |= visitAllocaInst(M, *AI);
      } else if (VAArgInst *VI = dyn_cast<VAArgInst>(&I)) {
        modified |= visitVAArgInst(M, *VI);
      }
    }
  }

  // Record types for byval arguments.

  while(!toProcess.empty()) {
    Function *F = toProcess.back();
    toProcess.pop_back();
    if(F->isDeclaration())
      continue;
    modified |= visitByValFunction(M, *F);
    modified |= visitVarArgFunction(M, *F);
  }

  return modified;
}

// Transform Variable Argument functions, by also passing
// the relavant metadata info
bool
TypeChecks::visitVarArgFunction(Module &M, Function &F) {
  if(!F.isVarArg())
    return false;
  if(!F.hasInternalLinkage())
    return false;
  // FIXME:handle external functions

  // Find all uses of the function
  for(Value::use_iterator ui = F.use_begin(), ue = F.use_end();
      ui != ue;)  {
    // Check for call sites
    CallInst *CI = dyn_cast<CallInst>(ui++);
    if(!CI)
      continue;
    if(CI->getNumOperands() - 1 <= F.arg_size()) 
      continue;
    std::vector<Value *> Args;
    unsigned int i = F.arg_size() + 1;
    for(i = 1 ;i < CI->getNumOperands(); i++) {
      // As the first vararg argument pass the number of var_arg arguments
      if(i == F.arg_size() + 1) {
        Args.push_back(ConstantInt::get(Int64Ty, 2*(CI->getNumOperands()-1 - F.arg_size())));
      }
      if(i > F.arg_size()) {
        // For each vararg argument, also add its type information before it
        Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[CI->getOperand(i)->getType()]));
      }

      // Add the original argument
      Args.push_back(CI->getOperand(i));
    }

    // Create the new call
    CallInst *CI_New = CallInst::Create(CI->getCalledValue(), Args.begin(), Args.end(), "", CI);
    CI->replaceAllUsesWith(CI_New);
    CI->eraseFromParent();
  }

  // Modify the function to add a call to get the num of arguments
  VAArgInst *VASize = NULL;
  for (Function::iterator B = F.begin(), FE = F.end(); B != FE; ++B) {
    for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
      CallInst *CI = dyn_cast<CallInst>(I++);
      if(!CI)
        continue;
      Function *CalledF = dyn_cast<Function>(CI->getCalledFunction());
      if(!CalledF)
        continue;
      if(!CalledF->isIntrinsic())
        continue;
      if(CalledF->getIntrinsicID() != Intrinsic::vastart) 
        continue;
      VASize = new VAArgInst(CI->getOperand(1), Int64Ty, "NumArgs");
      VASize->insertAfter(CI);
      break;
    }
  }
  assert(VASize && "Varargs function without a call to VAStart???");

  // Modify function to add checks on every var_arg call to ensure that we
  // are not accessing more arguments than we passed in.

  // Add a counter variable
  AllocaInst *Counter = new AllocaInst(Int64Ty, "", VASize);
  new StoreInst(ConstantInt::get(Int64Ty, 0), Counter, VASize); 

  // Increment the counter

  for (Function::iterator B = F.begin(), FE = F.end(); B != FE; ++B) {
    for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
      VAArgInst *VI = dyn_cast<VAArgInst>(I++);
      if(!VI)
        continue;
      if(VI == VASize)
        continue;
      Constant *One = ConstantInt::get(Int64Ty, 1);
      LoadInst *OldValue = new LoadInst(Counter, "count", VI);
      Instruction *NewValue = BinaryOperator::Create(BinaryOperator::Add,
                                                     OldValue,
                                                     One,
                                                     "count",
                                                     VI);
      new StoreInst(NewValue, Counter, VI);


      std::vector<Value *> Args;
      Args.push_back(VASize);
      Args.push_back(NewValue);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *Func = M.getOrInsertFunction("compareNumber", VoidTy, Int64Ty, Int64Ty,Int32Ty,  NULL);
      CallInst::Create(Func, Args.begin(), Args.end(), "", VI);

    }
  }
  return true;
}

bool
TypeChecks::visitByValFunction(Module &M, Function &F) {

  bool hasByValArg = false;
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    if (I->hasByValAttr()) {
      if(EnableTypeSafeOpt) {
        if(!TS->isTypeSafe(cast<Value>(&I), &F)) {
          hasByValArg = true;
          break;
        }
      } else {
        hasByValArg = true;
        break;
      }
    }
  }
  if(!hasByValArg)
    return false;

  // For internal functions
  //   Replace with a cloned function with extra arguments
  //   That takes as argument the original pointers without a byval parameter too
  //   Use them to copy the metadata over to the byval arguments
  //   Also update all the call sites.

  // For external functions
  //  Create an internal clone (treated same as internal functions)
  //  Modify the original function
  //  To assume that the metadata for the byval arguments is TOP 

  if(F.hasInternalLinkage()) {
    visitInternalFunction(M, F);
  } else {
    // create internal clone
    Function *F_clone = CloneFunction(&F);
    F_clone->setName(F.getNameStr() + "internal");
    F.setLinkage(GlobalValue::InternalLinkage);
    F.getParent()->getFunctionList().push_back(F_clone);
    visitInternalFunction(M, *F_clone);
    visitExternalFunction(M, F);
  }
  return true;
}

bool TypeChecks::visitInternalFunction(Module &M, Function &F) {

  // Create a list of the argument types in the new function.
  std::vector<const Type*>TP;
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    TP.push_back(I->getType());
    // for every byval argument, add a new argument that indicates the source of
    // the metadata. It is of the same type as the byval argument.
    if (I->hasByValAttr())
      TP.push_back(I->getType());
  }
  // Create the new function prototype
  const FunctionType *NewFTy = FunctionType::get(F.getReturnType(), TP, false);
  Function *NewF = Function::Create(NewFTy,
                                    GlobalValue::InternalLinkage,
                                    F.getNameStr() + ".INT",
                                    &M);

  Function::arg_iterator NI = NewF->arg_begin();
  DenseMap<const Value*, Value*> ValueMap;
  for (Function::arg_iterator II = F.arg_begin(); NI != NewF->arg_end(); ++II, ++NI) {
    // Each new argument maps to the argument in the old function
    // For these arguments, also copy over the attributes
    ValueMap[II] = NI;
    NI->setName(II->getName());
    NI->addAttr(F.getAttributes().getParamAttributes(II->getArgNo() + 1));
    // If we have encountered a byval argument in the old function
    // We must skip over the next argument in the new function, as that is 
    // the newly added source argument.
    if(II->hasByValAttr()) {
      NI++;
      // Give this new argument some name, for clarity
      NI->setName("src");
    }
  }
  // Copy over the attributes for the function.
  NewF->setAttributes(NewF->getAttributes()
                      .addAttr(0, F.getAttributes().getRetAttributes()));
  NewF->setAttributes(NewF->getAttributes().addAttr(~0, F.getAttributes().getFnAttributes()));

  // Perform the cloning.
  SmallVector<ReturnInst*,100> Returns;
  CloneFunctionInto(NewF, &F, ValueMap, Returns);

  // Add calls to the runtime to copy metadata from source to the byval argument pointer. 
  typedef SmallVector<Value *, 4> RegisteredArgTy;
  // Keep track of the byval arguments.
  RegisteredArgTy registeredArguments;
  for (Function::arg_iterator I = NewF->arg_begin(), E = NewF->arg_end(); I != E; ++I) {
    if (I->hasByValAttr()) {
      registeredArguments.push_back(&*I);
      assert (isa<PointerType>(I->getType()));
      const PointerType * PT = cast<PointerType>(I->getType());
      const Type * ET = PT->getElementType();
      Value * AllocSize = ConstantInt::get(Int64Ty, TD->getTypeAllocSize(ET));
      Instruction * InsertBefore = &(NewF->getEntryBlock().front());
      // If I is the byval argument, the next argument is the source
      CastInst *BCI_Dest = BitCastInst::CreatePointerCast(I, VoidPtrTy, "", InsertBefore);
      CastInst *BCI_Src = BitCastInst::CreatePointerCast(++I, VoidPtrTy, "", InsertBefore);
      std::vector<Value *> Args;
      Args.push_back(BCI_Dest);
      Args.push_back(BCI_Src);
      Args.push_back(AllocSize);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("copyTypeInfo", VoidTy, VoidPtrTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", InsertBefore);
    }
  }

  // Find all basic blocks which terminate the function.
  std::set<BasicBlock *> exitBlocks;
  for (inst_iterator I = inst_begin(NewF), E = inst_end(NewF); I != E; ++I) {
    if (isa<ReturnInst>(*I) || isa<UnwindInst>(*I)) {
      exitBlocks.insert(I->getParent());
    }
  }

  // At each function exit, insert code to set the metadata as uninitialized.
  for (std::set<BasicBlock*>::const_iterator BI = exitBlocks.begin(),
       BE = exitBlocks.end();
       BI != BE; ++BI) {
    for (RegisteredArgTy::const_iterator I = registeredArguments.begin(),
         E = registeredArguments.end();
         I != E; ++I) {
      SmallVector<Value *, 2> args;
      Instruction * Pt = &((*BI)->back());
      const PointerType * PT = cast<PointerType>((*I)->getType());
      const Type * ET = PT->getElementType();
      Value * AllocSize = ConstantInt::get(Int64Ty, TD->getTypeAllocSize(ET));
      CastInst *BCI = BitCastInst::CreatePointerCast(*I, VoidPtrTy, "", Pt);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(AllocSize);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackUnInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", Pt);
    }
  }

  // Update the call sites
  for(Value::use_iterator ui = F.use_begin(), ue = F.use_end();
      ui != ue;)  {
    // Check that F is the called value
    if(CallInst *CI = dyn_cast<CallInst>(ui++)) {
      if(CI->getCalledFunction() == &F) {
        SmallVector<Value*, 8> Args;
        SmallVector<AttributeWithIndex, 8> AttributesVec;

        // Get the initial attributes of the call
        AttrListPtr CallPAL = CI->getAttributes();
        Attributes RAttrs = CallPAL.getRetAttributes();
        Attributes FnAttrs = CallPAL.getFnAttributes();

        Function::arg_iterator II = F.arg_begin();

        for(unsigned j =1;j<CI->getNumOperands();j++, II++) {
          // Add the original argument
          Args.push_back(CI->getOperand(j));
          // If there are attributes on this argument, copy them to the correct 
          // position in the AttributesVec
          if (Attributes Attrs = CallPAL.getParamAttributes(j))
            AttributesVec.push_back(AttributeWithIndex::get(Args.size(), Attrs));
          // If it is a value passed as byval, add it again, as the source
          if(II->hasByValAttr()) 
            Args.push_back(CI->getOperand(j));
        }

        // Create the new attributes vec.
        if (FnAttrs != Attribute::None)
          AttributesVec.push_back(AttributeWithIndex::get(~0, FnAttrs));
        if (RAttrs)
          AttributesVec.push_back(AttributeWithIndex::get(0, RAttrs));

        AttrListPtr NewCallPAL = AttrListPtr::get(AttributesVec.begin(),
                                                  AttributesVec.end());


        // Create the substitute call
        CallInst *CallI = CallInst::Create(NewF,Args.begin(), Args.end(),"", CI);
        CallI->setCallingConv(CI->getCallingConv());
        CallI->setAttributes(NewCallPAL);
        CI->replaceAllUsesWith(CallI);
        CI->eraseFromParent();
      }
    }
  }
  return true;
}

bool TypeChecks::visitExternalFunction(Module &M, Function &F) {

  // A list of the byval arguments that we are setting metadata for
  typedef SmallVector<Value *, 4> RegisteredArgTy;
  RegisteredArgTy registeredArguments;
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    if (I->hasByValAttr()) {
      assert (isa<PointerType>(I->getType()));
      const PointerType * PT = cast<PointerType>(I->getType());
      const Type * ET = PT->getElementType();
      Value * AllocSize = ConstantInt::get(Int64Ty, TD->getTypeAllocSize(ET));
      Instruction * InsertBefore = &(F.getEntryBlock().front());
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy, "", InsertBefore);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(AllocSize);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      // Set the metadata for the byval argument to TOP/Initialized
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", InsertBefore);
      registeredArguments.push_back(&*I);
    }
  }

  // Find all basic blocks which terminate the function.
  std::set<BasicBlock *> exitBlocks;
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    if (isa<ReturnInst>(*I) || isa<UnwindInst>(*I)) {
      exitBlocks.insert(I->getParent());
    }
  }

  // At each function exit, insert code to set the metadata as uninitialized.
  for (std::set<BasicBlock*>::const_iterator BI = exitBlocks.begin(),
       BE = exitBlocks.end();
       BI != BE; ++BI) {
    for (RegisteredArgTy::const_iterator I = registeredArguments.begin(),
         E = registeredArguments.end();
         I != E; ++I) {
      SmallVector<Value *, 2> args;
      Instruction * Pt = &((*BI)->back());
      const PointerType * PT = cast<PointerType>((*I)->getType());
      const Type * ET = PT->getElementType();
      Value * AllocSize = ConstantInt::get(Int64Ty, TD->getTypeAllocSize(ET));
      CastInst *BCI = BitCastInst::CreatePointerCast(*I, VoidPtrTy, "", Pt);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(AllocSize);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackUnInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", Pt);
    }
  }
  return true;
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
bool TypeChecks::initShadow(Module &M) {
  // Create the call to the runtime initialization function and place it before the store instruction.

  Constant * RuntimeCtor = M.getOrInsertFunction("shadowInit", VoidTy, NULL);

  //
  // Insert the run-time ctor into the ctor list.
  //
  std::vector<Constant *> CtorInits;
  CtorInits.push_back (ConstantInt::get (Int32Ty, 65535));
  CtorInits.push_back (RuntimeCtor);
  Constant * RuntimeCtorInit=ConstantStruct::get(M.getContext(),CtorInits, false);

  //
  // Get the current set of static global constructors and add the new ctor
  // to the list.
  //
  std::vector<Constant *> CurrentCtors;
  GlobalVariable * GVCtor = M.getNamedGlobal ("llvm.global_ctors");
  if (GVCtor) {
    if (Constant * C = GVCtor->getInitializer()) {
      for (unsigned index = 0; index < C->getNumOperands(); ++index) {
        CurrentCtors.push_back (cast<Constant>(C->getOperand (index)));
      }
    }

    //
    // Rename the global variable so that we can name our global
    // llvm.global_ctors.
    //
    GVCtor->setName ("removed");
  }

  //
  // The ctor list seems to be initialized in different orders on different
  // platforms, and the priority settings don't seem to work.  Examine the
  // module's platform string and take a best guess to the order.
  //
  if (M.getTargetTriple().find ("linux") == std::string::npos)
    CurrentCtors.insert (CurrentCtors.begin(), RuntimeCtorInit);
  else
    CurrentCtors.push_back (RuntimeCtorInit);

  //
  // Create a new initializer.
  //
  const ArrayType * AT = ArrayType::get (RuntimeCtorInit-> getType(),
                                         CurrentCtors.size());
  Constant * NewInit=ConstantArray::get (AT, CurrentCtors);

  //
  // Create the new llvm.global_ctors global variable and replace all uses of
  // the old global variable with the new one.
  //
  new GlobalVariable (M,
                      NewInit->getType(),
                      false,
                      GlobalValue::AppendingLinkage,
                      NewInit,
                      "llvm.global_ctors");


  return true;
}

// Initialize the shadow memory which contains the 1:1 mapping.
bool TypeChecks::unmapShadow(Module &M, Instruction &I) {
  // Create the call to the runtime shadow memory unmap function and place it before any exiting instruction.
  Constant *F = M.getOrInsertFunction("shadowUnmap", VoidTy, NULL);
  CallInst::Create(F, "", &I);

  return true;
}

bool TypeChecks::visitGlobal(Module &M, GlobalVariable &GV, 
                             Constant *C, Instruction &I, unsigned offset) {

  // FIXME:This should maybe move into the global ctor.

  /*if(EnableTypeSafeOpt) {
    if(TS->isTypeSafe(&GV, I.getParent()->getParent())) {
    return false;
    }
    }*/

  if(ConstantArray *CA = dyn_cast<ConstantArray>(C)) {
    const Type * ElementType = CA->getType()->getElementType();
    unsigned int t = TD->getTypeStoreSize(ElementType);
    // Create the type entry for the first element
    // using recursive creation till we get to the base types
    visitGlobal(M, GV, CA->getOperand(0), I, offset);

    // Copy the type metadata for the first element
    // over for the rest of the elements.
    CastInst *BCI = BitCastInst::CreatePointerCast(&GV, VoidPtrTy, "", &I);
    std::vector<Value *> Args;
    Args.push_back(BCI);
    Args.push_back(ConstantInt::get(Int64Ty, t));
    Args.push_back(ConstantInt::get(Int64Ty, CA->getNumOperands()));
    Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
    Constant *F = M.getOrInsertFunction("trackGlobalArray", VoidTy, VoidPtrTy, Int64Ty, Int64Ty, Int32Ty, NULL);
    CallInst::Create(F, Args.begin(), Args.end(), "", &I);
  }
  else if(ConstantStruct *CS = dyn_cast<ConstantStruct>(C)) {
    // Create metadata for each field of the struct
    // at the correct offset.
    const StructLayout *SL = TD->getStructLayout(cast<StructType>(CS->getType()));
    for (unsigned i = 0, e = CS->getNumOperands(); i != e; ++i) {
      if (SL->getElementOffset(i) < SL->getSizeInBytes()) {
        Constant * ConstElement = cast<Constant>(CS->getOperand(i));
        unsigned field_offset = offset + (unsigned)SL->getElementOffset(i);
        visitGlobal(M, GV, ConstElement, I, field_offset);
      }
    }
  } else if(ConstantAggregateZero *CAZ = dyn_cast<ConstantAggregateZero>(C)) {
    // Similiar to having an initializer with all values NULL
    // Must set metadata, similiar to the previous 2 cases.
    const Type *Ty = CAZ->getType();
    if(const ArrayType * ATy = dyn_cast<ArrayType>(Ty)) {
      const Type * ElementType = ATy->getElementType();
      unsigned int t = TD->getTypeStoreSize(ElementType);
      visitGlobal(M, GV, Constant::getNullValue(ElementType), I, offset);
      CastInst *BCI = BitCastInst::CreatePointerCast(&GV, VoidPtrTy, "", &I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(ConstantInt::get(Int64Ty, t));
      Args.push_back(ConstantInt::get(Int64Ty, ATy->getNumElements()));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackGlobalArray", VoidTy, VoidPtrTy, Int64Ty, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", &I);
    } else if(const StructType *STy = dyn_cast<StructType>(Ty)) {
      const StructLayout *SL = TD->getStructLayout(STy);
      for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
        if (SL->getElementOffset(i) < SL->getSizeInBytes()) {
          unsigned field_offset = offset + (unsigned)SL->getElementOffset(i);
          visitGlobal(M, GV, Constant::getNullValue(STy->getElementType(i)), I, field_offset);
        }
      }
    } else {
      // Zeroinitializer of a primitive type
      CastInst *BCI = BitCastInst::CreatePointerCast(&GV, VoidPtrTy, "", &I);
      SmallVector<Value*, 8> Indices;
      Indices.push_back(ConstantInt::get(Int32Ty, offset));
      GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(BCI, Indices.begin(),
                                                                 Indices.end(),"", &I) ;

      std::vector<Value *> Args;
      Args.push_back(GEP);
      Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[CAZ->getType()]));
      Args.push_back(ConstantInt::get(Int64Ty, TD->getTypeStoreSize(CAZ->getType())));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackGlobal", VoidTy, VoidPtrTy, Int8Ty, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", &I);
    }
  }
  else {
    // Primitive type value
    CastInst *BCI = BitCastInst::CreatePointerCast(&GV, VoidPtrTy, "", &I);
    SmallVector<Value*, 8> Indices;
    Indices.push_back(ConstantInt::get(Int32Ty, offset));
    GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(BCI, Indices.begin(),
                                                               Indices.end(),"", &I) ;

    std::vector<Value *> Args;
    Args.push_back(GEP);
    Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[C->getType()]));
    Args.push_back(ConstantInt::get(Int64Ty, TD->getTypeStoreSize(C->getType())));
    Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
    Constant *F = M.getOrInsertFunction("trackGlobal", VoidTy, VoidPtrTy, Int8Ty, Int64Ty, Int32Ty, NULL);
    CallInst::Create(F, Args.begin(), Args.end(), "", &I);
  }

  return true;
}

// Insert code to initialize meta data to bottom
// Insert code to set objects to 0
bool TypeChecks::visitAllocaInst(Module &M, AllocaInst &AI) {

  // Set the object to be zero
  //
  // Add the memset function to the program.
  Constant *memsetF = M.getOrInsertFunction ("llvm.memset.i64", VoidTy,
                                             VoidPtrTy,
                                             Int8Ty,
                                             Int64Ty,
                                             Int32Ty,
                                             NULL);

  const PointerType * PT = AI.getType();
  const Type * ET = PT->getElementType();
  Value * AllocSize = ConstantInt::get(Int64Ty, TD->getTypeAllocSize(ET));
  CastInst *BCI = BitCastInst::CreatePointerCast(&AI, VoidPtrTy);
  BCI->insertAfter(&AI);

  CastInst *ArraySize = CastInst::CreateSExtOrBitCast(AI.getArraySize(), Int64Ty);
  ArraySize->insertAfter(BCI);
  BinaryOperator *Size = BinaryOperator::Create(Instruction::Mul, AllocSize, ArraySize);
  Size->insertAfter(ArraySize);
  std::vector<Value *> Args2;
  Args2.push_back(BCI);
  Args2.push_back(ConstantInt::get(Int8Ty, 0));
  Args2.push_back(Size);
  Args2.push_back(ConstantInt::get(Int32Ty, AI.getAlignment()));
  CallInst *CI_Init = CallInst::Create(memsetF, Args2.begin(), Args2.end());
  CI_Init->insertAfter(Size);

  if(EnableTypeSafeOpt) {
    if(TS->isTypeSafe(&AI, AI.getParent()->getParent())) {
      return true;
    }
  }

  // Setting metadata to be 0(BOTTOM/Uninitialized)

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(AllocSize);
  Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
  Constant *F = M.getOrInsertFunction("trackUnInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
  CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
  CI->insertAfter(BCI);
  std::vector<Value *> Args1;
  Args1.push_back(BCI);
  Args1.push_back(AllocSize);
  Args1.push_back(AI.getArraySize());
  Args1.push_back(ConstantInt::get(Int32Ty, tagCounter++));
  F = M.getOrInsertFunction("trackGlobalArray", VoidTy, VoidPtrTy, Int64Ty, AI.getArraySize()->getType(), Int32Ty, NULL);
  CallInst *CI_Arr = CallInst::Create(F, Args1.begin(), Args1.end());
  CI_Arr->insertAfter(CI);

  return true;
}

// Insert runtime check for va_arg instructions
bool TypeChecks::visitVAArgInst(Module &M, VAArgInst &VI) {
  Function *Func = VI.getParent()->getParent();
  if(!Func->hasInternalLinkage())
    return false;

  // FIXME:handle external functions
  // For every va_arg instruction,
  // In a transformed function, we pass in the type metadata before the actual argument
  // Read metadata from va_list
  // And then add a compare in the runtime
  VAArgInst *VI_Type = new VAArgInst(VI.getOperand(0), Int8Ty, "", &VI);
  std::vector<Value *> Args;
  Args.push_back(VI_Type);
  Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[VI.getType()]));
  Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
  Constant *F = M.getOrInsertFunction("compareTypes", VoidTy, Int8Ty, Int8Ty,Int32Ty,  NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &VI);
  return true;
}

// Insert runtime checks for certain call instructions
bool TypeChecks::visitCallInst(Module &M, CallInst &CI) {
  return visitCallSite(M, &CI);
}

// Insert runtime checks for certain call instructions
bool TypeChecks::visitInvokeInst(Module &M, InvokeInst &II) {
  return visitCallSite(M, &II);
}

bool TypeChecks::visitCallSite(Module &M, CallSite CS) {
  //
  // Get the called value.  Strip off any casts which are lossless.
  //
  Value *Callee = CS.getCalledValue()->stripPointerCasts();
  Instruction *I = CS.getInstruction();

  // Special case handling of certain libc allocation functions here.
  if (Function *F = dyn_cast<Function>(Callee)) {
    if (F->isIntrinsic()) {
      switch(F->getIntrinsicID()) {
      case Intrinsic::memcpy: 
      case Intrinsic::memmove: 
        {
          if(EnableTypeSafeOpt) {
            if(TS->isTypeSafe(I->getOperand(2), I->getParent()->getParent())) {
              return false;
            }
          }
          CastInst *BCI_Src = BitCastInst::CreatePointerCast(I->getOperand(2), VoidPtrTy, "", I);
          CastInst *BCI_Dest = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
          std::vector<Value *> Args;
          Args.push_back(BCI_Dest);
          Args.push_back(BCI_Src);
          Args.push_back(I->getOperand(3));
          Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
          Constant *F = M.getOrInsertFunction("copyTypeInfo", VoidTy, VoidPtrTy, VoidPtrTy, I->getOperand(3)->getType(), Int32Ty, NULL);
          CallInst::Create(F, Args.begin(), Args.end(), "", I);
          return true;
        }

      case Intrinsic::memset:
        if(EnableTypeSafeOpt) {
          if(TS->isTypeSafe(I->getOperand(1), I->getParent()->getParent())) {
            return false;
          }
        }
        CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
        std::vector<Value *> Args;
        Args.push_back(BCI);
        Args.push_back(I->getOperand(3));
        Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
        Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
        CallInst::Create(F, Args.begin(), Args.end(), "", I);
        return true;
      }
    } else if(F->getNameStr() == std::string("ftime")) {
      if(EnableTypeSafeOpt) {
        if(TS->isTypeSafe(I->getOperand(1), I->getParent()->getParent())) {
          return false;
        }
      }
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
      const PointerType *PTy = cast<PointerType>(I->getOperand(1)->getType());
      const Type * ElementType = PTy->getElementType();
      unsigned int t = TD->getTypeStoreSize(ElementType);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(ConstantInt::get(Int64Ty, t));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", I);
      return true;
    } else if(F->getNameStr() == std::string("read")) {
      if(EnableTypeSafeOpt) {
        if(TS->isTypeSafe(I->getOperand(2), I->getParent()->getParent())) {
          return false;
        }
      }
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(2), VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(I);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, I->getType(), Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI);
      return true;
    } else if(F->getNameStr() == std::string("fread")) {
      if(EnableTypeSafeOpt) {
        if(TS->isTypeSafe(I->getOperand(1), I->getParent()->getParent())) {
          return false;
        }
      }
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(I);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, I->getType(), Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI);
      return true;
    } else if(F->getNameStr() == std::string("calloc")) {
      if(EnableTypeSafeOpt) {
        if(TS->isTypeSafe(I, I->getParent()->getParent())) {
          return false;
        }
      }
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(I->getOperand(2));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy,I->getOperand(2)->getType(), Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI);
      std::vector<Value *> Args1;
      Args1.push_back(BCI);
      Args1.push_back(I->getOperand(2));
      Args1.push_back(I->getOperand(1));
      Args1.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      F = M.getOrInsertFunction("trackGlobalArray", VoidTy, VoidPtrTy, I->getOperand(2)->getType(), I->getOperand(1)->getType(), Int32Ty, NULL);
      CallInst *CI_Arr = CallInst::Create(F, Args1.begin(), Args1.end());
      CI_Arr->insertAfter(CI);
      return true;
    } else if(F->getNameStr() ==  std::string("realloc")) {
      if(EnableTypeSafeOpt) {
        if(TS->isTypeSafe(I, I->getParent()->getParent())) {
          return false;
        }
      }
      CastInst *BCI_Src = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy);
      CastInst *BCI_Dest = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI_Src->insertAfter(I);
      BCI_Dest->insertAfter(BCI_Src);
      std::vector<Value *> Args;
      Args.push_back(BCI_Dest);
      Args.push_back(BCI_Src);
      Args.push_back(I->getOperand(2));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("copyTypeInfo", VoidTy, VoidPtrTy, VoidPtrTy, I->getOperand(2)->getType(), Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI_Dest);
      return true;
    } else if(F->getNameStr() == std::string("fgets")) {
      if(EnableTypeSafeOpt) {
        if(TS->isTypeSafe(I->getOperand(1), I->getParent()->getParent())) {
          return true;
        }
      }
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(I->getOperand(2));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, I->getOperand(2)->getType(), Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", I);
      return true;
    } else if(F->getNameStr() == std::string("sscanf")) {
      // FIXME: Need to look at the format string and check
      unsigned i = 3;
      while(i < I->getNumOperands()) {
        visitInputFunctionValue(M, I->getOperand(i), I);
        i++;
      }
    } else if(F->getNameStr() == std::string("fscanf")) {
      unsigned i = 3;
      while(i < I->getNumOperands()) {
        visitInputFunctionValue(M, I->getOperand(i), I);
        i++;
      }
    }
  }
  return false;
}

bool TypeChecks::visitInputFunctionValue(Module &M, Value *V, Instruction *CI) {
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(V, VoidPtrTy, "", CI);
  const PointerType *PTy = dyn_cast<PointerType>(V->getType());
  if(!PTy)
    return false;

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[PTy->getElementType()])); // SI.getValueOperand()
  Args.push_back(ConstantInt::get(Int64Ty, TD->getTypeStoreSize(PTy->getElementType())));
  Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));

  // Create the call to the runtime check and place it before the store instruction.
  Constant *F = M.getOrInsertFunction("trackStoreInst", VoidTy, VoidPtrTy, Int8Ty, Int64Ty, Int32Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", CI);

  return true;
}

// Insert runtime checks before all load instructions.
bool TypeChecks::visitLoadInst(Module &M, LoadInst &LI) {
  if(EnableTypeSafeOpt) {
    if(TS->isTypeSafe(LI.getOperand(0), LI.getParent()->getParent())) {
      return false;
    }
  }
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(LI.getPointerOperand(), VoidPtrTy, "", &LI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[LI.getType()]));
  Args.push_back(ConstantInt::get(Int64Ty, TD->getTypeStoreSize(LI.getType())));
  Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));

  // Create the call to the runtime check and place it before the load instruction.
  Constant *F = M.getOrInsertFunction("trackLoadInst", VoidTy, VoidPtrTy, Int8Ty, Int64Ty, Int32Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &LI);
  numLoadChecks++;
  return true;
}

// Insert runtime checks before all store instructions.
bool TypeChecks::visitStoreInst(Module &M, StoreInst &SI) {
  if(EnableTypeSafeOpt) {
    if(TS->isTypeSafe(SI.getOperand(1), SI.getParent()->getParent())) {
      return false;
    }
  }
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(SI.getPointerOperand(), VoidPtrTy, "", &SI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(ConstantInt::get(Int8Ty, UsedTypes[SI.getOperand(0)->getType()])); // SI.getValueOperand()
  Args.push_back(ConstantInt::get(Int64Ty, TD->getTypeStoreSize(SI.getOperand(0)->getType())));
  Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));

  // Create the call to the runtime check and place it before the store instruction.
  Constant *F = M.getOrInsertFunction("trackStoreInst", VoidTy, VoidPtrTy, Int8Ty, Int64Ty, Int32Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &SI);
  numStoreChecks++;

  return true;
}

// Insert runtime checks before copying store instructions.
bool TypeChecks::visitCopyingStoreInst(Module &M, StoreInst &SI, Value *SS) {
  if(EnableTypeSafeOpt) {
    if(TS->isTypeSafe(SI.getOperand(1), SI.getParent()->getParent())) {
      return false;
    }
  }
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI_Dest = BitCastInst::CreatePointerCast(SI.getPointerOperand(), VoidPtrTy, "", &SI);
  CastInst *BCI_Src = BitCastInst::CreatePointerCast(SS, VoidPtrTy, "", &SI);

  std::vector<Value *> Args;
  Args.push_back(BCI_Dest);
  Args.push_back(BCI_Src);
  Args.push_back(ConstantInt::get(Int64Ty, TD->getTypeStoreSize(SI.getOperand(0)->getType())));
  Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));

  // Create the call to the runtime check and place it before the copying store instruction.
  Constant *F = M.getOrInsertFunction("copyTypeInfo", VoidTy, VoidPtrTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
  CallInst::Create(F, Args.begin(), Args.end(), "", &SI);
  numStoreChecks++;

  return true;
}
