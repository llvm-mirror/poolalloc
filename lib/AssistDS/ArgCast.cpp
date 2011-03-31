//===-- ArgSimplify.cpp - Special case for conditional ptr args ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "argcast"

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

STATISTIC(numChanged,   "Number of Args bitcasted");

namespace {

  class ArgCast : public ModulePass {
  public:
    static char ID;
    ArgCast() : ModulePass(&ID) {}

    bool runOnModule(Module& M) {

      std::vector<CallInst*> worklist;
      for (Module::iterator I = M.begin(); I != M.end(); ++I) 
        if (!I->isDeclaration() && !I->mayBeOverridden()) {
          if(I->getNameStr() == "main")
            continue;
          for(Value::use_iterator ui = I->use_begin(), ue = I->use_end(); ui != ue; ++ui)
            if (Constant *C = dyn_cast<Constant>(ui)) 
              if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) 
                if (CE->getOpcode() == Instruction::BitCast) 
                  if(CE->getOperand(0) == I) 
                    if(const FunctionType *FTy  = dyn_cast<FunctionType>
                       ((cast<PointerType>(CE->getType()))->getElementType())) {
                      //casting to a varargs funtion
                      if(FTy->isVarArg()){
                        for(Value::use_iterator uii = CE->use_begin(),
                            uee = CE->use_end(); uii != uee; ++uii) 
                          if (CallInst* CI = dyn_cast<CallInst>(uii)) {
                            if(CI->getNumOperands() != I->arg_size() + 1)
                              continue;
                            if(CI->getCalledValue() == CE) 
                              worklist.push_back(CI);
                          }
                      }
                    }
        }
      while(!worklist.empty()) {
        CallInst *CI = worklist.back();
        worklist.pop_back();
        Function *F = cast<Function>(CI->getCalledValue()->stripPointerCasts());
        const FunctionType *FTy = F->getFunctionType();
        if(F->getReturnType() != CI->getType()) {
          continue;
        }

        SmallVector<Value*, 8> Args;
        unsigned i =0;
        for(i =0; i< FTy->getNumParams(); ++i) {
          const Type *ArgType = CI->getOperand(i+1)->getType();
          const Type *FormalType = FTy->getParamType(i);
          if(ArgType == FormalType)
            Args.push_back(CI->getOperand(i+1));

          else if(ArgType->isPointerTy() && FormalType->isPointerTy()){
            BitCastInst *BI = new BitCastInst(CI->getOperand(i+1), FormalType, "", CI);
            Args.push_back(BI);
          } else if (ArgType->isIntegerTy() && FormalType->isIntegerTy()) {
            CastInst *CastI = CastInst::CreateIntegerCast(CI->getOperand(i+1), FormalType, true, "", CI);
            Args.push_back(CastI);
          } else {
            ArgType->dump();
            FormalType->dump();
            break;
          }

        }
        if(i != FTy->getNumParams())
          continue;


        CallInst *CINew = CallInst::Create(F, Args.begin(), Args.end(), "", CI);
        CINew->setCallingConv(CI->getCallingConv());
        CINew->setAttributes(CI->getAttributes());
        CI->replaceAllUsesWith(CINew);
        CI->eraseFromParent();
        numChanged++;
      }
      return true;
    }
  };
}

char ArgCast::ID = 0;
static RegisterPass<ArgCast>
X("arg-cast", "Cast Arguments");
