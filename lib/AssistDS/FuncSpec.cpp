//===-- FuncSpec.cpp - Clone Functions With Constant Function Ptr Args ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass clones functions that take constant function pointers as arguments
// from some call sites. It changes those call sites to call cloned functions.
// 
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "funcspec"

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

using namespace llvm;

STATISTIC(numCloned, "Number of Functions Cloned in FuncSpec");
STATISTIC(numReplaced, "Number of Calls Replaced");

namespace {
  class FuncSpec : public ModulePass {
  public:
    static char ID;
    FuncSpec() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
      std::map<CallInst*, std::vector<std::pair<unsigned, Constant*> > > cloneSites;
      std::map<std::pair<Function*, std::vector<std::pair<unsigned, Constant*> > >, Function* > toClone;

      for (Module::iterator I = M.begin(); I != M.end(); ++I)
        if (!I->isDeclaration() && !I->mayBeOverridden()) {
          std::vector<unsigned> FPArgs;
          for (Function::arg_iterator ii = I->arg_begin(), ee = I->arg_end();
               ii != ee; ++ii)
            if (const PointerType* Ty = dyn_cast<PointerType>(ii->getType())) {
              if (isa<FunctionType>(Ty->getElementType())) {
                FPArgs.push_back(ii->getArgNo());
                DEBUG(errs() << "Eligible: " << I->getNameStr() << "\n");
              }
            } else if (isa<FunctionType>(ii->getType())) {
              FPArgs.push_back(ii->getArgNo());
              DEBUG(errs() << "Eligible: " << I->getNameStr() << "\n");
            } 
          for(Value::use_iterator ui = I->use_begin(), ue = I->use_end();
              ui != ue; ++ui) {
            if (CallInst* CI = dyn_cast<CallInst>(ui)) {
              if(CI->getCalledValue()->stripPointerCasts() == I) {
                std::vector<std::pair<unsigned, Constant*> > Consts;
                for (unsigned x = 0; x < FPArgs.size(); ++x)
                  if (Constant* C = dyn_cast<Constant>(ui->getOperand(FPArgs.at(x) + 1))) {
                    Consts.push_back(std::make_pair(FPArgs.at(x), C));
                  }
                if (!Consts.empty()) {
                  cloneSites[CI] = Consts;
                  toClone[std::make_pair(I, Consts)] = 0;
                }
              }
            }
          }
        }

      numCloned += toClone.size();

      for (std::map<std::pair<Function*, std::vector<std::pair<unsigned, Constant*> > >, Function* >::iterator I = toClone.begin(), E = toClone.end(); I != E; ++I) {
        Function* DirectF = CloneFunction(I->first.first);
        DirectF->setName(I->first.first->getNameStr() + "_SPEC");
        DirectF->setLinkage(GlobalValue::InternalLinkage);
        I->first.first->getParent()->getFunctionList().push_back(DirectF);
        I->second = DirectF;
      }
      
      for (std::map<CallInst*, std::vector<std::pair<unsigned, Constant*> > >::iterator ii = cloneSites.begin(), ee = cloneSites.end(); ii != ee; ++ii) {
        ii->first->setOperand(0, toClone[std::make_pair(cast<Function>(ii->first->getOperand(0)), ii->second)]);
        ++numReplaced;
      }

      return true;
    }
  };
}

char FuncSpec::ID = 0;
static RegisterPass<FuncSpec>
X("funcspec", "Specialize for Function Pointers");
