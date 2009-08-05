//===- EquivClassGraphs.cpp - Merge equiv-class graphs & inline bottom-up -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass is the same as the complete bottom-up graphs, but
// with functions partitioned into equivalence classes and a single merged
// DS graph for all functions in an equivalence class.  After this merging,
// graphs are inlined bottom-up on the SCCs of the final (CBU) call graph.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "ECGraphs"
#include "rdsa/DataStructure.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "rdsa/DSGraph.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/STLExtras.h"
using namespace llvm;

namespace {
  RegisterPass<EquivBUDataStructures> X("dsa-eq",
                    "Equivalence-class Bottom-up Data Structure Analysis");
}

char EquivBUDataStructures::ID = 0;

// runOnModule - Calculate the bottom up data structure graphs for each function
// in the program.
//
bool EquivBUDataStructures::runOnModule(Module &M) {
  init(&getAnalysis<CompleteBUDataStructures>(), false, true, false, true);

  //update the EQ class from indirect calls
  buildIndirectFunctionSets(M);

  mergeGraphsByGlobalECs();

  return runOnModuleInternal(M);
}


// Merge all graphs that are in the same equivalence class
// the ensures things like poolalloc only deal with one graph for a 
// call site
void EquivBUDataStructures::mergeGraphsByGlobalECs() {
  // Merge the graphs for each equivalence class.
  //
  for (EquivalenceClasses<const GlobalValue*>::iterator EQSI = GlobalECs.begin(), 
         EQSE = GlobalECs.end(); EQSI != EQSE; ++EQSI) {
    if (!EQSI->isLeader()) continue;
    DSGraph* BaseGraph = 0;
    std::vector<DSNodeHandle> Args;
    for (EquivalenceClasses<const GlobalValue*>::member_iterator MI = GlobalECs.member_begin(EQSI);
         MI != GlobalECs.member_end(); ++MI) {
      if (const Function* F = dyn_cast<Function>(*MI)) {
        if (!BaseGraph) {
          BaseGraph = getOrFetchDSGraph(F);
          BaseGraph->getFunctionArgumentsForCall(F, Args);
        } else if (BaseGraph->containsFunction(F)) {
          //already merged
        } else {
          std::vector<DSNodeHandle> NextArgs;
          BaseGraph->cloneInto(getOrFetchDSGraph(F));
          BaseGraph->getFunctionArgumentsForCall(F, NextArgs);
          unsigned i = 0, e = Args.size();
          for (; i != e; ++i) {
            if (i == NextArgs.size()) break;
            Args[i].mergeWith(NextArgs[i]);
          }
          for (e = NextArgs.size(); i != e; ++i)
            Args.push_back(NextArgs[i]);
          setDSGraph(F, BaseGraph);
        }       
      }
    }
  }
}

