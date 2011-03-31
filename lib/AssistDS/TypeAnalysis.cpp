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

