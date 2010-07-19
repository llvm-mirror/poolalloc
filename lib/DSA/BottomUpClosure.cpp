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
  STATISTIC (NumIndResolved, "Number of resolved IndCalls");
  STATISTIC (NumIndUnresolved, "Number of unresolved IndCalls");
  STATISTIC (NumEmptyCalls, "Number of calls we know nothing about");

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
#if 0
  llvm::errs() << "BU is currently being worked in in very invasive ways.\n"
          << "It is probably broken right now\n";
#endif

  //
  // Put the callgraph into canonical form by finding SCCs.
  //
  callgraph.buildSCCs();
  callgraph.buildRoots();

  //
  // Merge the DSGraphs of functions belonging to an SCC.
  //
  mergeSCCs();

  //
  // Do a post-order traversal of the SCC callgraph and do bottom-up inlining.
  //
  {
    //errs() << *DSG.knownRoots.begin() << " -> " << *DSG.knownRoots.rbegin() << "\n";
    svset<const Function*> marked;
    for (DSCallGraph::root_iterator ii = callgraph.root_begin(),
         ee = callgraph.root_end(); ii != ee; ++ii) {
      //errs() << (*ii)->getName() << "\n";

      //
      // Do bottom-up inlining of the function.
      //
      DSGraph* G = postOrder(*ii, marked);

      //
      // Update the list of unresolved indirect function call sites in the
      // globals graph with the new information learned about the current
      // function.
      //
      CloneAuxIntoGlobal(G);
    }
  }


  std::vector<const Function*> EntryPoints;
  EP = &getAnalysis<EntryPointAnalysis>();
  EP->findEntryPoints(M, EntryPoints);

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

  //
  // Create equivalence classes for aliasing globals so that we only need to
  // record one global per DSNode.
  //
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

//
// Method: mergeSCCs()
//
// Description:
//  Create a single DSGraph for every Strongly Connected Component (SCC) in the
//  callgraph.  This is done by merging the DSGraphs of every function within
//  each SCC.
//
void BUDataStructures::mergeSCCs() {

  for (DSCallGraph::flat_key_iterator ii = callgraph.flat_key_begin(),
       ee = callgraph.flat_key_end(); ii != ee; ++ii) {
    //
    // External functions form their own singleton SCC.
    //
    if ((*ii)->isDeclaration()) continue;

    DSGraph* SCCGraph = getOrCreateGraph(*ii);
    unsigned SCCSize = 1;
    callgraph.assertSCCRoot(*ii);

    for (DSCallGraph::scc_iterator Fi = callgraph.scc_begin(*ii),
         Fe = callgraph.scc_end(*ii); Fi != Fe; ++Fi) {
      const Function* F = *Fi;
      if (F->isDeclaration()) continue;
      if (F == *ii) continue;
      ++SCCSize;
      DSGraph* NFG = getOrCreateGraph(F);
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

//
// Method: postOrder()
//
// Description:
//  Process the SCCs of the callgraph in post order.  When we process a
//  function, we inline the DSGraphs of its callees into the function's own
//  DSGraph, thereby doing the "bottom-up" pass that makes BU so famous.
//
// Inputs:
//  F      - The function in the SCC to process.  Note that its children in the 
//           callgraph will be processed first through a recursive call.
//  marked - A reference to a set containing all values processed by
//           previous invocations (this method is recursive).
//
// Outputs:
//  marked - A set containing pointers to functions that have already been
//           processed.
//
// Return value:
//  The DSGraph of the function after it has been processed is returned.
//
DSGraph*
BUDataStructures::postOrder(const Function* F, svset<const Function*>& marked) {
  //
  // If we have already processed this function before, do not process it
  // again.
  //
  callgraph.assertSCCRoot(F);
  DSGraph* G = getDSGraph(*F);
  if (marked.count(F)) return G;

  //
  // Find the set of callees to process.
  //
  // For this operation, we do not want to use the call graph.  Instead, we
  // want to consult the DSGraph and see which call sites have not yet been
  // resolved.  This is because we may learn about more call sites after doing
  // one pass of bottom-up inlining, and so we don't want to reprocess the
  // callees that were previously processed in an earlier BU phase.
  //
  for (DSCallGraph::flat_iterator ii = callgraph.flat_callee_begin(F),
          ee = callgraph.flat_callee_end(F); ii != ee; ++ii) {
    callgraph.assertSCCRoot(*ii);
    assert (*ii != F && "Simple loop in callgraph");
    if (!(*ii)->isDeclaration())
      postOrder(*ii, marked);
  }

  //
  // Record that we are about to process the given function.
  //
  marked.insert(F);

  //
  // Inline the graphs of callees into this function's callgraph.
  //
  calculateGraph(G);

  //
  // Now that we have new information merged into the function's DSGraph,
  // update the call graph using this new information.
  //
  G->buildCallGraph(callgraph);

  //
  // Return the DSGraph associated with this function.
  //
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
    ii->getVAVal().getNode()->markReachableNodes(reachable);
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


//
// Method: CloneAuxIntoGlobal()
//
// Description:
//  This method takes the specified graph and processes each unresolved call
//  site (a call site for which all targets are not yet known).  For each
//  unresolved call site, it adds it to the globals graph and merges
//  information about the call site if the globals graph already had the call
//  site in its own list of unresolved call sites.
//
void BUDataStructures::CloneAuxIntoGlobal(DSGraph* G) {
  DSGraph* GG = G->getGlobalsGraph();
  ReachabilityCloner RC(GG, G, 0);

  //
  // Scan through all unresolved call sites (call sites for which we do not yet
  // know all of the callees) in the specified graph and see if the globals
  // graph also has an unresolved call site for the same function pointer.  If
  // it does, merge them together; otherwise, just bring the unresolved call
  // site into the global graph's set of unresolved call sites.
  //
  for (DSGraph::afc_iterator ii = G->afc_begin(), ee = G->afc_end();
       ii != ee;
       ++ii) {
#if 0
    cerr << "Pushing " << ii->getCallSite().getInstruction()->getOperand(0) << "\n";
#endif

    //
    // If we can, merge with an existing call site for this instruction.
    //
    if (GG->hasNodeForValue(ii->getCallSite().getCalledValue())) {
      //
      // Determine whether the globals graph knows about this call site and
      // consider it to be unresolved.
      //
      DSGraph::afc_iterator GGii;
      for(GGii = GG->afc_begin(); GGii != GG->afc_end(); ++GGii)
        if (GGii->getCallSite().getCalledValue() ==
            ii->getCallSite().getCalledValue())
          break;

      //
      // If the globals graph knows about the call site, merge it in.
      // Otherwise, just record it as an unresolved call site.
      //
      if (GGii != GG->afc_end())
        RC.cloneCallSite(*ii).mergeWith(*GGii);
      else
        GG->addAuxFunctionCall(RC.cloneCallSite(*ii));
    } else {
      GG->addAuxFunctionCall(RC.cloneCallSite(*ii));
    }
  }
}

//
// Description:
//  Inline all graphs in the callgraph and remove callsites that are completely
//  dealt with
//
void BUDataStructures::calculateGraph(DSGraph* Graph) {
  DEBUG(Graph->AssertGraphOK(); Graph->getGlobalsGraph()->AssertGraphOK());

  // Move our call site list into TempFCs so that inline call sites go into the
  // new call site list and doesn't invalidate our iterators!
  std::list<DSCallSite> TempFCs;
  std::list<DSCallSite> &AuxCallsList = Graph->getAuxFunctionCalls();
  TempFCs.swap(AuxCallsList);

  while (!TempFCs.empty()) {
    DEBUG(Graph->AssertGraphOK(); Graph->getGlobalsGraph()->AssertGraphOK());
    
    DSCallSite &CS = *TempFCs.begin();

    // Fast path for noop calls.  Note that we don't care about merging globals
    // in the callee with nodes in the caller here.
    if (CS.getRetVal().isNull() && CS.getNumPtrArgs() == 0 && !CS.isVarArg()) {
      TempFCs.erase(TempFCs.begin());
      continue;
    }

    //
    // Find all called functions called by this call site.  Remove from the
    // list all calls to external functions (functions with no bodies).
    //
    std::vector<const Function*> CalledFuncs;
    {
      // Get the callees from the callgraph
      std::copy(callgraph.callee_begin(CS.getCallSite()),
                callgraph.callee_end(CS.getCallSite()),
                std::back_inserter(CalledFuncs));

      // Remove calls to external functions
      std::vector<const Function*>::iterator ErasePoint =
              std::remove_if(CalledFuncs.begin(), CalledFuncs.end(),
                             std::mem_fun(&Function::isDeclaration));
      CalledFuncs.erase(ErasePoint, CalledFuncs.end());
    }

    if (CalledFuncs.empty()) {
      ++NumEmptyCalls;
      // Remember that we could not resolve this yet!
      AuxCallsList.splice(AuxCallsList.end(), TempFCs, TempFCs.begin());
      continue;
    }

    // Direct calls are always inlined and removed from AuxCalls
    // Indirect calls are removed if the callnode is complete and the callnode's
    // functions set is a subset of the Calls from the callgraph
    // We only inline from the callgraph (which is immutable during this phase
    // of bu) so as to not introduce SCCs and still be able to inline
    // aggressively
    bool eraseCS = true;
    if (CS.isIndirectCall()) {
      eraseCS = false;
      if (CS.getCalleeNode()->isCompleteNode()) {
        std::vector<const Function*> NodeCallees;
        CS.getCalleeNode()->addFullFunctionList(NodeCallees);
        std::vector<const Function*>::iterator ErasePoint =
                std::remove_if(NodeCallees.begin(), NodeCallees.end(),
                               std::mem_fun(&Function::isDeclaration));
        NodeCallees.erase(ErasePoint, NodeCallees.end());
        std::sort(CalledFuncs.begin(), CalledFuncs.end());
        std::sort(NodeCallees.begin(), NodeCallees.end());
        eraseCS = std::includes(CalledFuncs.begin(), CalledFuncs.end(),
                                NodeCallees.begin(), NodeCallees.end());
      }
      if (eraseCS) ++NumIndResolved;
      else ++NumIndUnresolved;
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

      //
      // Merge in the DSGraph of the called function.
      //
      // TODO:
      //  Why are the strip alloca bit and don't clone call nodes bit set?
      //
      //  I believe the answer is on page 6 of the PLDI paper on DSA.  The
      //  idea is that stack objects are invalid if they escape.
      //
      Graph->mergeInGraph(CS, *Callee, *GI,
                          DSGraph::StripAllocaBit|DSGraph::DontCloneCallNodes);
      ++NumInlines;
      DEBUG(Graph->AssertGraphOK(););
    }
    if (eraseCS)
      TempFCs.erase(TempFCs.begin());
    else
      AuxCallsList.splice(AuxCallsList.end(), TempFCs, TempFCs.begin());
  }

  // Recompute the Incomplete markers
  Graph->maskIncompleteMarkers();
  Graph->markIncompleteNodes(DSGraph::MarkFormalArgs);

  // Delete dead nodes.  Treat globals that are unreachable but that can
  // reach live nodes as live.
  Graph->removeDeadNodes(DSGraph::KeepUnreachableGlobals);

  cloneIntoGlobals(Graph);
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

