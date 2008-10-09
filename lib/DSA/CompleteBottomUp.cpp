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
#include "llvm/Module.h"
#include "dsa/DSGraph.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/STLExtras.h"
using namespace llvm;

namespace {
  RegisterPass<CompleteBUDataStructures>
  X("dsa-cbu", "'Complete' Bottom-up Data Structure Analysis");
  STATISTIC (NumCBUInlines, "Number of graphs inlined");
}

char CompleteBUDataStructures::ID;

// run - Calculate the bottom up data structure graphs for each function in the
// program.
//
bool CompleteBUDataStructures::runOnModule(Module &M) {
  init(&getAnalysis<BUDataStructures>(), false, true);

  std::vector<DSGraph*> Stack;
  hash_map<DSGraph*, unsigned> ValMap;
  unsigned NextID = 1;

  Function *MainFunc = M.getFunction("main");
  if (MainFunc) {
    if (!MainFunc->isDeclaration())
      calculateSCCGraphs(getOrCreateGraph(MainFunc), Stack, NextID, ValMap);
  } else {
    DOUT << "CBU-DSA: No 'main' function found!\n";
  }

  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && !hasDSGraph(*I)) {
      if (MainFunc) {
        DOUT << "*** CBU: Function unreachable from main: "
             << I->getName() << "\n";
      }
      calculateSCCGraphs(getOrCreateGraph(I), Stack, NextID, ValMap);
    }

  GlobalsGraph->removeTriviallyDeadNodes();


  // Merge the globals variables (not the calls) from the globals graph back
  // into the main function's graph so that the main function contains all of
  // the information about global pools and GV usage in the program.
  if (MainFunc && !MainFunc->isDeclaration()) {
    DSGraph &MainGraph = getOrCreateGraph(MainFunc);
    const DSGraph &GG = *MainGraph.getGlobalsGraph();
    ReachabilityCloner RC(MainGraph, GG,
                          DSGraph::DontCloneCallNodes |
                          DSGraph::DontCloneAuxCallNodes);

    // Clone the global nodes into this graph.
    for (DSScalarMap::global_iterator I = GG.getScalarMap().global_begin(),
           E = GG.getScalarMap().global_end(); I != E; ++I)
      if (isa<GlobalVariable>(*I))
        RC.getClonedNH(GG.getNodeForValue(*I));

    MainGraph.maskIncompleteMarkers();
    MainGraph.markIncompleteNodes(DSGraph::MarkFormalArgs |
                                  DSGraph::IgnoreGlobals);
  }

  return false;
}

unsigned CompleteBUDataStructures::calculateSCCGraphs(DSGraph &FG,
                                                  std::vector<DSGraph*> &Stack,
                                                  unsigned &NextID,
                                         hash_map<DSGraph*, unsigned> &ValMap) {
  assert(!ValMap.count(&FG) && "Shouldn't revisit functions!");
  unsigned Min = NextID++, MyID = Min;
  ValMap[&FG] = Min;
  Stack.push_back(&FG);

  // The edges out of the current node are the call site targets...
  for (DSGraph::fc_iterator CI = FG.fc_begin(), CE = FG.fc_end();
       CI != CE; ++CI) {
    Instruction *Call = CI->getCallSite().getInstruction();

    // Loop over all of the actually called functions...
    for (callee_iterator I = callee_begin(Call), E = callee_end(Call); 
         I != E ; ++I) {
      if (!(*I)->isDeclaration()) {
        DSGraph &Callee = getOrCreateGraph(*I);
        unsigned M;
        // Have we visited the destination function yet?
        hash_map<DSGraph*, unsigned>::iterator It = ValMap.find(&Callee);
        if (It == ValMap.end())  // No, visit it now.
          M = calculateSCCGraphs(Callee, Stack, NextID, ValMap);
        else                    // Yes, get it's number.
          M = It->second;
        if (M < Min) Min = M;
      }
    }
  }

  assert(ValMap[&FG] == MyID && "SCC construction assumption wrong!");
  if (Min != MyID)
    return Min;         // This is part of a larger SCC!

  // If this is a new SCC, process it now.
  bool IsMultiNodeSCC = false;
  while (Stack.back() != &FG) {
    DSGraph *NG = Stack.back();
    ValMap[NG] = ~0U;

    FG.cloneInto(*NG);

    // Update the DSInfo map and delete the old graph...
    for (DSGraph::retnodes_iterator I = NG->retnodes_begin();
         I != NG->retnodes_end(); ++I)
      setDSGraph(*I->first, &FG);

    // Remove NG from the ValMap since the pointer may get recycled.
    ValMap.erase(NG);
    delete NG;

    Stack.pop_back();
    IsMultiNodeSCC = true;
  }

  // Clean up the graph before we start inlining a bunch again...
  if (IsMultiNodeSCC)
    FG.removeTriviallyDeadNodes();

  Stack.pop_back();
  processGraph(FG);
  ValMap[&FG] = ~0U;
  return MyID;
}


/// processGraph - Process the BU graphs for the program in bottom-up order on
/// the SCC of the __ACTUAL__ call graph.  This builds "complete" BU graphs.
void CompleteBUDataStructures::processGraph(DSGraph &G) {
  hash_set<Instruction*> calls;

  // The edges out of the current node are the call site targets...
  unsigned i = 0;
  for (DSGraph::fc_iterator CI = G.fc_begin(), CE = G.fc_end(); CI != CE;
       ++CI, ++i) {
    const DSCallSite &CS = *CI;
    Instruction *TheCall = CS.getCallSite().getInstruction();

    assert(calls.insert(TheCall).second &&
           "Call instruction occurs multiple times in graph??");

    // Fast path for noop calls.  Note that we don't care about merging globals
    // in the callee with nodes in the caller here.
    if (CS.getRetVal().isNull() && CS.getNumPtrArgs() == 0)
      continue;

    // Loop over all of the potentially called functions...
    // Inline direct calls as well as indirect calls because the direct
    // callee may have indirect callees and so may have changed.
    //
    unsigned TNum = 0, Num = 0;
    DEBUG(Num = std::distance(callee_begin(TheCall), callee_end(TheCall)));
    for (callee_iterator I = callee_begin(TheCall), E = callee_end(TheCall);
         I != E; ++I, ++TNum) {
      const Function *CalleeFunc = *I;
      if (!CalleeFunc->isDeclaration()) {
        // Merge the callee's graph into this graph.  This works for normal
        // calls or for self recursion within an SCC.
        DSGraph &GI = getOrCreateGraph(CalleeFunc);
        ++NumCBUInlines;
        G.mergeInGraph(CS, *CalleeFunc, GI,
                       DSGraph::StripAllocaBit | DSGraph::DontCloneCallNodes |
                       DSGraph::DontCloneAuxCallNodes);
        DOUT << "    Inlining graph [" << i << "/"
             << G.getFunctionCalls().size()-1
             << ":" << TNum << "/" << Num-1 << "] for "
             << CalleeFunc->getName() << "["
             << GI.getGraphSize() << "+" << GI.getAuxFunctionCalls().size()
             << "] into '" /*<< G.getFunctionNames()*/ << "' ["
             << G.getGraphSize() << "+" << G.getAuxFunctionCalls().size()
             << "]\n";
      }
    }
  }

  // Recompute the Incomplete markers
  G.maskIncompleteMarkers();
  G.markIncompleteNodes(DSGraph::MarkFormalArgs);

  // Delete dead nodes.  Treat globals that are unreachable but that can
  // reach live nodes as live.
  G.removeDeadNodes(DSGraph::KeepUnreachableGlobals);
}
