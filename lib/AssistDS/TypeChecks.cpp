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
static Constant *trackGlobal;
static Constant *trackStringInput;
static Constant *trackArray;
static Constant *trackInitInst;
static Constant *trackUnInitInst;
static Constant *trackStoreInst;
static Constant *trackLoadInst;
static Constant *copyTypeInfo;
static Constant *RegisterArgv;
static Constant *compareTypeAndNumber;

unsigned int TypeChecks::getTypeMarker(const Type * Ty) {
  if(DisablePointerTypeChecks) {
    if(Ty->isPointerTy()) {
      Ty = VoidPtrTy;
    }
  }
  if(UsedTypes.find(Ty) == UsedTypes.end())
    UsedTypes[Ty] = UsedTypes.size();

  return UsedTypes[Ty];
}

unsigned int TypeChecks::getTypeMarker(Value *V) {
  return getTypeMarker(V->getType());
}

unsigned int TypeChecks::getSize(const Type *Ty) {
  return TD->getTypeStoreSize(Ty);
}

Constant *TypeChecks::getSizeConstant(const Type *Ty) {
  return (ConstantInt::get(Int64Ty, getSize(Ty)));
}

static Constant *getTagCounter() {
  return ConstantInt::get(Int32Ty, tagCounter++);
}

Constant *TypeChecks::getTypeMarkerConstant(Value * V) {
  return ConstantInt::get(Int8Ty, getTypeMarker(V));
}

Constant *TypeChecks::getTypeMarkerConstant(const Type *T) {
  return ConstantInt::get(Int8Ty, getTypeMarker(T));
}

bool TypeChecks::runOnModule(Module &M) {
  bool modified = false; // Flags whether we modified the module.

  TD = &getAnalysis<TargetData>();
  TA = &getAnalysis<TypeAnalysis>();
  addrAnalysis = &getAnalysis<AddressTakenAnalysis>();

  // Create the necessary prototypes
  VoidTy = IntegerType::getVoidTy(M.getContext());
  Int8Ty = IntegerType::getInt8Ty(M.getContext());
  Int32Ty = IntegerType::getInt32Ty(M.getContext());
  Int64Ty = IntegerType::getInt64Ty(M.getContext());
  VoidPtrTy = PointerType::getUnqual(Int8Ty);

  RegisterArgv = M.getOrInsertFunction("trackArgvType",
                                       VoidTy,
                                       Int32Ty, /*argc */
                                       VoidPtrTy->getPointerTo(),/*argv*/
                                       NULL);
  trackGlobal = M.getOrInsertFunction("trackGlobal",
                                      VoidTy,
                                      VoidPtrTy,/*ptr*/
                                      Int8Ty,/*type*/
                                      Int64Ty,/*size*/
                                      Int32Ty,/*tag*/
                                      NULL);
  trackArray = M.getOrInsertFunction("trackArray",
                                     VoidTy,
                                     VoidPtrTy,/*ptr*/
                                     Int64Ty,/*size*/
                                     Int64Ty,/*count*/
                                     Int32Ty,/*tag*/
                                     NULL);
  trackInitInst = M.getOrInsertFunction("trackInitInst",
                                        VoidTy,
                                        VoidPtrTy,/*ptr*/
                                        Int64Ty,/*size*/
                                        Int32Ty,/*tag*/
                                        NULL);
  trackUnInitInst = M.getOrInsertFunction("trackUnInitInst",
                                          VoidTy,
                                          VoidPtrTy,/*ptr*/
                                          Int64Ty,/*size*/
                                          Int32Ty,/*tag*/
                                          NULL);
  trackStoreInst = M.getOrInsertFunction("trackStoreInst",
                                         VoidTy,
                                         VoidPtrTy,/*ptr*/
                                         Int8Ty,/*type*/
                                         Int64Ty,/*size*/
                                         Int32Ty,/*tag*/
                                         NULL);
  trackLoadInst = M.getOrInsertFunction("trackLoadInst",
                                        VoidTy,
                                        VoidPtrTy,/*ptr*/
                                        Int8Ty,/*type*/
                                        Int64Ty,/*size*/
                                        Int32Ty,/*tag*/
                                        NULL);
  copyTypeInfo = M.getOrInsertFunction("copyTypeInfo",
                                       VoidTy,
                                       VoidPtrTy,/*dest ptr*/
                                       VoidPtrTy,/*src ptr*/
                                       Int64Ty,/*size*/
                                       Int32Ty,/*tag*/
                                       NULL);
  compareTypeAndNumber = M.getOrInsertFunction("compareTypeAndNumber",
                                               VoidTy,
                                               Int64Ty,/*Total Args*/
                                               Int64Ty,/*Arg accessed*/
                                               Int8Ty,/*type*/
                                               VoidPtrTy,/*metadata ptr*/
                                               Int32Ty,/*tag*/
                                               NULL);
  trackStringInput = M.getOrInsertFunction("trackStringInput",
                                           VoidTy,
                                           VoidPtrTy,
                                           Int32Ty,
                                           NULL);

  UsedTypes.clear(); // Reset if run multiple times.
  VAListFunctions.clear();
  VAArgFunctions.clear();
  ByValFunctions.clear();
  AddressTakenFunctions.clear();

  // Only works for whole program analysis
  Function *MainF = M.getFunction("main");
  if (MainF == 0 || MainF->isDeclaration()) {
    assert(0 && "No main function found");
    return false;
  }

  // Insert the shadow initialization function.
  modified |= initShadow(M);

  // Record argv
  modified |= visitMain(M, *MainF);

  // Recognize special cases
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    Function &F = *MI;
    if(F.isDeclaration())
      continue;

    std::string name = F.getName();
    if (strncmp(name.c_str(), "tc.", 3) == 0) continue;
    if (strncmp(name.c_str(), "main", 4) == 0) continue;

    // Iterate and find all byval functions
    bool hasByValArg = false;
    for (Function::arg_iterator I = F.arg_begin(); I != F.arg_end(); ++I) {
      if (I->hasByValAttr()) {
        hasByValArg = true;
        break;
      }
    }
    if(hasByValArg) {
      ByValFunctions.push_back(&F);
    }

    // Iterate and find all address taken functions
    if(addrAnalysis->hasAddressTaken(&F)) {
      AddressTakenFunctions.push_back(&F);
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
    for (Function::arg_iterator I = F.arg_begin(); I != F.arg_end(); ++I) {
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
  
  // Modify all byval functions
  while(!ByValFunctions.empty()) {
    Function *F = ByValFunctions.back();
    ByValFunctions.pop_back();
    modified |= visitByValFunction(M, *F);
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
  std::map<Function *, Function *>::iterator FI = VAListFunctionsMap.begin(), 
                                             FE = VAListFunctionsMap.end();
  for(; FI != FE; FI++) {
    visitVAListCall(FI->second);
  }
  while(!VAArgFunctions.empty()) {
    Function *F = VAArgFunctions.back();
    VAArgFunctions.pop_back();
    assert(F->isVarArg());
    modified |= visitVarArgFunction(M, *F);
  }

  while(!AddressTakenFunctions.empty()) {
    Function *F = AddressTakenFunctions.back();
    AddressTakenFunctions.pop_back();
    if(F->isVarArg())
      continue;
    visitAddressTakenFunction(M, *F);
  }
  
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    Function &F = *MI;
    if(F.isDeclaration())
      continue;

    // Loop over all of the instructions in the function, 
    // adding their return type as well as the types of their operands.
    for (inst_iterator II = inst_begin(F), IE = inst_end(F); II != IE;++II) {
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

  // visit all the uses of the address taken functions and modify if
  // visit all the indirect call sites
  std::set<Instruction*>::iterator II = IndCalls.begin();
  for(; II != IndCalls.end();) {
    Instruction *I = *II++;
    modified |= visitIndirectCallSite(M,I);
  }

  FI = IndFunctionsMap.begin(), FE = IndFunctionsMap.end();
  for(;FI!=FE;++FI) {
    Function *F = FI->first;
    std::vector<User *> toReplace;
    for(Function::use_iterator User = F->use_begin();
        User != F->use_end();++User) {
      toReplace.push_back(*User);
    }
    Constant *CNew = ConstantExpr::getBitCast(FI->second, F->getType());
    while (toReplace.size()) {
      llvm::User * user = toReplace.back();
      toReplace.pop_back(); 
      if(Constant *C = dyn_cast<Constant>(user)) {
        if(!isa<GlobalValue>(C)) {
          bool changeUse = true;
          for(Value::use_iterator II = user->use_begin();
              II != user->use_end(); II++) {
            if(CallInst *CI = dyn_cast<CallInst>(II))
              if(CI->getCalledFunction()->isDeclaration())
                changeUse = false;
          }
          if(!changeUse)
            continue;
          std::vector<Use *> ReplaceWorklist;
          for (User::op_iterator use = user->op_begin();
               use != user->op_end();
               ++use) {
            if (use->get() == F) {
              ReplaceWorklist.push_back (use);
            }
          }

          //
          // Do replacements in the worklist.
          //
          for (unsigned index = 0; index < ReplaceWorklist.size(); ++index)
            C->replaceUsesOfWithOnConstant(F, CNew, ReplaceWorklist[index]);
          continue;
        }
      }
      user->replaceUsesOfWith(F, CNew);
    }
  }


  // add a global that contains the mapping from metadata to strings
  addTypeMap(M);

  // Update stats
  numTypes += UsedTypes.size();

  return modified;
}


// add a global that has the metadata -> typeString mapping
void TypeChecks::addTypeMap(Module &M) {

  // Declare the type of the global
  ArrayType*  AType = ArrayType::get(VoidPtrTy, UsedTypes.size() + 1);
  std::vector<Constant *> Values;
  Values.reserve(UsedTypes.size() + 1);

  // Declare indices useful for creating a GEP
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
  Constant *C = ConstantExpr::getGetElementPtr(GV, 
                                               &Indices[0], 
                                               Indices.size());
  Values[0] = C;

  // For each used type, create a new entry. 
  // Also add these strings to the Values list
  std::map<const Type*, unsigned int >::iterator TI = UsedTypes.begin(),
                                                 TE = UsedTypes.end(); 
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
    Constant *C = ConstantExpr::getGetElementPtr(GV, 
                                                 &Indices[0], 
                                                 Indices.size());
    Values[TI->second]= C;
  }

  new GlobalVariable(M, 
                     AType,
                     true,
                     GlobalValue::ExternalLinkage,
                     ConstantArray::get(AType,&Values[0],UsedTypes.size()+1),
                     "typeNames"
                    );
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
      Args.push_back(NII++); // total count
      Args.push_back(NII++); // current count
      Args.push_back(NII); // MD
      for(unsigned i = 1 ;i < CI->getNumOperands(); i++) {
        // Add the original argument
        Args.push_back(CI->getOperand(i));
      }
      CallInst *CINew = CallInst::Create(VAListFunctionsMap[CalledF], 
                                         Args.begin(), 
                                         Args.end(),
                                         "", CI);
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
  for (Function::arg_iterator I = F_orig.arg_begin(); 
       I != F_orig.arg_end(); ++I) {
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
  for (Function::arg_iterator I = F_orig.arg_begin();
       I != F_orig.arg_end(); ++I) {
    TP.push_back(I->getType());
  }
  // 2. Create the new function prototype
  const FunctionType *NewFTy = FunctionType::get(F_orig.getReturnType(),
                                                 TP,/*argument types*/
                                                 false);/*vararg*/
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
  for (Function::arg_iterator II = F_orig.arg_begin(); 
       NI != F->arg_end(); ++II, ++NI) {
    // Each new argument maps to the argument in the old function
    // For these arguments, also copy over the attributes
    ValueMap[II] = NI;
    NI->setName(II->getName());
    NI->addAttr(F_orig.getAttributes()
                .getParamAttributes(II->getArgNo() + 1));
  }

  // 4. Copy over the attributes for the function.
  F->setAttributes(F->getAttributes()
                   .addAttr(0, F_orig.getAttributes().getRetAttributes()));
  F->setAttributes(F->getAttributes()
                   .addAttr(~0, F_orig.getAttributes().getFnAttributes()));

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
      Args.push_back(getTypeMarkerConstant(VI));
      Args.push_back(VAMetaData);
      Args.push_back(getTagCounter());
      CallInst::Create(compareTypeAndNumber,
                       Args.begin(),
                       Args.end(),
                       "", VI);
    }
  }

  return true;
}

bool TypeChecks::visitAddressTakenFunction(Module &M, Function &F) {
  // Clone function
  // 1. Create the new argument types vector
  std::vector<const Type*> TP;
  TP.push_back(Int64Ty); // for count
  TP.push_back(VoidPtrTy); // for MD
  for(Function::arg_iterator I = F.arg_begin(); I !=F.arg_end(); ++I) {
    TP.push_back(I->getType());
  }

  // 2. Create the new function prototype
  const FunctionType *NewFTy = FunctionType::get(F.getReturnType(),
                                                 TP,
                                                 false);
  Function *NewF = Function::Create(NewFTy,
                                    GlobalValue::InternalLinkage,
                                    F.getNameStr() + ".mod",
                                    &M);

  // 3. Set the mapping for the args
  Function::arg_iterator NI = NewF->arg_begin();
  DenseMap<const Value *, Value*> ValueMap;
  NI->setName("TotalCount");
  NI++;
  NI->setName("MD");
  NI++;
  for(Function::arg_iterator II = F.arg_begin(); 
      NI!=NewF->arg_end(); ++II, ++NI) {
    // Each new argument maps to the argument in the old function
    // For each of these also copy attributes
    ValueMap[II] = NI;
    NI->setName(II->getName());
    NI->addAttr(F.getAttributes().getParamAttributes(II->getArgNo()+1));
  }

  // 4. Copy over attributes for the function
  NewF->setAttributes(NewF->getAttributes()
                      .addAttr(0, F.getAttributes().getRetAttributes()));
  NewF->setAttributes(NewF->getAttributes()
                      .addAttr(~0, F.getAttributes().getFnAttributes()));

  // 5. Perform the cloning
  SmallVector<ReturnInst*, 100>Returns;
  CloneFunctionInto(NewF, &F, ValueMap, Returns);
  IndFunctionsMap[&F] = NewF;
  
  // Find all uses of the function
  for(Value::use_iterator ui = F.use_begin(), ue = F.use_end();
      ui != ue;)  {
    // Check for call sites
    CallInst *CI = dyn_cast<CallInst>(ui++);
    if(!CI)
      continue;
    std::vector<Value *> Args;
    unsigned int i;
    unsigned int NumArgs = CI->getNumOperands() - 1;
    Value *NumArgsVal = ConstantInt::get(Int32Ty, NumArgs);
    AllocaInst *AI = new AllocaInst(Int8Ty, NumArgsVal, "", CI);
    // set the metadata for the varargs in AI
    for(i = 1; i <CI->getNumOperands(); i++) {
      Value *Idx[2];
      Idx[0] = ConstantInt::get(Int32Ty, i - 1 );
      // For each vararg argument, also add its type information
      GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(AI, 
                                                                 Idx, 
                                                                 Idx + 1, 
                                                                 "", CI);
      Constant *C = getTypeMarkerConstant(CI->getOperand(i));
      new StoreInst(C, GEP, CI);
    }

    // As the first argument pass the number of var_arg arguments
    Args.push_back(ConstantInt::get(Int64Ty, NumArgs));
    Args.push_back(AI);
    for(i = 1 ;i < CI->getNumOperands(); i++) {
      // Add the original argument
      Args.push_back(CI->getOperand(i));
    }

    // Create the new call
    CallInst *CI_New = CallInst::Create(NewF, 
                                        Args.begin(), Args.end(), 
                                        "", CI);
    CI->replaceAllUsesWith(CI_New);
    CI->eraseFromParent();
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
}

// each vararg function is modified so that the first
// argument is the number of arguments passed in,
// and the second is a pointer to a metadata array, 
// containing type information for each of the arguments

// These are read and stored at the beginning of the function.

// We keep a counter for the number of arguments accessed
// from the va_list(Counter). It is incremented and 
// checked on every va_arg access. It is initialized to zero.
// It is also reset to zero on a call to va_start.

// Similiarly we check type on every va_arg access.

// Aside from this, this function also transforms all
// callsites of the var_arg function.

bool TypeChecks::visitInternalVarArgFunction(Module &M, Function &F) {

  // Clone function
  // 1. Create the new argument types vector
  std::vector<const Type*> TP;
  TP.push_back(Int64Ty); // for count
  TP.push_back(VoidPtrTy); // for MD
  for(Function::arg_iterator I = F.arg_begin(); I !=F.arg_end(); ++I) {
    TP.push_back(I->getType());
  }

  // 2. Create the new function prototype
  const FunctionType *NewFTy = FunctionType::get(F.getReturnType(),
                                                 TP,
                                                 true);
  Function *NewF = Function::Create(NewFTy,
                                    GlobalValue::InternalLinkage,
                                    F.getNameStr() + ".mod",
                                    &M);

  // 3. Set the mapping for the args
  Function::arg_iterator NI = NewF->arg_begin();
  DenseMap<const Value *, Value*> ValueMap;
  NI->setName("TotalCount");
  NI++;
  NI->setName("MD");
  NI++;
  for(Function::arg_iterator II = F.arg_begin(); 
      NI!=NewF->arg_end(); ++II, ++NI) {
    // Each new argument maps to the argument in the old function
    // For each of these also copy attributes
    ValueMap[II] = NI;
    NI->setName(II->getName());
    NI->addAttr(F.getAttributes().getParamAttributes(II->getArgNo()+1));
  }

  // 4. Copy over attributes for the function
  NewF->setAttributes(NewF->getAttributes()
                      .addAttr(0, F.getAttributes().getRetAttributes()));
  NewF->setAttributes(NewF->getAttributes()
                      .addAttr(~0, F.getAttributes().getFnAttributes()));

  // 5. Perform the cloning
  SmallVector<ReturnInst*, 100>Returns;
  CloneFunctionInto(NewF, &F, ValueMap, Returns);


  // Store the information
  inst_iterator InsPt = inst_begin(NewF);
  Function::arg_iterator NII = NewF->arg_begin();
  AllocaInst *VASizeLoc = new AllocaInst(Int64Ty, "", &*InsPt);
  // Subtract the number of initial arguments
  Constant *InitialArgs = ConstantInt::get(Int64Ty, F.arg_size());
  Instruction *NewValue = BinaryOperator::Create(BinaryOperator::Sub,
                                                 NII,
                                                 InitialArgs,
                                                 "varargs",
                                                 &*InsPt);
  new StoreInst(NewValue, VASizeLoc, &*InsPt);
  NII++;

  AllocaInst *VAMDLoc = new AllocaInst(VoidPtrTy, "", &*InsPt);
  // Increment by the number of Initial Args, so as to not read the metadata
  //for those.
  Value *Idx[2];
  Idx[0] = InitialArgs;
  // For each vararg argument, also add its type information
  GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(NII, 
                                                             Idx, 
                                                             Idx + 1, 
                                                             "", &*InsPt);
  new StoreInst(GEP, VAMDLoc, &*InsPt);
  // Add a counter variable to the function entry
  AllocaInst *Counter = new AllocaInst(Int64Ty, "",&*InsPt);
  new StoreInst(ConstantInt::get(Int64Ty, 0), Counter, &*InsPt); 

  // Modify function to add checks on every var_arg call to ensure that we
  // are not accessing more arguments than we passed in.

  // Increment the counter
  for (Function::iterator B = NewF->begin(), FE = NewF->end(); B != FE; ++B) {
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
      Args.push_back(getTypeMarkerConstant(VI));
      Args.push_back(VAMetaData);
      Args.push_back(getTagCounter());
      CallInst::Create(compareTypeAndNumber,
                       Args.begin(),
                       Args.end(),
                       "", VI);
    }
  }

  // visit all VAStarts and initialize the counter
  CallInst *VAStart = NULL;
  for (Function::iterator B = NewF->begin(), FE = NewF->end(); B != FE; ++B) {
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
      // Reinitialize the counter
      StoreInst *SI3 = new StoreInst(ConstantInt::get(Int64Ty, 0), Counter);
      SI3->insertAfter(CI);
    }
  }

  // modify calls to va list functions to pass the metadata
  for (Function::iterator B = NewF->begin(), FE = NewF->end(); B != FE; ++B) {
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
    if(isa<InvokeInst>(ui)) {
      //FIXME
      ui->dump();
    }
    CallInst *CI = dyn_cast<CallInst>(ui++);
    if(!CI)
      continue;
    std::vector<Value *> Args;
    unsigned int i;
    unsigned int NumArgs = CI->getNumOperands() - 1;
    Value *NumArgsVal = ConstantInt::get(Int32Ty, NumArgs);
    AllocaInst *AI = new AllocaInst(Int8Ty, NumArgsVal, "", CI);
    // set the metadata for the varargs in AI
    for(i = 1; i <CI->getNumOperands(); i++) {
      Value *Idx[2];
      Idx[0] = ConstantInt::get(Int32Ty, i - 1 );
      // For each vararg argument, also add its type information
      GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(AI, 
                                                                 Idx, 
                                                                 Idx + 1, 
                                                                 "", CI);
      Constant *C = getTypeMarkerConstant(CI->getOperand(i));
      new StoreInst(C, GEP, CI);
    }

    // As the first argument pass the number of var_arg arguments
    Args.push_back(ConstantInt::get(Int64Ty, NumArgs));
    Args.push_back(AI);
    for(i = 1 ;i < CI->getNumOperands(); i++) {
      // Add the original argument
      Args.push_back(CI->getOperand(i));
    }

    // Create the new call
    CallInst *CI_New = CallInst::Create(NewF, 
                                        Args.begin(), Args.end(), 
                                        "", CI);
    CI->replaceAllUsesWith(CI_New);
    CI->eraseFromParent();
  }
  IndFunctionsMap[&F] = NewF;
  return true;
}

bool TypeChecks::visitByValFunction(Module &M, Function &F) {

  // For internal functions
  //   Replace with a function with a a new function with no byval attr.
  //   Add an explicity copy in the function
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
  for (Function::arg_iterator I = F.arg_begin(); I != F.arg_end(); ++I) {
    if (!I->hasByValAttr())
      continue;
    assert(I->getType()->isPointerTy());
    const Type *ETy = (cast<PointerType>(I->getType()))->getElementType();
    AllocaInst *AI = new AllocaInst(ETy, "", InsertBefore);
    // Do this before adding the load/store pair, so that those uses are not replaced.
    I->replaceAllUsesWith(AI);
    LoadInst *LI = new LoadInst(I, "", InsertBefore);
    new StoreInst(LI, AI, InsertBefore);
  }

  // Update the call sites
  for(Value::use_iterator ui = F.use_begin(), ue = F.use_end();
      ui != ue;)  {
    if(isa<InvokeInst>(ui)) {
    //FIXME
      ui->dump();
    }
    if(!isa<CallInst>(ui)) {
      ui->dump();
    }
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
          //FIXME: copy the rest of the attributes.
          if(II->hasByValAttr()) 
            continue;
          if (Attributes Attrs = CallPAL.getParamAttributes(j)) {
            AttributesVec.push_back(AttributeWithIndex::get(j, Attrs));
          }
        }

        // Create the new attributes vec.
        if (FnAttrs != Attribute::None)
          AttributesVec.push_back(AttributeWithIndex::get(~0, FnAttrs));

        AttrListPtr NewCallPAL = AttrListPtr::get(AttributesVec.begin(),
                                                  AttributesVec.end());


        // Create the substitute call
        CallInst *CallI = CallInst::Create(&F,
                                           Args.begin(),
                                           Args.end(),
                                           "", CI);

        CallI->setCallingConv(CI->getCallingConv());
        CallI->setAttributes(NewCallPAL);
        CI->replaceAllUsesWith(CallI);
        CI->eraseFromParent();
      }
    }
  }

  // remove the byval attribute from the function
  for (Function::arg_iterator I = F.arg_begin(); I != F.arg_end(); ++I) {
    if (!I->hasByValAttr())
      continue;
    I->removeAttr(llvm::Attribute::ByVal);
  }
  return true;
}

bool TypeChecks::visitExternalByValFunction(Module &M, Function &F) {
  // A list of the byval arguments that we are setting metadata for
  typedef SmallVector<Value *, 4> RegisteredArgTy;
  RegisteredArgTy registeredArguments;
  for (Function::arg_iterator I = F.arg_begin(); I != F.arg_end(); ++I) {
    if (I->hasByValAttr()) {
      assert (isa<PointerType>(I->getType()));
      const PointerType * PT = cast<PointerType>(I->getType());
      const Type * ET = PT->getElementType();
      Value * AllocSize = ConstantInt::get(Int64Ty, TD->getTypeAllocSize(ET));
      Instruction * InsertPt = &(F.getEntryBlock().front());
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy, "", InsertPt);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(AllocSize);
      Args.push_back(getTagCounter());
      // Set the metadata for the byval argument to TOP/Initialized
      CallInst::Create(trackInitInst, Args.begin(), Args.end(), "", InsertPt);
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
      Args.push_back(getTagCounter());
      CallInst::Create(trackUnInitInst, Args.begin(), Args.end(), "", Pt);
    }
  }
  return true;
}

// Print the types found in the module. If the optional Module parameter is
// passed in, then the types are printed symbolically if possible, using the
// symbol table from the module.
void TypeChecks::print(raw_ostream &OS, const Module *M) const {
  OS << "Types in use by this module:\n";
  std::map<const Type *,unsigned int>::const_iterator I = UsedTypes.begin(), 
                                                      E = UsedTypes.end();
  for (; I != E; ++I) {
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
      Args.push_back(getSizeConstant(I->getType()->getElementType()));
      Args.push_back(getTagCounter());
      CallInst::Create(trackInitInst, Args.begin(), Args.end(), "", InsertPt);
      continue;
    } 
    if(!I->hasInitializer())
      continue;
    SmallVector<Value*,8>index;
    visitGlobal(M, *I, I->getInitializer(), *InsertPt, index);
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

bool TypeChecks::visitMain(Module &M, Function &MainFunc) {
  if(MainFunc.arg_size() != 2)
    // No need to register
    return false;

  Function::arg_iterator AI = MainFunc.arg_begin();
  Value *Argc = AI;
  Value *Argv = ++AI;

  Instruction *InsertPt = MainFunc.front().begin();
  std::vector<Value *> fargs;
  fargs.push_back (Argc);
  fargs.push_back (Argv);
  CallInst::Create (RegisterArgv, fargs.begin(), fargs.end(), "", InsertPt);

  return true;
}

bool TypeChecks::visitGlobal(Module &M, GlobalVariable &GV, 
                             Constant *C, Instruction &I, SmallVector<Value *,8> Indices) {

  if(ConstantArray *CA = dyn_cast<ConstantArray>(C)) {
    const Type * ElementType = CA->getType()->getElementType();
    // Create the type entry for the first element
    // using recursive creation till we get to the base types
    visitGlobal(M, GV, CA->getOperand(0), I, Indices);

    // Copy the type metadata for the first element
    // over for the rest of the elements.
    CastInst *BCI = BitCastInst::CreatePointerCast(&GV, VoidPtrTy, "", &I);
    std::vector<Value *> Args;
    Args.push_back(BCI);
    Args.push_back(getSizeConstant(ElementType));
    Args.push_back(ConstantInt::get(Int64Ty, CA->getNumOperands()));
    Args.push_back(getTagCounter());
    CallInst::Create(trackArray, Args.begin(), Args.end(), "", &I);
  }
  else if(ConstantStruct *CS = dyn_cast<ConstantStruct>(C)) {
    // Create metadata for each field of the struct
    // at the correct offset.
    const StructLayout *SL = TD->getStructLayout(cast<StructType>(CS->getType()));
    for (unsigned i = 0, e = CS->getNumOperands(); i != e; ++i) {
      if (SL->getElementOffset(i) < SL->getSizeInBytes()) {
        Constant * ConstElement = cast<Constant>(CS->getOperand(i));
        Indices.push_back(ConstantInt::get(Int32Ty, i));
        visitGlobal(M, GV, ConstElement, I, Indices);
        Indices.pop_back();
      }
    }
  } else if(ConstantAggregateZero *CAZ = dyn_cast<ConstantAggregateZero>(C)) {
    // Similiar to having an initializer with all values NULL
    // Must set metadata, similiar to the previous 2 cases.
    const Type *Ty = CAZ->getType();
    if(const ArrayType * ATy = dyn_cast<ArrayType>(Ty)) {
      const Type * ElementType = ATy->getElementType();
      visitGlobal(M, GV, Constant::getNullValue(ElementType), I, Indices);
      CastInst *BCI = BitCastInst::CreatePointerCast(&GV, VoidPtrTy, "", &I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(getSizeConstant(ElementType));
      Args.push_back(ConstantInt::get(Int64Ty, ATy->getNumElements()));
      Args.push_back(getTagCounter());
      CallInst::Create(trackArray, Args.begin(), Args.end(), "", &I);
    } else if(const StructType *STy = dyn_cast<StructType>(Ty)) {
      const StructLayout *SL = TD->getStructLayout(STy);
      for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
        if (SL->getElementOffset(i) < SL->getSizeInBytes()) {
          Indices.push_back(ConstantInt::get(Int32Ty, i));
          visitGlobal(M, GV, Constant::getNullValue(STy->getElementType(i)), I, Indices);
          Indices.pop_back();
        }
      }
    } else {
      // Zeroinitializer of a primitive type
      GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(&GV, Indices.begin(),
                                                                 Indices.end(),"", &I) ;

      CastInst *BCI = BitCastInst::CreatePointerCast(GEP, VoidPtrTy, "", &I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(getTypeMarkerConstant(CAZ));
      Args.push_back(getSizeConstant(CAZ->getType()));
      Args.push_back(getTagCounter());
      CallInst::Create(trackGlobal, Args.begin(), Args.end(), "", &I);
    }
  }
  else {
    // Primitive type value
    GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(&GV, Indices.begin(),
                                                               Indices.end(),"", &I) ;

    CastInst *BCI = BitCastInst::CreatePointerCast(GEP, VoidPtrTy, "", &I);
    std::vector<Value *> Args;
    Args.push_back(BCI);
    Args.push_back(getTypeMarkerConstant(C));
    Args.push_back(getSizeConstant(C->getType()));
    Args.push_back(getTagCounter());
    CallInst::Create(trackGlobal, Args.begin(), Args.end(), "", &I);
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

  // Setting metadata to be 0(BOTTOM/Uninitialized)

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(Size);
  Args.push_back(getTagCounter());
  CallInst *CI = CallInst::Create(trackUnInitInst, Args.begin(), Args.end());
  CI->insertAfter(CI_Init);
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
          CastInst *BCI_Src = BitCastInst::CreatePointerCast(I->getOperand(2), VoidPtrTy, "", I);
          CastInst *BCI_Dest = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
          std::vector<Value *> Args;
          Args.push_back(BCI_Dest);
          Args.push_back(BCI_Src);
          CastInst *Size = CastInst::CreateIntegerCast(I->getOperand(3), Int64Ty, false, "", I);
          Args.push_back(Size);
          Args.push_back(getTagCounter());
          CallInst::Create(copyTypeInfo, Args.begin(), Args.end(), "", I);
          return true;
        }

      case Intrinsic::memset:
        CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
        std::vector<Value *> Args;
        Args.push_back(BCI);
        CastInst *Size = CastInst::CreateIntegerCast(I->getOperand(3), Int64Ty, false, "", I);
        Args.push_back(Size);
        Args.push_back(getTagCounter());
        CallInst::Create(trackInitInst, Args.begin(), Args.end(), "", I);
        return true;
      }
    } else if (F->getNameStr() == std::string("__ctype_b_loc")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *>Args;
      Args.push_back(BCI);
      Args.push_back(getTagCounter());
      Constant *F = M.getOrInsertFunction("trackctype", VoidTy, VoidPtrTy, Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI);
    } else if (F->getNameStr() == std::string("__ctype_toupper_loc")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *>Args;
      Args.push_back(BCI);
      Args.push_back(getTagCounter());
      Constant *F = M.getOrInsertFunction("trackctype_32", VoidTy, VoidPtrTy, Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI);
    } else if (F->getNameStr() == std::string("__ctype_tolower_loc")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *>Args;
      Args.push_back(BCI);
      Args.push_back(getTagCounter());
      Constant *F = M.getOrInsertFunction("trackctype_32", VoidTy, VoidPtrTy, Int32Ty, NULL);
      CallInst *CI = CallInst::Create(F, Args.begin(), Args.end());
      CI->insertAfter(BCI);
    } else if (F->getNameStr() == std::string("strcpy")) {
      std::vector<Value *> Args;
      Args.push_back(I->getOperand(1));
      Args.push_back(I->getOperand(2));
      Args.push_back(getTagCounter());
      Constant *F = M.getOrInsertFunction("trackStrcpyInst", VoidTy, VoidPtrTy, VoidPtrTy, Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", I);
    } else if (F->getNameStr() == std::string("strncpy")) {
      std::vector<Value *>Args;
      Args.push_back(I->getOperand(1));
      Args.push_back(I->getOperand(2));
      Args.push_back(I->getOperand(3));
      Args.push_back(getTagCounter());
      Constant *F = M.getOrInsertFunction("trackStrncpyInst", VoidTy, VoidPtrTy, VoidPtrTy, I->getOperand(3)->getType(), Int32Ty, NULL);
      CallInst::Create(F, Args.begin(), Args.end(), "", I);
    } else if(F->getNameStr() == std::string("ftime")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
      const PointerType *PTy = cast<PointerType>(I->getOperand(1)->getType());
      const Type * ElementType = PTy->getElementType();
      std::vector<Value *> Args;
      Args.push_back(BCI);
      Args.push_back(getSizeConstant(ElementType));
      Args.push_back(getTagCounter());
      CallInst::Create(trackInitInst, Args.begin(), Args.end(), "", I);
      return true;
    } else if(F->getNameStr() == std::string("read")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(2), VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      CastInst *Size = CastInst::CreateIntegerCast(I, Int64Ty, false);
      Size->insertAfter(I);
      Args.push_back(Size);
      Args.push_back(getTagCounter());
      CallInst *CI = CallInst::Create(trackInitInst, Args.begin(), Args.end());
      CI->insertAfter(BCI);
      return true;
    } else if(F->getNameStr() == std::string("fread")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      CastInst *Size = CastInst::CreateIntegerCast(I, Int64Ty, false);
      Size->insertAfter(I);
      Args.push_back(Size);
      Args.push_back(getTagCounter());
      CallInst *CI = CallInst::Create(trackInitInst, Args.begin(), Args.end());
      CI->insertAfter(BCI);
      return true;
    } else if(F->getNameStr() == std::string("calloc")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI->insertAfter(I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      CastInst *Size = CastInst::CreateIntegerCast(I->getOperand(2), Int64Ty, false, "", I);
      Args.push_back(Size);
      Args.push_back(getTagCounter());
      CallInst *CI = CallInst::Create(trackInitInst, Args.begin(), Args.end());
      CI->insertAfter(BCI);
      std::vector<Value *> Args1;
      Args1.push_back(BCI);
      Args1.push_back(Size);
      CastInst *Num = CastInst::CreateIntegerCast(I->getOperand(1), Int64Ty, false, "", I);
      Args1.push_back(Num);
      Args1.push_back(getTagCounter());
      CallInst *CI_Arr = CallInst::Create(trackArray, Args1.begin(), Args1.end());
      CI_Arr->insertAfter(CI);
      return true;
    } else if(F->getNameStr() ==  std::string("realloc")) {
      CastInst *BCI_Src = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy);
      CastInst *BCI_Dest = BitCastInst::CreatePointerCast(I, VoidPtrTy);
      BCI_Src->insertAfter(I);
      BCI_Dest->insertAfter(BCI_Src);
      std::vector<Value *> Args;
      Args.push_back(BCI_Dest);
      Args.push_back(BCI_Src);
      CastInst *Size = CastInst::CreateIntegerCast(I->getOperand(2), Int64Ty, false, "", I);
      Args.push_back(Size);
      Args.push_back(getTagCounter());
      CallInst *CI = CallInst::Create(copyTypeInfo, Args.begin(), Args.end());
      CI->insertAfter(BCI_Dest);
      return true;
    } else if(F->getNameStr() == std::string("fgets")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
      std::vector<Value *> Args;
      Args.push_back(BCI);
      CastInst *Size = CastInst::CreateIntegerCast(I->getOperand(2), Int64Ty, false, "", I);
      Args.push_back(Size);
      Args.push_back(getTagCounter());
      CallInst::Create(trackInitInst, Args.begin(), Args.end(), "", I);
      return true;
    } else if(F->getNameStr() == std::string("sprintf")) {
      CastInst *BCI = BitCastInst::CreatePointerCast(I->getOperand(1), VoidPtrTy, "", I);
      std::vector<Value*>Args;
      Args.push_back(BCI);
      CastInst *Size = CastInst::CreateIntegerCast(I, Int64Ty, false);
      Size->insertAfter(I);
      Constant *One = ConstantInt::get(Int64Ty, 1);
      Instruction *NewValue = BinaryOperator::Create(BinaryOperator::Add,
                                                     Size,
                                                     One);
      NewValue->insertAfter(Size);
      Args.push_back(NewValue);
      Args.push_back(getTagCounter());
      CallInst *CINew = CallInst::Create(trackInitInst, Args.begin(), Args.end());
      CINew->insertAfter(NewValue);
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
    IndCalls.insert(CS.getInstruction());
    return false;
  }
  return false;
}

bool TypeChecks::visitIndirectCallSite(Module &M, Instruction *I) {
  // add the number of arguments as the first argument

  unsigned int NumArgs = I->getNumOperands() - 1;
  Value *NumArgsVal = ConstantInt::get(Int32Ty, NumArgs);

  AllocaInst *AI = new AllocaInst(Int8Ty, NumArgsVal, "", I);
  for(unsigned int i = 1; i < I->getNumOperands(); i++) {
    Value *Idx[2];
    Idx[0] = ConstantInt::get(Int32Ty, i-1);
    GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(AI,
                                                               Idx,
                                                               Idx + 1,
                                                               "", I);
    Constant *C = getTypeMarkerConstant(I->getOperand(i));
    new StoreInst(C, GEP, I);
  }
  std::vector<Value *> Args;
  Args.push_back(ConstantInt::get(Int64Ty, NumArgs));
  Args.push_back(AI);

  for(unsigned int i = 1; i < I->getNumOperands(); i++)
    Args.push_back(I->getOperand(i));

  const Type* OrigType = I->getOperand(0)->getType();
  assert(OrigType->isPointerTy());
  const FunctionType *FOldType = cast<FunctionType>((cast<PointerType>(OrigType))->getElementType());
  std::vector<const Type*>TP;
  TP.push_back(Int64Ty);
  TP.push_back(VoidPtrTy);

  for(llvm::FunctionType::param_iterator ArgI = FOldType->param_begin(); ArgI != FOldType->param_end(); ++ArgI)
    TP.push_back(*ArgI);

  const FunctionType *FTy = FunctionType::get(FOldType->getReturnType(), TP, FOldType->isVarArg());
  CastInst *Func = CastInst::CreatePointerCast(I->getOperand(0), FTy->getPointerTo(), "", I);
  if(isa<CallInst>(I)) {
    std::vector<Value *> Args;
    Args.push_back(ConstantInt::get(Int64Ty, NumArgs));
    Args.push_back(AI);

    for(unsigned int i = 1; i < I->getNumOperands(); i++)
      Args.push_back(I->getOperand(i));
    CallInst *CI_New = CallInst::Create(Func, 
                                        Args.begin(),
                                        Args.end(), 
                                        "", I);
    I->replaceAllUsesWith(CI_New);
    I->eraseFromParent();
  } else if(InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    std::vector<Value *> Args;
    Args.push_back(ConstantInt::get(Int64Ty, NumArgs));
    Args.push_back(AI);

    for(unsigned int i = 3; i < I->getNumOperands(); i++) {
      Args.push_back(I->getOperand(i));
    }

    InvokeInst *INew = InvokeInst::Create(Func,
                                          II->getNormalDest(),
                                          II->getUnwindDest(),
                                          Args.begin(),
                                          Args.end(),
                                          "", I);
    I->replaceAllUsesWith(INew);
    I->eraseFromParent();

  }


  // add they types of the argument as the second argument
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
  Args.push_back(getTypeMarkerConstant(PTy->getElementType()));
  Args.push_back(getSizeConstant(PTy->getElementType()));
  Args.push_back(getTagCounter());

  // Create the call to the runtime check and place it before the store instruction.
  CallInst::Create(trackStoreInst, Args.begin(), Args.end(), "", CI);

  if(PTy == VoidPtrTy) {
    // TODO: This is currently a heuristic for strings. If we see a i8* in a call to 
    // input functions, treat as string, and get length using strlen.
    std::vector<Value*> Args;
    Args.push_back(BCI);
    Args.push_back(getTagCounter());
    CallInst *CINew = CallInst::Create(trackStringInput, Args.begin(), Args.end());
    CINew->insertAfter(CI);
  }

  return true;
}

// Insert runtime checks before all load instructions.
bool TypeChecks::visitLoadInst(Module &M, LoadInst &LI) {
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(LI.getPointerOperand(), VoidPtrTy, "", &LI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(getTypeMarkerConstant(&LI));
  Args.push_back(getSizeConstant(LI.getType()));
  Args.push_back(getTagCounter());

  // Create the call to the runtime check and place it before the load instruction.
  CallInst::Create(trackLoadInst, Args.begin(), Args.end(), "", &LI);
  numLoadChecks++;
  return true;
}

// Insert runtime checks before all store instructions.
bool TypeChecks::visitStoreInst(Module &M, StoreInst &SI) {
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI = BitCastInst::CreatePointerCast(SI.getPointerOperand(), VoidPtrTy, "", &SI);

  std::vector<Value *> Args;
  Args.push_back(BCI);
  Args.push_back(getTypeMarkerConstant(SI.getOperand(0))); // SI.getValueOperand()
  Args.push_back(getSizeConstant(SI.getOperand(0)->getType()));
  Args.push_back(getTagCounter());

  // Create the call to the runtime check and place it before the store instruction.
  CallInst::Create(trackStoreInst, Args.begin(), Args.end(), "", &SI);
  numStoreChecks++;

  return true;
}

// Insert runtime checks before copying store instructions.
bool TypeChecks::visitCopyingStoreInst(Module &M, StoreInst &SI, Value *SS) {
  // Cast the pointer operand to i8* for the runtime function.
  CastInst *BCI_Dest = BitCastInst::CreatePointerCast(SI.getPointerOperand(), VoidPtrTy, "", &SI);
  CastInst *BCI_Src = BitCastInst::CreatePointerCast(SS, VoidPtrTy, "", &SI);

  std::vector<Value *> Args;
  Args.push_back(BCI_Dest);
  Args.push_back(BCI_Src);
  Args.push_back(getSizeConstant(SI.getOperand(0)->getType()));
  Args.push_back(getTagCounter());

  // Create the call to the runtime check and place it before the copying store instruction.
  CallInst::Create(copyTypeInfo, Args.begin(), Args.end(), "", &SI);
  numStoreChecks++;

  return true;
}
