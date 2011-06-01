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
STATISTIC(numTypes, "Number of Types used in the module");

namespace {
  static cl::opt<bool> EnableTypeSafeOpt("enable-type-safe-opt",
         cl::desc("Use DSA pass"),
         cl::Hidden,
         cl::init(false));
  static cl::opt<bool> DisablePointerTypeChecks("disable-ptr-type-checks",
         cl::desc("DONT Distinguish pointer types"),
         cl::Hidden,
         cl::init(false));
}

static int tagCounter = 0;
static const Type *VoidTy = 0;
static const Type *Int8Ty = 0;
static const Type *Int32Ty = 0;
static const Type *Int64Ty = 0;
static const PointerType *VoidPtrTy = 0;

unsigned int 
TypeChecks::getTypeMarker(const Type * Ty) {
  if(DisablePointerTypeChecks) {
    if(Ty->isPointerTy()) {
      Ty = VoidPtrTy;
    }
  }
  if(UsedTypes.find(Ty) == UsedTypes.end())
    UsedTypes[Ty] = UsedTypes.size();

  return UsedTypes[Ty];
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
  VAListFunctions.clear();
  VAArgFunctions.clear();
  ByValFunctions.clear();

  Function *MainF = M.getFunction("main");
  if (MainF == 0 || MainF->isDeclaration()) {
    assert(0 && "No main function found");
    return false;
  }

  // Insert the shadow initialization function.
  modified |= initShadow(M);

  // record argv
  modified |= visitMain(M, *MainF);
  

  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    Function &F = *MI;
    if(F.isDeclaration())
      continue;

    std::string name = F.getName();
    if (strncmp(name.c_str(), "tc.", 3) == 0) continue;

    // check for byval arguments
    bool hasByValArg = false;
    for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
      if (I->hasByValAttr()) {
        hasByValArg = true;
        break;
      }
    }
    if(hasByValArg) {
      ByValFunctions.push_back(&F);
    }

    // Iterate and find all varargs functions
    if(F.isVarArg()) {
      VAArgFunctions.push_back(&F);
      continue;
    }
    // Iterate and find all VAList functions
    bool isVAListFunc = false;
    const Type *ListType  = M.getTypeByName("struct.__va_list_tag");
    if(!ListType)
      continue;

    const Type *ListPtrType = ListType->getPointerTo();
    for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
      if(I->getType() == ListPtrType) {
        isVAListFunc = true;
        break;
      }
    }
    if(isVAListFunc) {
      VAListFunctions.push_back(&F);
      continue;
    }
  }
  
  while(!ByValFunctions.empty()) {
    Function *F = ByValFunctions.back();
    ByValFunctions.pop_back();
    modified |= visitByValFunction(M, *F);
  }

  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    Function &F = *MI;
    if(F.isDeclaration())
      continue;

    // Loop over all of the instructions in the function, 
    // adding their return type as well as the types of their operands.
    for (inst_iterator II = inst_begin(F), IE = inst_end(F); II != IE; ++II) {
      Instruction &I = *II;
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
      }
    }
  }


  // NOTE:must visit before VAArgFunctions, to populate the map with the
  // correct cloned functions.
  while(!VAListFunctions.empty()) {
    Function *F = VAListFunctions.back();
    VAListFunctions.pop_back();
    modified |= visitVAListFunction(M, *F);
  }

  // iterate through all the VAList funtions and modify call sites
  // to call the new function 
  std::map<Function *, Function *>::iterator FI = VAListFunctionsMap.begin(), FE = VAListFunctionsMap.end();
  for(; FI != FE; FI++) {
    visitVAListCall(FI->second);
  }
  while(!VAArgFunctions.empty()) {
    Function *F = VAArgFunctions.back();
    VAArgFunctions.pop_back();
    assert(F->isVarArg());
    modified |= visitVarArgFunction(M, *F);
  }

  addTypeMapGlobal(M);
  numTypes += UsedTypes.size();

  return modified;
}

void TypeChecks::addTypeMapGlobal(Module &M) {

  // add a global that has the metadata -> typeString mapping
  ArrayType*  AType = ArrayType::get(VoidPtrTy, UsedTypes.size() + 1);
  std::vector<Constant *> Values;
  Values.reserve(UsedTypes.size() + 1);
  std::vector<Constant *> Indices;
  Indices.push_back(ConstantInt::get(Int32Ty,0));
  Indices.push_back(ConstantInt::get(Int32Ty,0));

  // Add an entry for uninitialized(Type Number = 0)

  Constant *CA = ConstantArray::get(M.getContext(), "UNINIT", true);
  GlobalVariable *GV = new GlobalVariable(M, 
                                          CA->getType(),
                                          true,
                                          GlobalValue::ExternalLinkage,
                                          CA,
                                          "");
  GV->setInitializer(CA);
  Constant *C = ConstantExpr::getGetElementPtr(GV, &Indices[0], Indices.size());
  Values[0] = C;

  std::map<const Type*, unsigned int >::iterator TI = UsedTypes.begin(), TE = UsedTypes.end(); 
  for(;TI!=TE; ++TI) {
    std::string *type = new std::string();
    llvm::raw_string_ostream *test = new llvm::raw_string_ostream(*type);

    WriteTypeSymbolic(*test, TI->first, &M);
    Constant *CA = ConstantArray::get(M.getContext(), test->str(), true);
    GlobalVariable *GV = new GlobalVariable(M, 
                                            CA->getType(),
                                            true,
                                            GlobalValue::ExternalLinkage,
                                            CA,
                                            "");
    GV->setInitializer(CA);
    Constant *C = ConstantExpr::getGetElementPtr(GV, &Indices[0], Indices.size());
    Values[TI->second]= C;
  }

  new GlobalVariable(M, 
                     AType,
                     true,
                     GlobalValue::ExternalLinkage,
                     ConstantArray::get(AType, &Values[0], UsedTypes.size() + 1),
                     "typeNames"
                    );

  return;
}

void TypeChecks::visitVAListCall(Function *F) {
  for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
    for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
      CallInst *CI = dyn_cast<CallInst>(I++);
      if(!CI)
        continue;
      Function *CalledF = dyn_cast<Function>(CI->getCalledFunction());
      if(VAListFunctionsMap.find(CalledF) == VAListFunctionsMap.end())
        continue;
      Function::arg_iterator NII = F->arg_begin();
      std::vector<Value *>Args;
      Args.push_back(NII++); // toatl count
      Args.push_back(NII++); // current count
      Args.push_back(NII); // MD
      for(unsigned i = 1 ;i < CI->getNumOperands(); i++) {
        // Add the original argument
        Args.push_back(CI->getOperand(i));
      }
      CallInst *CINew = CallInst::Create(VAListFunctionsMap[CalledF], Args.begin(), Args.end(), "", CI);
      CI->replaceAllUsesWith(CINew);
      CI->eraseFromParent();
    }
  }
}

  bool TypeChecks::visitVAListFunction(Module &M, Function &F_orig) {
    if(!F_orig.hasInternalLinkage())
      return false;

    int VAListArgNum = 0;
    // Check if one of the arguments is a va_list
    const Type *ListType  = M.getTypeByName("struct.__va_list_tag");
    if(!ListType)
      return false;
    const Type *ListPtrType = ListType->getPointerTo();
    Argument *VAListArg = NULL; 
    for (Function::arg_iterator I = F_orig.arg_begin(), E = F_orig.arg_end(); I != E; ++I) {
      VAListArgNum ++;
      if(I->getType() == ListPtrType) {
        VAListArg = I;
        break;
      }
    }

    // Clone the function to add arguments for count, MD

    // 1. Create the new argument types vector
    std::vector<const Type*>TP;
    TP.push_back(Int64Ty); // for count
    TP.push_back(Int64Ty); // for count
    TP.push_back(VoidPtrTy); // for MD
    for (Function::arg_iterator I = F_orig.arg_begin(), E = F_orig.arg_end(); I != E; ++I) {
      TP.push_back(I->getType());
    }
    // 2. Create the new function prototype
    const FunctionType *NewFTy = FunctionType::get(F_orig.getReturnType(), TP, false);
    Function *F = Function::Create(NewFTy,
                                   GlobalValue::InternalLinkage,
                                   F_orig.getNameStr() + ".INT",
                                   &M);

    // 3. Set the mapping for args
    Function::arg_iterator NI = F->arg_begin();
    DenseMap<const Value*, Value*> ValueMap;
    NI->setName("TotalCount");
    NI++;
    NI->setName("CurrentCount");
    NI++;
    NI->setName("MD");
    NI++;
    for (Function::arg_iterator II = F_orig.arg_begin(); NI != F->arg_end(); ++II, ++NI) {
      // Each new argument maps to the argument in the old function
      // For these arguments, also copy over the attributes
      ValueMap[II] = NI;
      NI->setName(II->getName());
      NI->addAttr(F_orig.getAttributes().getParamAttributes(II->getArgNo() + 1));
    }

    // 4. Copy over the attributes for the function.
    F->setAttributes(F->getAttributes()
                     .addAttr(0, F_orig.getAttributes().getRetAttributes()));
    F->setAttributes(F->getAttributes().addAttr(~0, F_orig.getAttributes().getFnAttributes()));

    // 5. Perform the cloning.
    SmallVector<ReturnInst*,100> Returns;
    CloneFunctionInto(F, &F_orig, ValueMap, Returns);

    VAListFunctionsMap[&F_orig] =  F;
    inst_iterator InsPt = inst_begin(F);

    // Store the information
    Function::arg_iterator NII = F->arg_begin();
    AllocaInst *VASizeLoc = new AllocaInst(Int64Ty, "", &*InsPt);
    new StoreInst(NII, VASizeLoc, &*InsPt);
    NII++;
    AllocaInst *Counter = new AllocaInst(Int64Ty, "",&*InsPt);
    new StoreInst(NII, Counter, &*InsPt); 
    NII++;
    AllocaInst *VAMDLoc = new AllocaInst(VoidPtrTy, "", &*InsPt);
    new StoreInst(NII, VAMDLoc, &*InsPt);

    // instrument va_arg to increment the counter
    for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {
      for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
        VAArgInst *VI = dyn_cast<VAArgInst>(I++);
        if(!VI)
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
        Instruction *VASize = new LoadInst(VASizeLoc, "", VI);
        Instruction *VAMetaData = new LoadInst(VAMDLoc, "", VI);
        Args.push_back(VASize);
        Args.push_back(OldValue);
        Args.push_back(ConstantInt::get(Int8Ty, getTypeMarker(VI->getType())));
        Args.push_back(VAMetaData);
        Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
        Constant *Func = M.getOrInsertFunction("compareTypeAndNumber", VoidTy, Int64Ty, Int64Ty, Int8Ty, VoidPtrTy, Int32Ty, NULL);
        CallInst::Create(Func, Args.begin(), Args.end(), "", VI);
      }
    }

    return true;
  }

// Transform Variable Argument functions, by also passing
// the relavant metadata info
bool TypeChecks::visitVarArgFunction(Module &M, Function &F) {
  if(F.hasInternalLinkage()) {
    return visitInternalVarArgFunction(M, F);
  }

  // create internal clone
  Function *F_clone = CloneFunction(&F);
  F_clone->setName(F.getNameStr() + "internal");
  F.setLinkage(GlobalValue::InternalLinkage);
  F.getParent()->getFunctionList().push_back(F_clone);
  F.replaceAllUsesWith(F_clone);
  return visitInternalVarArgFunction(M, *F_clone);
  return true;
}

// each vararg function is modified so that the first
// va_arg is the number of arguments in the va_list,
// and the second is a pointer to a metadata array, 
// containing type information for each of the arguments
// in the va_list.

// These are read and stored on a call to va_start.
// There can be multiple calls to va_start in a given
// function, which is why these are stored in memory

// We keep a counter for the number of arguments accessed
// from the va_list(Counter). It is incremented and 
// checked on every va_arg access. It is initialized to zero.
// It is also reset to zero on a call to va_start.

// Similiarly we check type on every va_arg access.

// Aside from this, this function also transforms all
// callsites of the var_arg function.

bool TypeChecks::visitInternalVarArgFunction(Module &M, Function &F) {

  inst_iterator InsPt = inst_begin(F);

  AllocaInst *VASizeLoc = new AllocaInst(Int64Ty, "", &*InsPt);
  AllocaInst *VAMDLoc = new AllocaInst(VoidPtrTy, "", &*InsPt);

  // Modify function to add checks on every var_arg call to ensure that we
  // are not accessing more arguments than we passed in.

  // Add a counter variable to the function entry
  AllocaInst *Counter = new AllocaInst(Int64Ty, "",&*InsPt);
  new StoreInst(ConstantInt::get(Int64Ty, 0), Counter, &*InsPt); 

  // Increment the counter
  for (Function::iterator B = F.begin(), FE = F.end(); B != FE; ++B) {
    for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
      VAArgInst *VI = dyn_cast<VAArgInst>(I++);
      if(!VI)
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
      Instruction *VASize = new LoadInst(VASizeLoc, "", VI);
      Instruction *VAMetaData = new LoadInst(VAMDLoc, "", VI);
      Args.push_back(VASize);
      Args.push_back(OldValue);
      Args.push_back(ConstantInt::get(Int8Ty, getTypeMarker(VI->getType())));
      Args.push_back(VAMetaData);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *Func = M.getOrInsertFunction("compareTypeAndNumber", 
                                             VoidTy, 
                                             Int64Ty, 
                                             Int64Ty, 
                                             Int8Ty, 
                                             VoidPtrTy, 
                                             Int32Ty, NULL);
      CallInst::Create(Func, Args.begin(), Args.end(), "", VI);
    }
  }

  // store the metadata
  CallInst *VAStart = NULL;
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
      VAStart = CI;
      // Modify the function to add a call to get the num of arguments
      VAArgInst *VASize = new VAArgInst(CI->getOperand(1), Int64Ty, "NumArgs");
      // Modify the function to add a call to get the metadata array
      VAArgInst *VAMetaData = new VAArgInst(CI->getOperand(1), VoidPtrTy, "MD");
      VASize->insertAfter(CI);
      VAMetaData->insertAfter(VASize);

      // Store the metadata
      StoreInst *SI1 = new StoreInst(VASize, VASizeLoc);
      SI1->insertAfter(VAMetaData);
      StoreInst *SI2 = new StoreInst(VAMetaData, VAMDLoc);
      SI2->insertAfter(SI1);

      // Reinitialize the counter
      StoreInst *SI3 = new StoreInst(ConstantInt::get(Int64Ty, 0), Counter);
      SI3->insertAfter(SI2);
    }
  }

  assert(VAStart && "Varargs function without a call to VAStart???");
  // modify calls to va list functions to pass the metadata
  for (Function::iterator B = F.begin(), FE = F.end(); B != FE; ++B) {
    for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
      CallInst *CI = dyn_cast<CallInst>(I++);
      if(!CI)
        continue;
      Function *CalledF = dyn_cast<Function>(CI->getCalledFunction());
      if(VAListFunctionsMap.find(CalledF) == VAListFunctionsMap.end())
        continue;
      std::vector<Value *>Args;
      Instruction *VASize = new LoadInst(VASizeLoc, "", CI);
      Instruction *VACounter = new LoadInst(Counter, "", CI);
      Instruction *VAMetaData = new LoadInst(VAMDLoc, "", CI);
      Args.push_back(VASize); // toatl count
      Args.push_back(VACounter); // current count
      Args.push_back(VAMetaData); // MD
      for(unsigned i = 1 ;i < CI->getNumOperands(); i++) {
        // Add the original argument
        Args.push_back(CI->getOperand(i));
      }
      CallInst *CINew = CallInst::Create(VAListFunctionsMap[CalledF], 
                                         Args.begin(), Args.end(), "", CI);
      CI->replaceAllUsesWith(CINew);
      CI->eraseFromParent();
    }
  }

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
    unsigned int i;
    unsigned int NumVarArgs = CI->getNumOperands() -F.arg_size() - 1;
    Value *NumArgs = ConstantInt::get(Int32Ty, NumVarArgs);
    AllocaInst *AI = new AllocaInst(Int8Ty, NumArgs, "", CI);
    // set the metadata for the varargs in AI
    unsigned int j =0;
    for(i = F.arg_size() + 1; i <CI->getNumOperands(); i++) {
      Value *Idx[2];
      Idx[0] = ConstantInt::get(Int32Ty, j++);
      // For each vararg argument, also add its type information before it
      GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(AI, 
                                                                 Idx, 
                                                                 Idx + 1, 
                                                                 "", CI);
      Constant *C = ConstantInt::get(Int8Ty, 
                                     getTypeMarker(CI->getOperand(i)->getType()));
      new StoreInst(C, GEP, CI);
    }

    for(i = 1 ;i < CI->getNumOperands(); i++) {
      // As the first vararg argument pass the number of var_arg arguments
      if(i == F.arg_size() + 1) {
        Args.push_back(ConstantInt::get(Int64Ty, NumVarArgs));
        Args.push_back(AI);
      }

      // Add the original argument
      Args.push_back(CI->getOperand(i));
    }

    // Create the new call
    CallInst *CI_New = CallInst::Create(CI->getCalledValue(), 
                                        Args.begin(), Args.end(), 
                                        "", CI);
    CI->replaceAllUsesWith(CI_New);
    CI->eraseFromParent();
  }
  return true;
}

bool TypeChecks::visitByValFunction(Module &M, Function &F) {

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
    visitInternalByValFunction(M, F);
  } else {
    // create internal clone
    Function *F_clone = CloneFunction(&F);
    F_clone->setName(F.getNameStr() + "internal");
    F.setLinkage(GlobalValue::InternalLinkage);
    F.getParent()->getFunctionList().push_back(F_clone);
    F.replaceAllUsesWith(F_clone);
    visitInternalByValFunction(M, *F_clone);
    visitExternalByValFunction(M, F);
  }
  return true;
}

bool TypeChecks::visitInternalByValFunction(Module &M, Function &F) {

  // for every byval argument
  // add an alloca, a load, and a store inst
  Instruction * InsertBefore = &(F.getEntryBlock().front());
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    if (!I->hasByValAttr())
      continue;
    if(EnableTypeSafeOpt) {
      if(TS->isTypeSafe(cast<Value>(I), &F)) {
        continue;
      }
    }
    assert(I->getType()->isPointerTy());
    const Type *ETy = (cast<PointerType>(I->getType()))->getElementType();
    AllocaInst *AI = new AllocaInst(ETy, "", InsertBefore);
    // Do this before add a load/store pair, so that those uses are not replaced.
    I->replaceAllUsesWith(AI);
    LoadInst *LI = new LoadInst(I, "", InsertBefore);
    new StoreInst(LI, AI, InsertBefore);
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

        if (RAttrs)
          AttributesVec.push_back(AttributeWithIndex::get(0, RAttrs));

        Function::arg_iterator II = F.arg_begin();
        for(unsigned j =1;j<CI->getNumOperands();j++, II++) {
          // Add the original argument
          Args.push_back(CI->getOperand(j));
          // If there are attributes on this argument, copy them to the correct 
          // position in the AttributesVec
          if(EnableTypeSafeOpt) {
            if(TS->isTypeSafe(II, CI->getParent()->getParent())) {
              if (Attributes Attrs = CallPAL.getParamAttributes(j))
                AttributesVec.push_back(AttributeWithIndex::get(Args.size(), Attrs));
              continue;
            }
          }
          //FIXME: copy the rest of the attributes.
          if(II->hasByValAttr()) 
            continue;
          if (Attributes Attrs = CallPAL.getParamAttributes(j)) {
            AttributesVec.push_back(AttributeWithIndex::get(Args.size(), Attrs));
          }
        }

        // Create the new attributes vec.
        if (FnAttrs != Attribute::None)
          AttributesVec.push_back(AttributeWithIndex::get(~0, FnAttrs));

        AttrListPtr NewCallPAL = AttrListPtr::get(AttributesVec.begin(),
                                                  AttributesVec.end());


        // Create the substitute call
        CallInst *CallI = CallInst::Create(&F,Args.begin(), Args.end(),"", CI);
        CallI->setCallingConv(CI->getCallingConv());
        CallI->setAttributes(NewCallPAL);
        CI->replaceAllUsesWith(CallI);
        CI->eraseFromParent();
      }
    }
  }

  // remove the byval attribute from the function
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    if (!I->hasByValAttr())
      continue;
    if(EnableTypeSafeOpt) {
      if(TS->isTypeSafe(cast<Value>(I), &F)) {
        continue;
      }
    }
    I->removeAttr(llvm::Attribute::ByVal);
  }
  return true;
}

bool TypeChecks::visitExternalByValFunction(Module &M, Function &F) {
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

  OS << "\nNumber of types: " << UsedTypes.size() << '\n';
}

// Initialize the shadow memory which contains the 1:1 mapping.
bool TypeChecks::initShadow(Module &M) {
  // Create the call to the runtime initialization function and place it before the store instruction.

  Constant * RuntimeCtor = M.getOrInsertFunction("tc.init", VoidTy, NULL);
  Constant * InitFn = M.getOrInsertFunction("shadowInit", VoidTy, NULL);

  //RuntimeCtor->setDoesNotThrow();
  //RuntimeCtor->setLinkage(GlobalValue::InternalLinkage);

  BasicBlock *BB = BasicBlock::Create(M.getContext(), "entry", cast<Function>(RuntimeCtor));
  CallInst::Create(InitFn, "", BB);

  Instruction *InsertPt = ReturnInst::Create(M.getContext(), BB); 

  // record all globals
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E; ++I) {
    if(I->use_empty())
      continue;
    if(I->getNameStr() == "stderr" ||
       I->getNameStr() == "stdout" ||
       I->getNameStr() == "stdin") {
      // assume initialized
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy, "", InsertPt);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      unsigned int size = TD->getTypeStoreSize(I->getType()->getElementType());
      Args.push_back(ConstantInt::get(Int64Ty, size));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", InsertPt);
      continue;
    } 
    if(!I->hasInitializer())
      continue;
    visitGlobal(M, *I, I->getInitializer(), *InsertPt, 0);
  }
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

  bool TypeChecks::visitMain(Module &M, Function &MainFunc) {
    if(MainFunc.arg_size() != 2)
      // No need to register
      return false;

    Function::arg_iterator AI = MainFunc.arg_begin();
    Value *Argc = AI;
    Value *Argv = ++AI;

    Instruction *InsertPt = MainFunc.front().begin();
    Constant * RegisterArgv = M.getOrInsertFunction("trackArgvType", VoidTy, Argc->getType(), Argv->getType(), NULL);
    std::vector<Value *> fargs;
    fargs.push_back (Argc);
    fargs.push_back (Argv);
    CallInst::Create (RegisterArgv, fargs.begin(), fargs.end(), "", InsertPt);

    return true;
  }

bool TypeChecks::visitGlobal(Module &M, GlobalVariable &GV, 
                             Constant *C, Instruction &I, unsigned offset) {

  if(EnableTypeSafeOpt) {
    if(TS->isTypeSafe(&GV)) {
      return false;
    }
  }

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
      Args.push_back(ConstantInt::get(Int8Ty, getTypeMarker(CAZ->getType())));
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
    Args.push_back(ConstantInt::get(Int8Ty, getTypeMarker(C->getType())));
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
  Function *Caller = I->getParent()->getParent();

  // Special case handling of certain libc allocation functions here.
  if (Function *F = dyn_cast<Function>(Callee)) {
    if (F->isIntrinsic()) {
      switch(F->getIntrinsicID()) {
      case Intrinsic::memcpy: 
      case Intrinsic::memmove: 
        {
          if(EnableTypeSafeOpt) {
            if(TS->isTypeSafe(I->getOperand(2), Caller)) {
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
          if(TS->isTypeSafe(I->getOperand(1), Caller)) {
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
    } else if (F->getNameStr() == std::string("__ctype_b_loc")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *>Args;
      Args.push_back(BCI);
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackctype", VoidTy, VoidPtrTy, Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI);
    } else if (F->getNameStr() == std::string("strcpy")) {
      std::vector<Value *> Args;
      Args.push_back(I->getOperand(1));
      Args.push_back(I->getOperand(2));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackStrcpyInst", VoidTy, VoidPtrTy, VoidPtrTy, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", I);
    } else if (F->getNameStr() == std::string("strncpy")) {
      std::vector<Value *>Args;
      Args.push_back(I->getOperand(1));
      Args.push_back(I->getOperand(2));
      Args.push_back(I->getOperand(3));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackStrncpyInst", VoidTy, VoidPtrTy, VoidPtrTy, I->getOperand(3)->getType(), Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", I);
    } else if(F->getNameStr() == std::string("ftime")) {
      if(EnableTypeSafeOpt) {
        if(TS->isTypeSafe(I->getOperand(1), Caller)) {
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
        if(TS->isTypeSafe(I->getOperand(2), Caller)) {
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
        if(TS->isTypeSafe(I->getOperand(1), Caller)) {
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
        if(TS->isTypeSafe(I, Caller)) {
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
        if(TS->isTypeSafe(I, Caller)) {
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
        if(TS->isTypeSafe(I->getOperand(1), Caller)) {
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
  } else {
    // indirect call site
    return visitIndirectCallSite(M, CS);
  }
  return false;
}

bool TypeChecks::visitIndirectCallSite(Module &M, CallSite CS) {
  Instruction *I = CS.getInstruction();
  I->dump();
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
  Args.push_back(ConstantInt::get(Int8Ty, getTypeMarker(PTy->getElementType())));
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
  Args.push_back(ConstantInt::get(Int8Ty, getTypeMarker(LI.getType())));
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
  Args.push_back(ConstantInt::get(Int8Ty, 
                                  getTypeMarker(SI.getOperand(0)->getType()))); // SI.getValueOperand()
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

  if(EnableTypeSafeOpt) {
    LoadInst *LI = cast<LoadInst>(SI.getOperand(0));
    if(TS->isTypeSafe(LI->getPointerOperand(), SI.getParent()->getParent())) {
      std::vector<Value *> Args;
      Args.push_back(BCI_Src);
      Args.push_back(ConstantInt::get(Int64Ty, TD->getTypeStoreSize(SI.getOperand(0)->getType())));
      Args.push_back(ConstantInt::get(Int32Ty, tagCounter++));
      Constant *F = M.getOrInsertFunction("trackInitInst", VoidTy, VoidPtrTy, Int64Ty, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", &SI);
    }
  }

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
