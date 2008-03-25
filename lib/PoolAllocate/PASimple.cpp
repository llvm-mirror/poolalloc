//===-- PASimple.cpp - Simple Pool Allocation Pass ------------------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// A minimal poolallocator that assignes all allocation to one common
// global pool.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "poolalloc"

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "dsa/CallTargets.h"
#include "poolalloc/PoolAllocate.h"
#include "Heuristic.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/ParameterAttributes.h"
#include "llvm/Support/CFG.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Timer.h"

#include <iostream>

using namespace llvm;
using namespace PA;

namespace {
  class PoolAllocateSimple : public PoolAllocate {
    GlobalValue* TheGlobalPool;
  public:
    static char ID;
    PoolAllocateSimple() : PoolAllocate(false, (intptr_t)&ID) {}
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnModule(Module &M);
    GlobalVariable *CreateGlobalPool(unsigned RecSize, unsigned Align,
                                     Instruction *IPHint, Module& M);
    void ProcessFunctionBodySimple(Function& F);

  };

  char PoolAllocateSimple::ID = 0;

  RegisterPass<PoolAllocateSimple>
  X("poolalloc-simple", "Pool allocate everything into a single global pool");

}

void PoolAllocateSimple::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetData>();
}

bool PoolAllocateSimple::runOnModule(Module &M) {
  if (M.begin() == M.end()) return false;

  // Add the pool* prototypes to the module
  AddPoolPrototypes(&M);

  // Get the main function to insert the poolinit calls.
  Function *MainFunc = M.getFunction("main");
  if (MainFunc == 0 || MainFunc->isDeclaration()) {
    std::cerr << "Cannot pool allocate this program: it has global "
              << "pools but no 'main' function yet!\n";
    return true;
  }

  TheGlobalPool = CreateGlobalPool(1, 1, MainFunc->getEntryBlock().begin(), M);


  // Now that all call targets are available, rewrite the function bodies of the
  // clones.
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration())
      ProcessFunctionBodySimple(*I);
  
  return true;
}

void PoolAllocateSimple::ProcessFunctionBodySimple(Function& F) {
  std::vector<Instruction*> toDelete;
  std::vector<ReturnInst*> Returns;
  std::vector<Instruction*> ToFree;
  for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i)
    for (BasicBlock::iterator ii = i->begin(), ee = i->end(); ii != ee; ++ii) {
      if (isa<MallocInst>(ii)) {
        toDelete.push_back(ii);
        //Fixme: fixup size
        Value* args[] = {TheGlobalPool, ii->getOperand(0)};
        Instruction* x = new CallInst(PoolAlloc, &args[0], &args[2], "", ii);
        ii->replaceAllUsesWith(CastInst::createPointerCast(x, ii->getType(), "", ii));
      } else if (isa<AllocaInst>(ii)) {
        toDelete.push_back(ii);
        //Fixme: fixup size
        Value* args[] = {TheGlobalPool, ii->getOperand(0)};
        Instruction* x = new CallInst(PoolAlloc, &args[0], &args[2], "", ii);
        ToFree.push_back(x);
        ii->replaceAllUsesWith(CastInst::createPointerCast(x, ii->getType(), "", ii));
      } else if (isa<FreeInst>(ii)) {
        toDelete.push_back(ii);
        Value* args[] = {TheGlobalPool, ii->getOperand(0)};
        new CallInst(PoolFree, &args[0], &args[2], "", ii);
      } else if (isa<ReturnInst>(ii)) {
        Returns.push_back(cast<ReturnInst>(ii));
      }
    }
  
  //add frees at each return for the allocas
  for (std::vector<ReturnInst*>::iterator i = Returns.begin(), e = Returns.end();
       i != e; ++i)
    for (std::vector<Instruction*>::iterator ii = ToFree.begin(), ee = ToFree.end();
         ii != ee; ++ii) {
        Value* args[] = {TheGlobalPool, *ii};
        new CallInst(PoolFree, &args[0], &args[2], "", *i);
    }
  
  //delete malloc and alloca insts
  for (unsigned x = 0; x < toDelete.size(); ++x)
    toDelete[x]->eraseFromParent();
}

/// CreateGlobalPool - Create a global pool descriptor object, and insert a
/// poolinit for it into main.  IPHint is an instruction that we should insert
/// the poolinit before if not null.
GlobalVariable *PoolAllocateSimple::CreateGlobalPool(unsigned RecSize, unsigned Align,
                                                     Instruction *IPHint, Module& M) {
  GlobalVariable *GV =
    new GlobalVariable(getPoolType(), false, GlobalValue::InternalLinkage, 
                       Constant::getNullValue(getPoolType()), "GlobalPool",
                       &M);

  Function *MainFunc = M.getFunction("main");
  assert(MainFunc && "No main in program??");

  BasicBlock::iterator InsertPt;
  if (IPHint)
    InsertPt = IPHint;
  else {
    InsertPt = MainFunc->getEntryBlock().begin();
    while (isa<AllocaInst>(InsertPt)) ++InsertPt;
  }

  Value *ElSize = ConstantInt::get(Type::Int32Ty, RecSize);
  Value *AlignV = ConstantInt::get(Type::Int32Ty, Align);
  Value* Opts[3] = {GV, ElSize, AlignV};
  new CallInst(PoolInit, Opts, Opts + 3, "", InsertPt);
  return GV;
}

