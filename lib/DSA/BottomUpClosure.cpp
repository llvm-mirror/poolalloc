//===- BottomUpClosure.cpp - Compute bottom-up interprocedural closure ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the BUDataStructures class, which represents the
// Bottom-Up Interprocedural closure of the data structure graph over the
// program.  This is useful for applications like pool allocation, but **not**
// applications like alias analysis.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "dsa-bu"
#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "llvm/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

namespace {
  STATISTIC (MaxSCC, "Maximum SCC Size in Call Graph");
  STATISTIC (NumInlines, "Number of graphs inlined");
  STATISTIC (NumCallEdges, "Number of 'actual' call edges");
  STATISTIC (NumSCCMerges, "Number of SCC merges");

  RegisterPass<BUDataStructures>
  X("dsa-bu", "Bottom-up Data Structure Analysis");
}

char BUDataStructures::ID;

// run - Calculate the bottom up data structure graphs for each function in the
// program.
//
bool BUDataStructures::runOnModule(Module &M) {
  init(&getAnalysis<StdLibDataStructures>(), false, true, false, false );
  EP = &getAnalysis<EntryPointAnalysis>();

  return runOnModuleInternal(M);
}

// BU:
// Construct the callgraph from the local graphs
// Find SCCs
// inline bottum up
//
// We must split these out (they were merged in PLDI07) to handle multiple
// entry-points correctly.  As a bonus, we can be more aggressive at propagating
// information upwards, as long as we don't remove unresolved call sites.
bool BUDataStructures::runOnModuleInternal(Module& M) {
  //Find SCCs and make SCC call graph
  DSSCCGraph DSG(callgraph);

  errs() << "DSNode: " << sizeof(DSNode) << "\nDSCallSite: "
          << sizeof(DSCallSite) << "\n";

//  DSG.dump();

  //merge SCCs
  mergeSCCs(DSG);

  //Post order traversal:
  {
    //errs() << *DSG.knownRoots.begin() << " -> " << *DSG.knownRoots.rbegin() << "\n";
    svset<unsigned> marked;
    for (svset<unsigned>::const_iterator ii = DSG.knownRoots.begin(),
         ee = DSG.knownRoots.end(); ii != ee; ++ii) {
      //errs() << *ii << " ";
      DSGraph* G = postOrder(DSG, *ii, marked);
      CloneAuxIntoGlobal(G);
    }
  }


  return false;

  std::vector<const Function*> EntryPoints;
  EP = &getAnalysis<EntryPointAnalysis>();
  EP->findEntryPoints(M, EntryPoints);

  #if 0

  std::vector<const Function*> Stack;
  std::map<const Function*, unsigned> ValMap;
  unsigned NextID = 1;

  std::vector<const Function*> EntryPoints;
  EP = &getAnalysis<EntryPointAnalysis>();
  EP->findEntryPoints(M, EntryPoints);

  for (std::vector<const Function*>::iterator ii = EntryPoints.begin(),
          ee = EntryPoints.end(); ii != ee; ++ii)
    if (!hasDSGraph(**ii)) {
      errs() << debugname << ": Main Function: " << (*ii)->getName() << "\n";
      calculateGraphs(*ii, Stack, NextID, ValMap);
      //CloneAuxIntoGlobal(getDSGraph(**ii));
    }

  errs() << "done main Funcs\n";

  // Calculate the graphs for any functions that are unreachable from main...
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && !hasDSGraph(*I)) {
      //DEBUG(
            if (EntryPoints.size())
            errs() << debugname << ": Function unreachable from main: "
            << I->getName() << "\n";
      //);
      calculateGraphs(I, Stack, NextID, ValMap); // Calculate all graphs.
      //CloneAuxIntoGlobal(getDSGraph(*I));
    }

  errs() << "done unreachable Funcs\n";
  #endif

  // If we computed any temporary indcallgraphs, free them now.
  for (std::map<std::vector<const Function*>,
         std::pair<DSGraph*, std::vector<DSNodeHandle> > >::iterator I =
         IndCallGraphMap.begin(), E = IndCallGraphMap.end(); I != E; ++I) {
    I->second.second.clear();  // Drop arg refs into the graph.
    delete I->second.first;
  }
  IndCallGraphMap.clear();

  // At the end of the bottom-up pass, the globals graph becomes complete.
  // FIXME: This is not the right way to do this, but it is sorta better than
  // nothing!  In particular, externally visible globals and unresolvable call
  // nodes at the end of the BU phase should make things that they point to
  // incomplete in the globals graph.
  //

  //finalizeGlobals();

  GlobalsGraph->removeTriviallyDeadNodes();
  GlobalsGraph->maskIncompleteMarkers();

  // Mark external globals incomplete.
  GlobalsGraph->markIncompleteNodes(DSGraph::IgnoreGlobals);

  formGlobalECs();

  // Merge the globals variables (not the calls) from the globals graph back
  // into the main function's graph so that the main function contains all of
  // the information about global pools and GV usage in the program.
  for (std::vector<const Function*>::iterator ii = EntryPoints.begin(),
       ee = EntryPoints.end(); ii != ee; ++ii) {
    DSGraph* MainGraph = getOrCreateGraph(*ii);
    cloneGlobalsInto(MainGraph);

    MainGraph->maskIncompleteMarkers();
    MainGraph->markIncompleteNodes(DSGraph::MarkFormalArgs |
                                   DSGraph::IgnoreGlobals);
  }

  NumCallEdges += callgraph.size();

  return false;
}

void BUDataStructures::mergeSCCs(DSSCCGraph& DSG) {

  for (std::map<unsigned, DSCallGraph::FuncSet>::iterator ii = DSG.SCCs.begin(),
       ee = DSG.SCCs.end(); ii != ee; ++ii) {

    DSGraph* SCCGraph = 0;
    unsigned SCCSize = 0;
    for (DSCallGraph::FuncSet::iterator Fi = ii->second.begin(),
         Fe = ii->second.end(); Fi != Fe; ++Fi) {
      const Function* F = *Fi;
      if (F->isDeclaration()) continue;
      ++SCCSize;
      DSGraph* NFG = getOrCreateGraph(F);
      if (!SCCGraph) {
        SCCGraph = NFG;
      }
      if (NFG != SCCGraph) {
        ++NumSCCMerges;
        // Update the Function -> DSG map.
        for (DSGraph::retnodes_iterator I = NFG->retnodes_begin(),
             E = NFG->retnodes_end(); I != E; ++I)
          setDSGraph(*I->first, SCCGraph);

        SCCGraph->spliceFrom(NFG);
        delete NFG;
      }
    }
    if (MaxSCC < SCCSize) MaxSCC = SCCSize;
  }
}

DSGraph* BUDataStructures::postOrder(DSSCCGraph& DSG, unsigned scc,
                                     svset<unsigned>& marked) {
  DSGraph* G = getDSGraph(**DSG.SCCs[scc].begin());
  if (marked.count(scc)) return G;

  for(svset<unsigned>::iterator ii = DSG.SCCCallees[scc].begin(),
          ee = DSG.SCCCallees[scc].end(); ii != ee; ++ii)
    postOrder(DSG, *ii, marked);

  marked.insert(scc);
  calculateGraph(G, DSG);
  return G;
}

void BUDataStructures::finalizeGlobals(void) {
  // Any unresolved call can be removed (resolved) if it does not contain
  // external functions and it is not reachable from any call that does
  // contain external functions
  std::set<DSCallSite> GoodCalls, BadCalls;
  for (DSGraph::afc_iterator ii = GlobalsGraph->afc_begin(), 
         ee = GlobalsGraph->afc_end(); ii != ee; ++ii)
    if (ii->isDirectCall() || ii->getCalleeNode()->isExternFuncNode())
      BadCalls.insert(*ii);
    else
      GoodCalls.insert(*ii);
  DenseSet<const DSNode*> reachable;
  for (std::set<DSCallSite>::iterator ii = BadCalls.begin(),
         ee = BadCalls.end(); ii != ee; ++ii) {
    ii->getRetVal().getNode()->markReachableNodes(reachable);
    for (unsigned x = 0; x < ii->getNumPtrArgs(); ++x)
      ii->getPtrArg(x).getNode()->markReachableNodes(reachable);
  }
  for (std::set<DSCallSite>::iterator ii = GoodCalls.begin(),
         ee = GoodCalls.end(); ii != ee; ++ii)
    if (reachable.count(ii->getCalleeNode()))
      GlobalsGraph->getAuxFunctionCalls()
        .erase(std::find(GlobalsGraph->getAuxFunctionCalls().begin(),
                         GlobalsGraph->getAuxFunctionCalls().end(),
                         *ii));
  GlobalsGraph->getScalarMap().clear_scalars();
}


void BUDataStructures::CloneAuxIntoGlobal(DSGraph* G) {
  DSGraph* GG = G->getGlobalsGraph();
  ReachabilityCloner RC(GG, G, 0);

  for(DSGraph::afc_iterator ii = G->afc_begin(), ee = G->afc_end();
      ii != ee; ++ii) {
    //cerr << "Pushing " << ii->getCallSite().getInstruction()->getOperand(0) << "\n";
    //If we can, merge with an existing call site for this instruction
    if (GG->hasNodeForValue(ii->getCallSite().getInstruction()->getOperand(0))) {
      DSGraph::afc_iterator GGii;
      for(GGii = GG->afc_begin(); GGii != GG->afc_end(); ++GGii)
        if (GGii->getCallSite().getInstruction()->getOperand(0) ==
            ii->getCallSite().getInstruction()->getOperand(0))
          break;
      if (GGii != GG->afc_end())
        RC.cloneCallSite(*ii).mergeWith(*GGii);
      else
        GG->addAuxFunctionCall(RC.cloneCallSite(*ii));
    } else {
      GG->addAuxFunctionCall(RC.cloneCallSite(*ii));
    }
  }
}

void BUDataStructures::calculateGraph(DSGraph* Graph, DSSCCGraph& DSG) {
  DEBUG(Graph->AssertGraphOK(); Graph->getGlobalsGraph()->AssertGraphOK());

  // If this graph contains the main function, clone the globals graph into this
  // graph before we inline callees and other fun stuff.
  bool ContainsMain = false;
  DSGraph::ReturnNodesTy &ReturnNodes = Graph->getReturnNodes();

  for (DSGraph::ReturnNodesTy::iterator I = ReturnNodes.begin(),
       E = ReturnNodes.end(); I != E; ++I)
    if (EP->isEntryPoint(I->first)) {
      ContainsMain = true;
      break;
    }

   // If this graph contains main, copy the contents of the globals graph over.
   // Note that this is *required* for correctness.  If a callee contains a use
   // of a global, we have to make sure to link up nodes due to global-argument
   // bindings.
   if (ContainsMain)
     cloneGlobalsInto(Graph);

  // Move our call site list into TempFCs so that inline call sites go into the
  // new call site list and doesn't invalidate our iterators!
  std::list<DSCallSite> TempFCs;
  std::list<DSCallSite> &AuxCallsList = Graph->getAuxFunctionCalls();
  TempFCs.swap(AuxCallsList);

  std::vector<const Function*> CalledFuncs;
  while (!TempFCs.empty()) {
    DEBUG(Graph->AssertGraphOK(); Graph->getGlobalsGraph()->AssertGraphOK());
    
    DSCallSite &CS = *TempFCs.begin();

    CalledFuncs.clear();

    // Fast path for noop calls.  Note that we don't care about merging globals
    // in the callee with nodes in the caller here.
    if (CS.getRetVal().isNull() && CS.getNumPtrArgs() == 0) {
      TempFCs.erase(TempFCs.begin());
      continue;
    }

    std::copy(DSG.oldGraph.callee_begin(CS.getCallSite()),
              DSG.oldGraph.callee_end(CS.getCallSite()),
              std::back_inserter(CalledFuncs));

    std::vector<const Function*>::iterator ErasePoint =
            std::remove_if(CalledFuncs.begin(), CalledFuncs.end(),
                           std::mem_fun(&Function::isDeclaration));
    CalledFuncs.erase(ErasePoint, CalledFuncs.end());

    if (CalledFuncs.empty()) {
      // Remember that we could not resolve this yet!
      AuxCallsList.splice(AuxCallsList.end(), TempFCs, TempFCs.begin());
      continue;
    }

    DSGraph *GI;

    for (unsigned x = 0; x < CalledFuncs.size(); ++x) {
      const Function *Callee = CalledFuncs[x];

      // Get the data structure graph for the called function.
      GI = getDSGraph(*Callee);  // Graph to inline
      DEBUG(GI->AssertGraphOK(); GI->getGlobalsGraph()->AssertGraphOK());
      DEBUG(errs() << "    Inlining graph for " << Callee->getName()
	    << "[" << GI->getGraphSize() << "+"
	    << GI->getAuxFunctionCalls().size() << "] into '"
	    << Graph->getFunctionNames() << "' [" << Graph->getGraphSize() <<"+"
	    << Graph->getAuxFunctionCalls().size() << "]\n");
      Graph->mergeInGraph(CS, *Callee, *GI,
                          DSGraph::StripAllocaBit|DSGraph::DontCloneCallNodes);
      ++NumInlines;
      DEBUG(Graph->AssertGraphOK(););
    }
    TempFCs.erase(TempFCs.begin());
  }

  // Recompute the Incomplete markers
  Graph->maskIncompleteMarkers();
  Graph->markIncompleteNodes(DSGraph::MarkFormalArgs);

  // Delete dead nodes.  Treat globals that are unreachable but that can
  // reach live nodes as live.
  Graph->removeDeadNodes(DSGraph::KeepUnreachableGlobals);

//  cloneIntoGlobals(Graph);
  //Graph.writeGraphToFile(cerr, "bu_" + F.getName());
}

//For Entry Points
void BUDataStructures::cloneGlobalsInto(DSGraph* Graph) {
  // If this graph contains main, copy the contents of the globals graph over.
  // Note that this is *required* for correctness.  If a callee contains a use
  // of a global, we have to make sure to link up nodes due to global-argument
  // bindings.
  const DSGraph* GG = Graph->getGlobalsGraph();
  ReachabilityCloner RC(Graph, GG,
                        DSGraph::DontCloneCallNodes |
                        DSGraph::DontCloneAuxCallNodes);

  // Clone the global nodes into this graph.
  for (DSScalarMap::global_iterator I = Graph->getScalarMap().global_begin(),
       E = Graph->getScalarMap().global_end(); I != E; ++I)
    if (isa<GlobalVariable > (*I))
      RC.getClonedNH(GG->getNodeForValue(*I));
}

//For all graphs
void BUDataStructures::cloneIntoGlobals(DSGraph* Graph) {
  // When this graph is finalized, clone the globals in the graph into the
  // globals graph to make sure it has everything, from all graphs.
  DSScalarMap &MainSM = Graph->getScalarMap();
  ReachabilityCloner RC(GlobalsGraph, Graph,
                        DSGraph::DontCloneCallNodes |
                        DSGraph::DontCloneAuxCallNodes |
                        DSGraph::StripAllocaBit);

  // Clone everything reachable from globals in the function graph into the
  // globals graph.
  for (DSScalarMap::global_iterator I = MainSM.global_begin(),
         E = MainSM.global_end(); I != E; ++I)
    RC.getClonedNH(MainSM[*I]);
}

