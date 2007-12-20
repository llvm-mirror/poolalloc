//===-- PoolOptimize.cpp - Optimize pool allocated program ----------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This simple pass optimizes a program that has been through pool allocation.
//
//===----------------------------------------------------------------------===//

#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include <set>
using namespace llvm;

namespace {
  STATISTIC (NumBumpPtr, "Number of bump pointer pools");

  struct PoolOptimize : public ModulePass {
    static char ID;
    PoolOptimize() : ModulePass((intptr_t)&ID) {}
    bool runOnModule(Module &M);
  };

  char PoolOptimize::ID = 0;
  RegisterPass<PoolOptimize>
  X("pooloptimize", "Optimize a pool allocated program");
}

static void getCallsOf(Constant *C, std::vector<CallInst*> &Calls) {
  // Get the Function out of the constant
  Function * F;
  ConstantExpr * CE;
  if (!(F=dyn_cast<Function>(C)))
    if ((CE = dyn_cast<ConstantExpr>(C)) && (CE->isCast()))
      F = dyn_cast<Function>(CE->getOperand(0));
    else
      assert (0 && "Constant is not a Function of ConstantExpr!"); 
  Calls.clear();
  for (Value::use_iterator UI = F->use_begin(), E = F->use_end(); UI != E; ++UI)
    Calls.push_back(cast<CallInst>(*UI));
}

bool PoolOptimize::runOnModule(Module &M) {
  const Type *VoidPtrTy = PointerType::getUnqual(Type::Int8Ty);
  const Type *PoolDescPtrTy = PointerType::getUnqual(ArrayType::get(VoidPtrTy, 16));


  // Get poolinit function.
  Constant *PoolInit = M.getOrInsertFunction("poolinit", Type::VoidTy,
                                             PoolDescPtrTy, Type::Int32Ty,
                                             Type::Int32Ty, NULL);

  // Get pooldestroy function.
  Constant *PoolDestroy = M.getOrInsertFunction("pooldestroy", Type::VoidTy,
                                                PoolDescPtrTy, NULL);
  
  // The poolalloc function.
  Constant *PoolAlloc = M.getOrInsertFunction("poolalloc", 
                                              VoidPtrTy, PoolDescPtrTy,
                                              Type::Int32Ty, NULL);
  
  // The poolrealloc function.
  Constant *PoolRealloc = M.getOrInsertFunction("poolrealloc",
                                                VoidPtrTy, PoolDescPtrTy,
                                                VoidPtrTy, Type::Int32Ty, NULL);
  // The poolmemalign function.
  Constant *PoolMemAlign = M.getOrInsertFunction("poolmemalign",
                                                 VoidPtrTy, PoolDescPtrTy,
                                                 Type::Int32Ty, Type::Int32Ty, 
                                                 NULL);

  // Get the poolfree function.
  Constant *PoolFree = M.getOrInsertFunction("poolfree", Type::VoidTy,
                                             PoolDescPtrTy, VoidPtrTy, NULL);  


  // Get poolinit_bp function.
  Constant *PoolInitBP = M.getOrInsertFunction("poolinit_bp", Type::VoidTy,
                                               PoolDescPtrTy, Type::Int32Ty, 
                                               NULL);
  
  // Get pooldestroy_bp function.
  Constant *PoolDestroyBP = M.getOrInsertFunction("pooldestroy_bp",Type::VoidTy,
                                                 PoolDescPtrTy, NULL);
  
  // The poolalloc_bp function.
  Constant *PoolAllocBP = M.getOrInsertFunction("poolalloc_bp", 
                                                VoidPtrTy, PoolDescPtrTy,
                                                Type::Int32Ty, NULL);

  Constant *Realloc = M.getOrInsertFunction("realloc",
                                            VoidPtrTy, VoidPtrTy, Type::Int32Ty,
                                            NULL);
  Constant *MemAlign = M.getOrInsertFunction("memalign",
                                             VoidPtrTy, Type::Int32Ty,
                                             Type::Int32Ty, NULL);


  // Optimize poolreallocs
  std::vector<CallInst*> Calls;
  getCallsOf(PoolRealloc, Calls);
  for (unsigned i = 0, e = Calls.size(); i != e; ++i) {
    CallInst *CI = Calls[i];
    // poolrealloc(PD, null, X) -> poolalloc(PD, X)
    if (isa<ConstantPointerNull>(CI->getOperand(2))) {
      Value* Opts[2] = {CI->getOperand(1), CI->getOperand(3)};
      Value *New = new CallInst(PoolAlloc, Opts, Opts + 2,
                                CI->getName(), CI);
      CI->replaceAllUsesWith(New);
      CI->eraseFromParent();
    } else if (isa<Constant>(CI->getOperand(3)) && 
               cast<Constant>(CI->getOperand(3))->isNullValue()) {
      // poolrealloc(PD, X, 0) -> poolfree(PD, X)
      Value* Opts[2] = {CI->getOperand(1), CI->getOperand(2)};
      new CallInst(PoolFree, Opts, Opts + 2, "", CI);
      CI->replaceAllUsesWith(Constant::getNullValue(CI->getType()));
      CI->eraseFromParent();
    } else if (isa<ConstantPointerNull>(CI->getOperand(1))) {
      // poolrealloc(null, X, Y) -> realloc(X, Y)
      Value* Opts[2] = {CI->getOperand(2), CI->getOperand(3)};
      Value *New = new CallInst(Realloc, Opts, Opts + 2,
                                CI->getName(), CI);
      CI->replaceAllUsesWith(New);
      CI->eraseFromParent();
    }
  }

  // Optimize poolallocs
  getCallsOf(PoolAlloc, Calls);
  for (unsigned i = 0, e = Calls.size(); i != e; ++i) {
    CallInst *CI = Calls[i];
    // poolalloc(null, X) -> malloc(X)
    if (isa<Constant>(CI->getOperand(1)) && 
        cast<Constant>(CI->getOperand(1))->isNullValue()) {
      Value *New = new MallocInst(Type::Int8Ty, CI->getOperand(2),
                                  CI->getName(), CI);
      CI->replaceAllUsesWith(New);
      CI->eraseFromParent();
    }
  }

  // Optimize poolmemaligns
  getCallsOf(PoolMemAlign, Calls);
  for (unsigned i = 0, e = Calls.size(); i != e; ++i) {
    CallInst *CI = Calls[i];
    // poolmemalign(null, X, Y) -> memalign(X, Y)
    if (isa<ConstantPointerNull>(CI->getOperand(1))) {
      Value* Opts[2] = {CI->getOperand(2), CI->getOperand(3)};
      Value *New = new CallInst(MemAlign, Opts, Opts + 2, CI->getName(), CI);
      CI->replaceAllUsesWith(New);
      CI->eraseFromParent();
    }
  }

  // Optimize poolfrees
  getCallsOf(PoolFree, Calls);
  for (unsigned i = 0, e = Calls.size(); i != e; ++i) {
    CallInst *CI = Calls[i];
    // poolfree(PD, null) -> noop
    if (isa<ConstantPointerNull>(CI->getOperand(2)))
      CI->eraseFromParent();
    else if (isa<ConstantPointerNull>(CI->getOperand(1))) {
      // poolfree(null, Ptr) -> free(Ptr)
      new FreeInst(CI->getOperand(2), CI);
      CI->eraseFromParent();
    }
  }
      
  // Transform pools that only have poolinit/destroy/allocate uses into
  // bump-pointer pools.  Also, delete pools that are unused.  Find pools by
  // looking for pool inits in the program.
  getCallsOf(PoolInit, Calls);
  std::set<Value*> Pools;
  for (unsigned i = 0, e = Calls.size(); i != e; ++i)
    Pools.insert(Calls[i]->getOperand(1));

  // Loop over all of the pools processing each as we find it.
  for (std::set<Value*>::iterator PI = Pools.begin(), E = Pools.end();
       PI != E; ++PI) {
    bool HasPoolAlloc = false, HasOtherUse = false;
    Value *PoolDesc = *PI;
    for (Value::use_iterator UI = PoolDesc->use_begin(),
           E = PoolDesc->use_end(); UI != E; ++UI) {
      if (CallInst *CI = dyn_cast<CallInst>(*UI)) {
        if (CI->getCalledFunction() == PoolInit ||
            CI->getCalledFunction() == PoolDestroy) {
          // ignore
        } else if (CI->getCalledFunction() == PoolAlloc) {
          HasPoolAlloc = true;
        } else {
          HasOtherUse = true;
          break;
        }
      } else {
        HasOtherUse = true;
        break;
      }
    }

    // Can we optimize it?
    if (!HasOtherUse) {
      // Yes, if there are uses at all, nuke the pool init, destroy, and the PD.
      if (!HasPoolAlloc) {
        while (!PoolDesc->use_empty())
          cast<Instruction>(PoolDesc->use_back())->eraseFromParent();
        if (AllocaInst *AI = dyn_cast<AllocaInst>(PoolDesc))
          AI->eraseFromParent();
        else
          cast<GlobalVariable>(PoolDesc)->eraseFromParent();
      } else {
        // Convert all of the pool descriptor users to the BumpPtr flavor.
        std::vector<User*> PDUsers(PoolDesc->use_begin(), PoolDesc->use_end());
        
        while (!PDUsers.empty()) {
          CallInst *CI = cast<CallInst>(PDUsers.back());
          PDUsers.pop_back();
          std::vector<Value*> Args;
          if (CI->getCalledFunction() == PoolAlloc) {
            Args.assign(CI->op_begin()+1, CI->op_end());
            Value *New = new CallInst(PoolAllocBP, Args.begin(), Args.end(), CI->getName(), CI);
            CI->replaceAllUsesWith(New);
            CI->eraseFromParent();
          } else if (CI->getCalledFunction() == PoolInit) {
            Args.assign(CI->op_begin()+1, CI->op_end());
            Args.erase(Args.begin()+1); // Drop the size argument.
            new CallInst(PoolInitBP, Args.begin(), Args.end(), "", CI);
            CI->eraseFromParent();
          } else {
            assert(CI->getCalledFunction() == PoolDestroy);
            Args.assign(CI->op_begin()+1, CI->op_end());
            new CallInst(PoolDestroyBP, Args.begin(), Args.end(), "", CI);
            CI->eraseFromParent();
          }
        }
        ++NumBumpPtr;
      }
    }
  }
  return true;
}
