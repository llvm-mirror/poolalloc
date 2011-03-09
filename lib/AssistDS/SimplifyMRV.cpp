//===-- SimplifyMRV.cpp - Remove extraneous insert/extractvalue insts------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "simplifymrv"

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

STATISTIC(numRemoved, "Number of Instructions Deleted");

namespace {
  class SimplifyMRV : public ModulePass {
  public:
    static char ID;
    SimplifyMRV() : ModulePass(&ID) {}
    bool runOnModule(Module& M) {
      bool changed = false;
      do {
        changed = false;
        for (Module::iterator F = M.begin(); F != M.end(); ++F) {
          for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
            for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
              if(ExtractValueInst *EV = dyn_cast<ExtractValueInst>(I++)) {
                Value *Agg = EV->getAggregateOperand();
                if (!EV->hasIndices()) {
                  EV->replaceAllUsesWith(Agg);
                  EV->eraseFromParent();
                  numRemoved++;
                  changed = true;
                  continue;
                }
                if (Constant *C = dyn_cast<Constant>(Agg)) {
                  if (isa<UndefValue>(C)) {
                    EV->replaceAllUsesWith(UndefValue::get(EV->getType()));
                    EV->eraseFromParent();
                    numRemoved++;
                    changed = true;
                    continue;
                  }
                  if (isa<ConstantAggregateZero>(C)) {
                    EV->replaceAllUsesWith(Constant::getNullValue(EV->getType()));
                    numRemoved++;
                    EV->eraseFromParent();
                    changed = true;
                    continue;
                  }
                  if (isa<ConstantArray>(C) || isa<ConstantStruct>(C)) {
                    // Extract the element indexed by the first index out of the constant
                    Value *V = C->getOperand(*EV->idx_begin());
                    if (EV->getNumIndices() > 1) {
                      // Extract the remaining indices out of the constant indexed by the
                      // first index
                      ExtractValueInst *EV_new = ExtractValueInst::Create(V, EV->idx_begin() + 1, EV->idx_end(), "", EV);
                      EV->replaceAllUsesWith(EV_new);
                      EV->eraseFromParent();
                      numRemoved++;
                      changed = true;
                      continue;
                    }  else {
                      EV->replaceAllUsesWith(V);
                      EV->eraseFromParent();
                      numRemoved++;
                      changed = true;
                      continue;
                    }
                  }
                  continue;
                }
                if (InsertValueInst *IV = dyn_cast<InsertValueInst>(Agg)) {
                  bool done = false;
                  // We're extracting from an insertvalue instruction, compare the indices
                  const unsigned *exti, *exte, *insi, *inse;
                  for (exti = EV->idx_begin(), insi = IV->idx_begin(),
                       exte = EV->idx_end(), inse = IV->idx_end();
                       exti != exte && insi != inse;
                       ++exti, ++insi) {
                    if (*insi != *exti) {
                      // The insert and extract both reference distinctly different elements.
                      // This means the extract is not influenced by the insert, and we can
                      // replace the aggregate operand of the extract with the aggregate
                      // operand of the insert. i.e., replace
                      // %I = insertvalue { i32, { i32 } } %A, { i32 } { i32 42 }, 1
                      // %E = extractvalue { i32, { i32 } } %I, 0
                      // with
                      // %E = extractvalue { i32, { i32 } } %A, 0
                      ExtractValueInst *EV_new = ExtractValueInst::Create(IV->getAggregateOperand(),
                                                                          EV->idx_begin(), EV->idx_end(),"", EV);
                      EV->replaceAllUsesWith(EV_new);
                      EV->eraseFromParent();
                      numRemoved++;
                      done = true;
                      changed = true;
                      break;
                    }
                  }
                  if(done)
                    continue;
                  if (exti == exte && insi == inse) {
                    // Both iterators are at the end: Index lists are identical. Replace
                    // %B = insertvalue { i32, { i32 } } %A, i32 42, 1, 0
                    // %C = extractvalue { i32, { i32 } } %B, 1, 0
                    // with "i32 42"
                    EV->replaceAllUsesWith(IV->getInsertedValueOperand());
                    EV->eraseFromParent();
                    numRemoved++;
                    changed = true;
                    continue;

                  }
                  if (exti == exte) {
                    // The extract list is a prefix of the insert list. i.e. replace
                    // %I = insertvalue { i32, { i32 } } %A, i32 42, 1, 0
                    // %E = extractvalue { i32, { i32 } } %I, 1
                    // with
                    // %X = extractvalue { i32, { i32 } } %A, 1
                    // %E = insertvalue { i32 } %X, i32 42, 0
                    // by switching the order of the insert and extract (though the
                    // insertvalue should be left in, since it may have other uses).
                    Value *NewEV = ExtractValueInst::Create(IV->getAggregateOperand(),
                                                            EV->idx_begin(), EV->idx_end(), "", EV);
                    Value *NewIV = InsertValueInst::Create(NewEV, IV->getInsertedValueOperand(),
                                                           insi, inse, "", EV);
                    EV->replaceAllUsesWith(NewIV);
                    EV->eraseFromParent();
                    numRemoved++;
                    changed = true;
                    continue;
                  }
                  if (insi == inse) {
                    // The insert list is a prefix of the extract list
                    // We can simply remove the common indices from the extract and make it
                    // operate on the inserted value instead of the insertvalue result.
                    // i.e., replace
                    // %I = insertvalue { i32, { i32 } } %A, { i32 } { i32 42 }, 1
                    // %E = extractvalue { i32, { i32 } } %I, 1, 0
                    // with
                    // %E extractvalue { i32 } { i32 42 }, 0
                    ExtractValueInst *EV_new = ExtractValueInst::Create(IV->getInsertedValueOperand(),
                                                                        exti, exte,"", EV);
                    EV->replaceAllUsesWith(EV_new);
                    EV->eraseFromParent();
                    numRemoved++;
                    changed = true;
                    continue;
                  }



                }
              }
            }
          }
        }
      } while(changed);
      return true;
    }
  };
}

char SimplifyMRV::ID = 0;
static RegisterPass<SimplifyMRV>
X("simplify-mrv", "Simplify extract/insert value insts created due to mrvs");
