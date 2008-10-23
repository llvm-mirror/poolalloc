//===-- IndClonder.cpp - Clone Indirect Called Functions ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "indclone"

#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/Statistic.h"
#include <set>

using namespace llvm;

STATISTIC(numCloned, "Number of Functions Cloned");
STATISTIC(numReplaced, "Number of Calls Replaced");

namespace {
  class IndClone : public ModulePass {
  public:
    static char ID;
    IndClone() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
      std::set<Function*> toClone;
      for (Module::iterator I = M.begin(); I != M.end(); ++I)
        if (!I->isDeclaration() && !I->mayBeOverridden())
          for(Value::use_iterator ui = I->use_begin(), ue = I->use_end();
              ui != ue; ++ui)
            if (!isa<CallInst>(ui)) {
              toClone.insert(I);
            }
      numCloned += toClone.size();
      for (std::set<Function*>::iterator I = toClone.begin(), 
             E = toClone.end(); I != E; ++I) {
        Function* DirectF = CloneFunction(*I);
        DirectF->setName((*I)->getName() + "_DIRECT");
        DirectF->setLinkage(GlobalValue::InternalLinkage);
        (*I)->getParent()->getFunctionList().push_back(DirectF);
        for(Value::use_iterator ui = (*I)->use_begin(), ue = (*I)->use_end();
            ui != ue; ++ui)
          if (CallInst* CI = dyn_cast<CallInst>(ui))
            if (CI->getOperand(0) == *I) {
              ++numReplaced;
              CI->setOperand(0, DirectF);
            }
      }
      
      return true;
    }
  };
}

char IndClone::ID = 0;
static RegisterPass<IndClone>
X("indclone", "Indirect call cloning");
