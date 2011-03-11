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
              ui != ue; ++ui) 
            //Bitcast
            if (Constant *C = dyn_cast<Constant>(ui)) 
              if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) 
                if (CE->getOpcode() == Instruction::BitCast) 
                  if(CE->getOperand(0) == I) 
                    if(const FunctionType *FTy  = dyn_cast<FunctionType>
                       ((cast<PointerType>(CE->getType()))->getElementType())) 
                      //casting to a varargs funtion
                      if(FTy->isVarArg()) 
                        for(Value::use_iterator uii = CE->use_begin(),
                            uee = CE->use_end(); uii != uee; ++uii) 
                          if (CallInst* CI = dyn_cast<CallInst>(uii)) 
                            if(CI->getCalledValue() == CE) 
                               worklist.push_back(CI);
        }
      }
      // process the worklist

      while(!worklist.empty()) {
        CallInst *CI = worklist.back();
        worklist.pop_back();
        Function *F = cast<Function>(CI->getCalledValue()->stripPointerCasts());
        // Only continue, if we are passing the exact number of arguments
        if(F->arg_size() != (CI->getNumOperands()-1))
          continue;
        // Only continue if we are getting the same return type value
        // Or we can discard the returned value.
        if(F->getReturnType() != CI->getType()) {
          if(!CI->use_empty())
          continue;
        }

        // Check if the parameters passed match the expected types of the 
        // formal arguments
        bool change = true;
        unsigned arg_count = 1;
        for (Function::arg_iterator ii = F->arg_begin(), ee = F->arg_end();ii != ee; ++ii,arg_count++) {
          if(ii->getType() != CI->getOperand(arg_count)->getType()) {
            change = false;
            break;
          }
        }
        
        if(change) {
            // if we want to ignore the returned value, create a new CallInst
            SmallVector<Value*, 8> Args;
            for(unsigned j =1;j<CI->getNumOperands();j++) {
              Args.push_back(CI->getOperand(j));
            }
            CallInst *CINew = CallInst::Create(F, Args.begin(), Args.end(), "", CI);
            if(F->getReturnType() != CI->getType()){ // means no uses
              CINew->setDoesNotReturn();
            } else {
              CI->replaceAllUsesWith(CINew);
            }
            CI->eraseFromParent();
            // else just set the function to call the original function.
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
