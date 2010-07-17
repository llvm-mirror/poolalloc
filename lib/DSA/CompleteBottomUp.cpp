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

#ifndef NDEBUG
    // --Verify that for every callee of an indirect function call
    //   we have an entry in the GlobalsGraph
    bool isIndirect = ((*ii).getCalledFunction() == NULL);

    if (isIndirect) {
      DSCallGraph::callee_iterator csii = callgraph.callee_begin(*ii),
                                   csee = callgraph.callee_end(*ii);

      for (; csii != csee; ++csii) {
        // Declarations don't have to have entries
        if(!(*csii)->isDeclaration())
          assert(SM.count(*csii) && "Indirect function callee not in globals?");
      }

    }
#endif
    // FIXME: Given the above is a valid assertion, we could probably replace
    // this code with something that *assumes* we have entries.  However because
    // I'm not convinced that we can just *skip* direct calls in this function
    // this code is careful to handle callees not existing in the globals graph
    // In other words what we have here should be correct, but might be overkill
    // that we can trim down later as needed.

    DSCallGraph::callee_iterator csi = callgraph.callee_begin(*ii),
            cse = callgraph.callee_end(*ii);

    // Grab the first callee we have an entry for...
    while (csi != cse && !SM.count(*csi))
      ++csi;

    // If we have no entries, we're done.
    if (csi == cse) break;

    DSNodeHandle& SrcNH = SM.find(*csi)->second;

    // Merge the rest of the callees (that we have entries for) together
    // with the first one.
    for (; csi != cse; ++csi) {
      if (SM.count(*csi))
        SrcNH.mergeWith(SM.find(*csi)->second);
    }
  }
}
