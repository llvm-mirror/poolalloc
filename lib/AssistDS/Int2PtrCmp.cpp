//===-- Int2PtrCmp.cpp - Merge inttoptr/ptrtoint --------------------------===//
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
#include "llvm/Support/PatternMatch.h"
#include "llvm/Target/TargetData.h"

#include <set>
#include <map>
#include <vector>

using namespace llvm;
using namespace PatternMatch;

namespace {
  class Int2PtrCmp : public ModulePass {
  private:
    TargetData * TD;
  public:
    static char ID;
    Int2PtrCmp() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
    TD = &getAnalysis<TargetData>();
      //std::vector<PtrToIntInst*> worklist;
      for (Module::iterator F = M.begin(); F != M.end(); ++F) {
        for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
          for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
            if(PtrToIntInst *P2I = dyn_cast<PtrToIntInst>(I++)) {
              if(IntToPtrInst *I2P = dyn_cast<IntToPtrInst>(P2I->getOperand(0))) {
                if(I2P->getSrcTy() == P2I->getDestTy()){
                  P2I->replaceAllUsesWith(I2P->getOperand(0));
                  P2I->eraseFromParent();
                  if(I2P->use_empty())
                    I2P->eraseFromParent();
                }

              }
            }
          }
        }
      }
      
      //icmp pred inttoptr(X), null  -> icmp pred X 0
      for (Module::iterator F = M.begin(); F != M.end(); ++F) {
        for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
          for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
            if(ICmpInst *CI = dyn_cast<ICmpInst>(I++)) {
              Value *Op0 = CI->getOperand(0);
              Value *Op1 = CI->getOperand(1);
              if (Constant *RHSC = dyn_cast<Constant>(Op1)) {
                if (Instruction *LHSI = dyn_cast<Instruction>(Op0)){
                  if(LHSI->getOpcode() == Instruction::IntToPtr) {
                    if (RHSC->isNullValue() && TD &&
                        TD->getIntPtrType(RHSC->getContext()) ==
                        LHSI->getOperand(0)->getType()){
                      ICmpInst *CI_new = new ICmpInst(CI, CI->getPredicate(), LHSI->getOperand(0),
                                                      Constant::getNullValue(LHSI->getOperand(0)->getType()));
                      
                      CI->replaceAllUsesWith(CI_new);
                      CI->eraseFromParent();
                      if(LHSI->use_empty())
                        LHSI->eraseFromParent();
                    }
                  }
                }
              }
            }
          }
        }
      }

      for (Module::iterator F = M.begin(); F != M.end(); ++F) {
        for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
          for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
            if(ICmpInst *ICI = dyn_cast<ICmpInst>(I++)) {
              Value *Op0 = ICI->getOperand(0);
              Value *Op1 = ICI->getOperand(1);
              if (ConstantInt *CI = dyn_cast<ConstantInt>(Op1)) {
                // Since the RHS is a ConstantInt (CI), if the left hand side is an
                // instruction, see if that instruction also has constants so that the
                // instruction can be folded into the icmp
                if (Instruction *LHSI = dyn_cast<Instruction>(Op0)){
                  if(LHSI->getOpcode() == Instruction::Or) {
                    if (!ICI->isEquality() || !CI->isNullValue() || !LHSI->hasOneUse())
                      break;
                    Value *P, *Q, *R, *S;
                    if (match(LHSI, m_Or(m_PtrToInt(m_Value(P)), m_PtrToInt(m_Value(Q))))) {
                      Value *ICIP = new ICmpInst(ICI, ICI->getPredicate(), P,
                                                        Constant::getNullValue(P->getType()));
                      Value *ICIQ = new ICmpInst(ICI, ICI->getPredicate(), Q,
                                                        Constant::getNullValue(Q->getType()));
                      Instruction *Op;
                      if (ICI->getPredicate() == ICmpInst::ICMP_EQ)
                        Op = BinaryOperator::CreateAnd(ICIP, ICIQ,"",ICI);
                      else
                        Op = BinaryOperator::CreateOr(ICIP, ICIQ, "", ICI);
                      ICI->replaceAllUsesWith(Op);

                    } else if(match(LHSI, m_Or(m_Or(m_PtrToInt(m_Value(P)), m_PtrToInt(m_Value(Q))), m_PtrToInt(m_Value(R))))) {
                      Value *ICIP = new ICmpInst(ICI, ICI->getPredicate(), P,
                                                        Constant::getNullValue(P->getType()));
                      Value *ICIQ = new ICmpInst(ICI, ICI->getPredicate(), Q,
                                                        Constant::getNullValue(Q->getType()));
                      Value *ICIR = new ICmpInst(ICI, ICI->getPredicate(), R,
                                                        Constant::getNullValue(R->getType()));
                      Instruction *Op;
                      if (ICI->getPredicate() == ICmpInst::ICMP_EQ)
                        Op = BinaryOperator::CreateAnd(ICIP, ICIQ,"",ICI);
                      else
                        Op = BinaryOperator::CreateOr(ICIP, ICIQ, "", ICI);

                      if (ICI->getPredicate() == ICmpInst::ICMP_EQ)
                        Op = BinaryOperator::CreateAnd(Op, ICIR,"",ICI);
                      else
                        Op = BinaryOperator::CreateOr(Op, ICIR, "", ICI);
                      ICI->replaceAllUsesWith(Op);

                    } else if(match(LHSI, m_Or(m_PtrToInt(m_Value(Q)), m_Or(m_PtrToInt(m_Value(P)), m_PtrToInt(m_Value(R)))))) {
                      Value *ICIP = new ICmpInst(ICI, ICI->getPredicate(), P,
                                                        Constant::getNullValue(P->getType()));
                      Value *ICIQ = new ICmpInst(ICI, ICI->getPredicate(), Q,
                                                        Constant::getNullValue(Q->getType()));
                      Value *ICIR = new ICmpInst(ICI, ICI->getPredicate(), R,
                                                        Constant::getNullValue(R->getType()));
                      Instruction *Op;
                      if (ICI->getPredicate() == ICmpInst::ICMP_EQ)
                        Op = BinaryOperator::CreateAnd(ICIP, ICIQ,"",ICI);
                      else
                        Op = BinaryOperator::CreateOr(ICIP, ICIQ, "", ICI);

                      if (ICI->getPredicate() == ICmpInst::ICMP_EQ)
                        Op = BinaryOperator::CreateAnd(Op, ICIR,"",ICI);
                      else
                        Op = BinaryOperator::CreateOr(Op, ICIR, "", ICI);
                      ICI->replaceAllUsesWith(Op);
                    }
                  }
                }
              }
            }
          }
        }
      }
      return true;
    }
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<TargetData>();
    }
  };
}

char Int2PtrCmp::ID = 0;
static RegisterPass<Int2PtrCmp>
X("int2ptrcmp", "Simplify inttoptr/ptrtoint if derived from the other");
