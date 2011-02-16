//===-- VarArgsFunc.cpp - Simplify calls to bitcasted const funcs --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "varargfunc"

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

STATISTIC(numSimplified, "Number of Calls Simplified");

namespace {
  class VarArgsFunc : public ModulePass {
  public:
    static char ID;
    VarArgsFunc() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
      bool changed = false;
      std::vector<CallInst*> worklist;
      for (Module::iterator I = M.begin(); I != M.end(); ++I) {
        if (!I->isDeclaration() && !I->mayBeOverridden()) {
          //Call Sites
          for(Value::use_iterator ui = I->use_begin(), ue = I->use_end();
              ui != ue; ++ui) {
            if (Constant *C = dyn_cast<Constant>(ui)) {
              if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
                if (CE->getOpcode() == Instruction::BitCast) {
                  if(CE->getOperand(0) == I) {                    
                    if(const FunctionType *FTy  = dyn_cast<FunctionType>((cast<PointerType>(CE->getType()))->getElementType())) {
                      if(FTy->isVarArg()) {
                        for(Value::use_iterator uii = CE->use_begin(), uee = CE->use_end();
                            uii != uee; ++uii) {
                          if (CallInst* CI = dyn_cast<CallInst>(uii)) {
                            if(CI->getCalledValue() == CE) {
                              worklist.push_back(CI);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      while(!worklist.empty()) {
        CallInst *CI = worklist.back();
        worklist.pop_back();
        Function *F = cast<Function>(CI->getCalledValue()->stripPointerCasts());
        if(F->arg_size() != (CI->getNumOperands()-1))
          continue;
        if(F->getReturnType() != CI->getType())
          continue;
        int arg_count = 1;
        bool change = true;
        for (Function::arg_iterator ii = F->arg_begin(), ee = F->arg_end();ii != ee; ++ii,arg_count++) {
          if(ii->getType() != CI->getOperand(arg_count)->getType()) {
            change = false;
            break;
          }
        }
        
        if(change) {
          CI->setCalledFunction(F);
          changed = true;
          numSimplified++;
        }
      }
      return changed;
    }
  };
}

char VarArgsFunc::ID = 0;
static RegisterPass<VarArgsFunc>
X("varargsfunc", "Specialize for ill-defined non-varargs functions");
