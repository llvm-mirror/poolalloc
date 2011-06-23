//===------------- TypeChecks.h - Insert runtime type checks --------------===//
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

#ifndef TYPE_CHECKS_H
#define TYPE_CHECKS_H

#include "dsa/AddressTakenAnalysis.h"

#include "llvm/Instructions.h"
#include "llvm/Function.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/Dominators.h"

#include <map>
#include <list>
#include <set>

namespace llvm {

class Type;
class Value;

class TypeChecks : public ModulePass {
private:
  std::map<const Type *, unsigned int> UsedTypes;
  std::map<Function *, Function *> IndFunctionsMap;
  std::list<Function *> VAArgFunctions;
  std::list<Function *> ByValFunctions;
  std::list<Function *> AddressTakenFunctions;
  std::set<Instruction*> IndCalls;

  // Analysis from other passes.
  TargetData *TD;
  AddressTakenAnalysis* addrAnalysis;
  
  unsigned int getTypeMarker(const Type*);
  unsigned int getTypeMarker(Value*);
  Constant *getTypeMarkerConstant(Value * V);
  Constant *getTypeMarkerConstant(const Type* T);
  unsigned int getSize(const Type*);
  Constant* getSizeConstant(const Type*);
  
  bool initShadow(Module &M);
  void addTypeMap(Module &M) ;
  void optimizeChecks(Module &M);
  
  bool visitMain(Module &M, Function &F); 

  bool visitCallInst(Module &M, CallInst &CI);
  bool visitInvokeInst(Module &M, InvokeInst &CI);
  bool visitCallSite(Module &M, CallSite CS);
  bool visitIndirectCallSite(Module &M, Instruction *I);

  bool visitLoadInst(Module &M, LoadInst &LI);
  bool visitStoreInst(Module &M, StoreInst &SI);
  bool visitAllocaInst(Module &M, AllocaInst &AI);
  bool visitVAArgInst(Module &M, VAArgInst &VI);
  
  bool visitUses(Instruction *I, AllocaInst *AI, CastInst *BCI);

  bool visitGlobal(Module &M, GlobalVariable &GV, 
                   Constant *C, Instruction &I, SmallVector<Value*,8>);
  
  bool visitInternalByValFunction(Module &M, Function &F); 
  bool visitExternalByValFunction(Module &M, Function &F); 
  bool visitByValFunction(Module &M, Function &F); 

  bool visitAddressTakenFunction(Module &M, Function &F);


  bool visitVarArgFunction(Module &M, Function &F); 
  bool visitInternalVarArgFunction(Module &M, Function &F); 

  bool visitInputFunctionValue(Module &M, Value *V, Instruction *CI);

public:
  static char ID;
  TypeChecks() : ModulePass(&ID) {}
  virtual bool runOnModule(Module &M);
  virtual void print(raw_ostream &OS, const Module *M) const;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetData>();
    AU.addRequired<DominatorTree>();
    AU.addRequired<AddressTakenAnalysis>();
  }

  // Return the map containing all of the types used in the module.
  const std::map<const Type *, unsigned int> &getTypes() const {
    return UsedTypes;
  }
};

} // End llvm namespace

#endif
