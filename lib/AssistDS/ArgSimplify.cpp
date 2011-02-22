//===-- FuncSpec.cpp - Clone Functions With Constant Function Ptr Args ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "argsimpl"

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

STATISTIC(numTransformable,   "Number of Args changeable");
namespace {
  static void simplify(Function *I, unsigned arg_count, const Type* type) {
    for(Value::use_iterator ui = I->use_begin(), ue = I->use_end();
        ui != ue; ++ui) {
      if (Constant *C = dyn_cast<Constant>(ui)) {
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
          if (CE->getOpcode() == Instruction::BitCast) {
            if(CE->getOperand(0) == I) {                    
              for(Value::use_iterator uii = CE->use_begin(), uee = CE->use_end();
                  uii != uee; ) {
                if (CallInst* CI = dyn_cast<CallInst>(uii)) {
                        ++uii;
                  if(CI->getCalledValue() == CE) {
                    if(I->getReturnType() == CI->getType()){
                      if(I->arg_size() == (CI->getNumOperands()-1)){
                        unsigned arg_count1 = 1;
                        bool change = true;
                        for (Function::arg_iterator ii1 = I->arg_begin(), ee1 = I->arg_end();ii1 != ee1; ++ii1,arg_count1++) {
                          if(ii1->getType() != CI->getOperand(arg_count1)->getType()) {
                            if(arg_count1 == (arg_count + 1)) {
                              continue;
                            }
                            change = false;
                            break;
                          }
                        }
                        if(change){
                          std::vector<const Type*>TP;
                          for(unsigned c = 1; c<CI->getNumOperands();c++) {
                            TP.push_back(CI->getOperand(c)->getType());
                          }
                          const FunctionType *NewFTy = FunctionType::get(CI->getType(), TP, false);
                          
                          Module *M = I->getParent();
                          Function *NewF = Function::Create(NewFTy,
                                                            GlobalValue::InternalLinkage,
                                                            "argbounce",
                                                            M);
                          std::vector<Value*> fargs;
                          for(Function::arg_iterator ai = NewF->arg_begin(), ae= NewF->arg_end(); ai != ae; ++ai) {
                            fargs.push_back(ai);
                            ai->setName("arg");
                          }
                          Value *CastedVal;
                          BasicBlock* entryBB = BasicBlock::Create (M->getContext(), "entry", NewF);
                          if(type->isIntegerTy()){
                            CastedVal = new PtrToIntInst(fargs.at(arg_count), type, "castd", entryBB);
                          } else {
                            CastedVal = new BitCastInst(fargs.at(arg_count), type, "castd", entryBB);
                          }
                          SmallVector<Value*, 8> Args;
                          for(Function::arg_iterator ai = NewF->arg_begin(), ae= NewF->arg_end(); ai != ae; ++ai) {
                            if(ai->getArgNo() == arg_count)
                              Args.push_back(CastedVal);
                            else 
                              Args.push_back(ai);
                          }
                          
                          CallInst * CallI = CallInst::Create(I,Args.begin(), Args.end(),"", entryBB);
                          if(CallI->getType()->isVoidTy())
                            ReturnInst::Create(M->getContext(), entryBB);
                          else 
                            ReturnInst::Create(M->getContext(), CallI, entryBB);
                          //new BitCastInst(fargs.at(ii->getArgNo()), ii->getType(), "test", entryBB);
                          //new UnreachableInst (M.getContext(), entryBB);
                          
                          CI->setCalledFunction(NewF);
                          numTransformable++;
                        }
                      }
                    }
                  }
                } else {
                  ++uii;
                }
              }
            }
          }
        }
      }
    }

  }
  class ArgSimplify : public ModulePass {
  public:
    static char ID;
    ArgSimplify() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
      for (Module::iterator I = M.begin(); I != M.end(); ++I) {
        if (!I->isDeclaration() && !I->mayBeOverridden()) {
          if(I->getNameStr() == "main")
            continue;
          std::vector<unsigned> Args;
          for (Function::arg_iterator ii = I->arg_begin(), ee = I->arg_end();
               ii != ee; ++ii) {
            bool change = true;
            for(Value::use_iterator ui = ii->use_begin(), ue = ii->use_end();
                ui != ue; ++ui) {
              if(!isa<ICmpInst>(ui)){
                change = false;
                break;
              }
            }
            if(change){
              simplify(I, ii->getArgNo(), ii->getType()); 
            }
          }
        }
      }

      return true;
    }
  };
}

char ArgSimplify::ID = 0;
static RegisterPass<ArgSimplify>
X("arg-simplify", "Specialize for Conditional Arguments");
