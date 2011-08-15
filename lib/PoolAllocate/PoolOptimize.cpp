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

static Type * VoidType  = 0;
static Type * Int8Type  = 0;
static Type * Int32Type = 0;

namespace {
  STATISTIC (NumBumpPtr, "Number of bump pointer pools");

  struct PoolOptimize : public ModulePass {
    static char ID;
    bool SAFECodeEnabled;

    PoolOptimize(bool SAFECode = true) : ModulePass(ID) {
      SAFECodeEnabled = SAFECode;
    }
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
  if (!(F=dyn_cast<Function>(C))) {
    if ((CE = dyn_cast<ConstantExpr>(C)) && (CE->isCast()))
      F = dyn_cast<Function>(CE->getOperand(0));
    else
      assert (0 && "Constant is not a Function of ConstantExpr!"); 
  }
  Calls.clear();
  for (Value::use_iterator UI = F->use_begin(), E = F->use_end(); UI != E; ++UI)
    Calls.push_back(cast<CallInst>(*UI));
}

bool PoolOptimize::runOnModule(Module &M) {
  //
  // Get pointers to 8 and 32 bit LLVM integer types.
  //
  VoidType  = Type::getVoidTy(M.getContext());
  Int8Type  = IntegerType::getInt8Ty(M.getContext());
  Int32Type = IntegerType::getInt32Ty(M.getContext());

  //
  // Create LLVM types used by the pool allocation passes.
  //
  Type *VoidPtrTy = PointerType::getUnqual(Int8Type);
  Type *PoolDescPtrTy;
  if (SAFECodeEnabled)
    PoolDescPtrTy = PointerType::getUnqual(ArrayType::get(VoidPtrTy, 92));
  else
    PoolDescPtrTy = PointerType::getUnqual(ArrayType::get(VoidPtrTy, 16));

  // Get poolinit function.
  Constant *PoolInit = M.getOrInsertFunction("poolinit", VoidType,
                                             PoolDescPtrTy, Int32Type,
                                             Int32Type, NULL);

  // Get pooldestroy function.
  Constant *PoolDestroy = M.getOrInsertFunction("pooldestroy", VoidType,
                                                PoolDescPtrTy, NULL);
  
  // The poolalloc function.
  Constant *PoolAlloc = M.getOrInsertFunction("poolalloc", 
                                              VoidPtrTy, PoolDescPtrTy,
                                              Int32Type, NULL);
  
  // The poolrealloc function.
  Constant *PoolRealloc = M.getOrInsertFunction("poolrealloc",
                                                VoidPtrTy, PoolDescPtrTy,
                                                VoidPtrTy, Int32Type, NULL);
  // The poolmemalign function.
  Constant *PoolMemAlign = M.getOrInsertFunction("poolmemalign",
                                                 VoidPtrTy, PoolDescPtrTy,
                                                 Int32Type, Int32Type, 
                                                 NULL);

  // Get the poolfree function.
  Constant *PoolFree = M.getOrInsertFunction("poolfree", VoidType,
                                             PoolDescPtrTy, VoidPtrTy, NULL);  


  // Get poolinit_bp function.
  Constant *PoolInitBP = M.getOrInsertFunction("poolinit_bp", VoidType,
                                               PoolDescPtrTy, Int32Type, 
                                               NULL);
  
  // Get pooldestroy_bp function.
  Constant *PoolDestroyBP = M.getOrInsertFunction("pooldestroy_bp",VoidType,
                                                 PoolDescPtrTy, NULL);
  
  // The poolalloc_bp function.
  Constant *PoolAllocBP = M.getOrInsertFunction("poolalloc_bp", 
                                                VoidPtrTy, PoolDescPtrTy,
                                                Int32Type, NULL);

  Constant *Realloc = M.getOrInsertFunction("realloc",
                                            VoidPtrTy, VoidPtrTy, Int32Type,
                                            NULL);
  Constant *MemAlign = M.getOrInsertFunction("memalign",
                                             VoidPtrTy, Int32Type,
                                             Int32Type, NULL);


  // Optimize poolreallocs
  std::vector<CallInst*> Calls;
  getCallsOf(PoolRealloc, Calls);
  for (unsigned i = 0, e = Calls.size(); i != e; ++i) {
    CallInst *CI = Calls[i];
    // poolrealloc(PD, null, X) -> poolalloc(PD, X)
    if (isa<ConstantPointerNull>(CI->getOperand(2))) {
      Value* Opts[2] = {CI->getOperand(1), CI->getOperand(3)};
      Value *New = CallInst::Create(PoolAlloc, Opts,
                                CI->getName(), CI);
      CI->replaceAllUsesWith(New);
      CI->eraseFromParent();
    } else if (isa<Constant>(CI->getOperand(3)) && 
               cast<Constant>(CI->getOperand(3))->isNullValue()) {
      // poolrealloc(PD, X, 0) -> poolfree(PD, X)
      Value* Opts[2] = {CI->getOperand(1), CI->getOperand(2)};
      CallInst::Create(PoolFree, Opts, "", CI);
      PointerType * PT = dyn_cast<PointerType>(CI->getType());
      assert (PT && "poolrealloc call does not return a pointer!\n");
      CI->replaceAllUsesWith(ConstantPointerNull::get(PT));
      CI->eraseFromParent();
    } else if (isa<ConstantPointerNull>(CI->getOperand(1))) {
      // poolrealloc(null, X, Y) -> realloc(X, Y)
      Value* Opts[2] = {CI->getOperand(2), CI->getOperand(3)};
      Value *New = CallInst::Create(Realloc, Opts,
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
//FIXME: handle malloc
      #if 0
      Value *New = new MallocInst(Int8Type, CI->getOperand(2),
                                  CI->getName(), CI);
      CI->replaceAllUsesWith(New);
      CI->eraseFromParent();
      #endif
    }
  }

  // Optimize poolmemaligns
  getCallsOf(PoolMemAlign, Calls);
  for (unsigned i = 0, e = Calls.size(); i != e; ++i) {
    CallInst *CI = Calls[i];
    // poolmemalign(null, X, Y) -> memalign(X, Y)
    if (isa<ConstantPointerNull>(CI->getOperand(1))) {
      Value* Opts[2] = {CI->getOperand(2), CI->getOperand(3)};
      Value *New = CallInst::Create(MemAlign, Opts, CI->getName(), CI);
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
      //FIXME: Handle free
      //new FreeInst(CI->getOperand(2), CI);
      //CI->eraseFromParent();
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
            Value *New = CallInst::Create(PoolAllocBP, Args, CI->getName(), CI);
            CI->replaceAllUsesWith(New);
            CI->eraseFromParent();
          } else if (CI->getCalledFunction() == PoolInit) {
            Args.assign(CI->op_begin()+1, CI->op_end());
            Args.erase(Args.begin()+1); // Drop the size argument.
            CallInst::Create(PoolInitBP, Args, "", CI);
            CI->eraseFromParent();
          } else {
            assert(CI->getCalledFunction() == PoolDestroy);
            Args.assign(CI->op_begin()+1, CI->op_end());
            CallInst::Create(PoolDestroyBP, Args, "", CI);
            CI->eraseFromParent();
          }
        }
        ++NumBumpPtr;
      }
    }
  }
  return true;
}
