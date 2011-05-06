//===-- TypeAnalysis.cpp - Clone Indirectly Called Functions --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "assistDS/TypeAnalysis.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"

#include <vector>

using namespace llvm;

// Pass ID variable
char TypeAnalysis::ID = 0;

// Register the Pass
static RegisterPass<TypeAnalysis>
X("type-analysis", "Get types for load/stores");

//
// Method: runOnModule()
//
bool
TypeAnalysis::runOnModule(Module& M) {
  return false;
}

const Type *
TypeAnalysis::getType(LoadInst *LI){
  return LI->getType();
}

const Type *
TypeAnalysis::getType(StoreInst *SI){
  return SI->getOperand(0)->getType();
}

bool
TypeAnalysis::isCopyingLoad(LoadInst *LI){
  if(LI->getNumUses() == 1) {
    if(StoreInst *SI = dyn_cast<StoreInst>(LI->use_begin())) {
      if(SI->getOperand(0) == LI) {
        return true;
      }
    }
  }
  // chk if passed through argument, and then stored.
  return false;
}


bool 
TypeAnalysis::isCopyingStore(StoreInst *SI) {
  if(SI->getOperand(0)->getNumUses() == 1) {
    if(isa<LoadInst>(SI->getOperand(0))) {
      return true;
    }
  }
  return false;
}

Value *
TypeAnalysis::getStoreSource(StoreInst *SI) {
  if(LoadInst *LI = dyn_cast<LoadInst>(SI->getOperand(0))) {
    return LI->getOperand(0);
  }
  return NULL;
}


void 
TypeAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}
