//===-- VarArgsFunc.cpp - Simplify calls to bitcasted const funcs --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "allocator-identify"

#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"

#include <set>
#include <map>
#include <vector>
#include <string>

using namespace llvm;


namespace {
  static bool flowsFrom(Value *Dest,Value *Src) {
    if(Dest == Src)
      return true;
    if(ReturnInst *Ret = dyn_cast<ReturnInst>(Dest)) {    
      return flowsFrom(Ret->getReturnValue(), Src);
    } 
    if(PHINode *PN = dyn_cast<PHINode>(Dest)) {
      bool ret = true;
      for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
        ret = ret && flowsFrom(PN->getIncomingValue(i), Src);
      }
      return ret;
    }
    if(BitCastInst *BI = dyn_cast<BitCastInst>(Dest)) {
      return flowsFrom(BI->getOperand(0), Src);
    }
    if(isa<ConstantPointerNull>(Dest))
      return true;
    return false;
  }
  static bool isNotStored(Value *V) {
    // check that V is not stroed to a location taht is accessible outside this fn

    for(Value::use_iterator ui = V->use_begin(), ue = V->use_end();
        ui != ue; ++ui) {
      if(isa<StoreInst>(ui))
        return false;
      if(isa<ICmpInst>(ui))
        continue;
      if(isa<ReturnInst>(ui))
        continue;
      if(BitCastInst *BI = dyn_cast<BitCastInst>(ui))
        if(isNotStored(BI))
          continue;
        else 
          return false;
      if(PHINode *PN = dyn_cast<PHINode>(ui))
        if(isNotStored(PN))
          continue;
        else 
          return false;

      //ui->dump();
      return false;
    }
    return true;
  }
  class AllocIdentify : public ModulePass {
  protected:
    std::set<std::string> allocators;
  public:
    static char ID;
    AllocIdentify() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {

      allocators.insert("malloc");
      allocators.insert("calloc");
      allocators.insert("realloc");
      allocators.insert("memset");

      bool changed;
      do {
        changed = false;
        std::set<std::string> TempAllocators;
        TempAllocators.insert( allocators.begin(), allocators.end());
        std::set<std::string>::iterator it;
        for(it = TempAllocators.begin(); it != TempAllocators.end(); ++it) {
          Function* F = M.getFunction(*it);
          if(!F)
            continue;
          for(Value::use_iterator ui = F->use_begin(), ue = F->use_end();
              ui != ue; ++ui) {
            // iterate though all calls to malloc
            if (CallInst* CI = dyn_cast<CallInst>(ui)) {
              // The function that calls malloc could be a potential allocator
              Function *WrapperF = CI->getParent()->getParent();
              if(WrapperF->doesNotReturn())
                continue;
              if(!(WrapperF->getReturnType()->isPointerTy()))
                continue;
              bool isWrapper = true;
              for (Function::iterator BBI = WrapperF->begin(), E = WrapperF->end(); BBI != E; ) {
                BasicBlock &BB = *BBI++;

                // Only look at return blocks.
                ReturnInst *Ret = dyn_cast<ReturnInst>(BB.getTerminator());
                if (Ret == 0) continue;

                //check for ALL return values
                if(flowsFrom(Ret, CI)) {
                  continue;
                } else {
                  isWrapper = false;
                  break;
                }
                // if true for all return add to list of allocators
              }
              if(isWrapper)
                isWrapper = isWrapper && isNotStored(CI);
              if(isWrapper) {
                errs() << WrapperF->getNameStr() << "\n";
                changed = (allocators.find(WrapperF->getName()) == allocators.end());
                allocators.insert(WrapperF->getName());
              }
            }
          }
        }
      } while(changed);
      return false;
    }
  };
}

char AllocIdentify::ID = 0;
static RegisterPass<AllocIdentify>
X("alloc-identify", "Identify allocator wrapper functions");
