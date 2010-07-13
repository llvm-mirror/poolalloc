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
#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
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

  DSGraph* G = getGlobalsGraph();
  DSGraph::ScalarMapTy& SM = G->getScalarMap();

  //mege nodes in the global graph for these functions
  for (DSCallGraph::callee_key_iterator ii = callgraph.key_begin(),
       ee = callgraph.key_end(); ii != ee; ++ii) {
    DSCallGraph::callee_iterator csi = callgraph.callee_begin(*ii),
            cse = callgraph.callee_end(*ii);
    if (csi != cse && SM.find(*csi) != SM.end()) {
      assert((SM.find(*csi) != SM.end()) && "Function not in Global graph?");
      DSNodeHandle& SrcNH = SM.find(*csi)->second;
      ++csi;
      for (; csi != cse; ++csi) {
        assert((SM.find(*csi) != SM.end()) && "Function not in Global graph?");
        SrcNH.mergeWith(SM.find(*csi)->second);
      }
    }
  }
}
