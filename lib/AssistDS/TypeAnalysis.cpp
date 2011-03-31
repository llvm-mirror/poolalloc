//===-- TypeAnalysis.cpp - Clone Indirectly Called Functions --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "assistDS/TypeAnalysis.h"
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
const Type *
TypeAnalysis::getType(InsertValueInst *I){
  return I->getInsertedValueOperand()->getType();
}
const Type *
TypeAnalysis::getType(ExtractValueInst *I){
  return I->getType();
}
bool
TypeAnalysis::isCopyingLoad(LoadInst *LI){
  if(LI->getNumUses() == 1)
    if(StoreInst *SI = dyn_cast<StoreInst>(LI->use_begin())) {
      if(SI->getOperand(0) == LI)
        return true;
    } else if(InsertValueInst *IV = dyn_cast<InsertValueInst>(LI->use_begin())) {
      if(IV->getInsertedValueOperand() == LI)
        return true;
    }
  return false;
}
bool 
TypeAnalysis::isCopyingLoad(ExtractValueInst * EI) {
  if(EI->getNumUses() == 1)
    if(StoreInst *SI = dyn_cast<StoreInst>(EI->use_begin())) {
      if(SI->getOperand(0) == EI)
        return true;
    } else if(InsertValueInst *IV = dyn_cast<InsertValueInst>(EI->use_begin())) {
      if(IV->getInsertedValueOperand() == EI)
        return true;
    }
  return false;
}
bool 
TypeAnalysis::isCopyingStore(StoreInst *SI) {
  if(SI->getOperand(0)->getNumUses() == 1)
    if(isa<LoadInst>(SI->getOperand(0)))
      return true;
    else if(isa<ExtractValueInst>(SI->getOperand(0)))
      return true;
  
  return false;
}
bool 
TypeAnalysis::isCopyingStore(InsertValueInst *IVI) {
  if(IVI->getInsertedValueOperand()->getNumUses() == 1) 
    if(isa<LoadInst>(IVI->getInsertedValueOperand()))
      return true;
    else if(isa<ExtractValueInst>(IVI->getInsertedValueOperand()))
      return true;

  return false;
}
void 
TypeAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}
