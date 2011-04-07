//===-- SimplifyExtractValue.cpp - Remove extraneous extractvalue insts----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Replace extract value by loads where possible
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "simplify-ev"

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

// Pass statistic
STATISTIC(numErased, "Number of Instructions Deleted");

namespace {
  class SimplifyEV : public ModulePass {
  public:
    static char ID;
    SimplifyEV() : ModulePass(&ID) {}
    //
    // Method: runOnModule()
    //
    // Description:
    //  Entry point for this LLVM pass. Search for extractvalue instructions
    //  that can be simplified.
    //
    // Inputs:
    //  M - A reference to the LLVM module to transform.
    //
    // Outputs:
    //  M - The transformed LLVM module.
    //
    // Return value:
    // true  - The module was modified.
    // false - The module was not modified.
    //
    bool runOnModule(Module& M) {
      // Repeat till no change
      bool changed;
      do {
        changed = false;
        for (Module::iterator F = M.begin(); F != M.end(); ++F) {
          for (Function::iterator B = F->begin(), FE = F->end(); B != FE; ++B) {      
            for (BasicBlock::iterator I = B->begin(), BE = B->end(); I != BE;) {
              ExtractValueInst *EV = dyn_cast<ExtractValueInst>(I++);
              if(!EV)
                continue;
              Value *Agg = EV->getAggregateOperand();
              LoadInst *LI = dyn_cast<LoadInst>(Agg);
              if(!LI)
                continue;
              // check that it is in same basic block
              SmallVector<Value*, 8> Indices;
              const Type *Int32Ty = Type::getInt32Ty(M.getContext());
              Indices.push_back(Constant::getNullValue(Int32Ty));
              for (ExtractValueInst::idx_iterator I = EV->idx_begin(), E = EV->idx_end();
                               I != E; ++I) {
                Indices.push_back(ConstantInt::get(Int32Ty, *I));
              }

              GetElementPtrInst *GEP = GetElementPtrInst::CreateInBounds(LI->getOperand(0), Indices.begin(),
                                                                         Indices.end(), LI->getName(), LI) ;
              LoadInst *LINew = new LoadInst(GEP, "", LI);
              EV->replaceAllUsesWith(LINew);
              EV->eraseFromParent();
              changed = true;
              numErased++;


            }
          }
        }
      } while(changed);
      return (numErased > 0);
    }
  };
}

// Pass ID variable
char SimplifyEV::ID = 0;

// Register the pass
static RegisterPass<SimplifyEV>
X("simplify-ev", "Simplify extract value");
