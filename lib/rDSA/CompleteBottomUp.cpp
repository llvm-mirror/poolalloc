//===- CompleteBottomUp.cpp - Complete Bottom-Up Data Structure Graphs ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the exact same as the bottom-up graphs, but we use take a completed
// call graph and inline all indirect callees into their callers graphs, making
// the result more useful for things like pool allocation.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "dsa-cbu"
#include "rdsa/DataStructure.h"
#include "rdsa/DSGraph.h"
#include "llvm/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

namespace {
  RegisterPass<CompleteBUDataStructures>
  X("dsa-cbu", "'Complete' Bottom-up Data Structure Analysis");
}

char CompleteBUDataStructures::ID;

// run - Calculate the bottom up data structure graphs for each function in the
// program.
//
bool CompleteBUDataStructures::runOnModule(Module &M) {
  init(&getAnalysis<BUDataStructures>(), false, true, false, true);

  buildIndirectFunctionSets(M);
  formGlobalECs();

  return runOnModuleInternal(M);
}

void CompleteBUDataStructures::buildIndirectFunctionSets(Module &M) {
  // Loop over all of the indirect calls in the program.  If a call site can
  // call multiple different functions, we need to unify all of the callees into
  // the same equivalence class.
  std::vector<const Instruction*> keys;
  callee.get_keys(back_inserter(keys));

  //mege nodes in the global graph for these functions
  for (std::vector<const Instruction*>::iterator ii = keys.begin(), ee = keys.end();
       ii != ee; ++ii) {
    if (*ii) {
      calleeTy::iterator csi = callee.begin(*ii), cse = callee.end(*ii); 
      if (csi != cse) ++csi;
      DSGraph* G = getOrFetchDSGraph((*ii)->getParent()->getParent());
      for ( ; csi != cse; ++csi) {
        G->getNodeForValue(*csi).mergeWith(G->getNodeForValue((*ii)->getOperand(0)));
        G->getNodeForValue((*ii)->getOperand(0)).getNode()->NodeType.setGlobalNode();
        G->getNodeForValue((*ii)->getOperand(0)).getNode()->addGlobal(*csi);
      }
    }
  }
}
