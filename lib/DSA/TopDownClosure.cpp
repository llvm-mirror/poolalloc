//===- TopDownClosure.cpp - Compute the top-down interprocedure closure ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the TDDataStructures class, which represents the
// Top-down Interprocedural closure of the data structure graph over the
// program.  This is useful (but not strictly necessary?) for applications
// like pointer analysis.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "td_dsa"

#include "dsa/DataStructure.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "dsa/DSGraph.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Timer.h"
#include "llvm/ADT/Statistic.h"
using namespace llvm;

#if 0
#define TIME_REGION(VARNAME, DESC) \
   NamedRegionTimer VARNAME(DESC)
#else
#define TIME_REGION(VARNAME, DESC)
#endif

namespace {
  RegisterPass<TDDataStructures>   // Register the pass
  Y("dsa-td", "Top-down Data Structure Analysis");

  RegisterPass<EQTDDataStructures>   // Register the pass
  Z("dsa-eqtd", "EQ Top-down Data Structure Analysis");

  STATISTIC (NumTDInlines, "Number of graphs inlined");
}

char TDDataStructures::ID;
char EQTDDataStructures::ID;

TDDataStructures::~TDDataStructures() {
  releaseMemory();
}

EQTDDataStructures::~EQTDDataStructures() {
  releaseMemory();
}

void TDDataStructures::markReachableFunctionsExternallyAccessible(DSNode *N,
                                                   svset<DSNode*> &Visited) {
  if (!N || Visited.count(N)) return;
  Visited.insert(N);

  for (DSNode::edge_iterator ii = N->edge_begin(),
          ee = N->edge_end(); ii != ee; ++ii)
    if (!ii->second.isNull()) {
      DSNodeHandle &NH = ii->second;
      DSNode * NN = NH.getNode();
      std::vector<const Function*> Functions;
      NN->addFullFunctionList(Functions);
      ArgsRemainIncomplete.insert(Functions.begin(), Functions.end());
      markReachableFunctionsExternallyAccessible(NN, Visited);
    }
}


// run - Calculate the top down data structure graphs for each function in the
// program.
//
bool TDDataStructures::runOnModule(Module &M) {
  
  init(useEQBU ? &getAnalysis<EquivBUDataStructures>()
       : &getAnalysis<BUDataStructures>(), 
       true, true, true, false);

  // Figure out which functions must not mark their arguments complete because
  // they are accessible outside this compilation unit.  Currently, these
  // arguments are functions which are reachable by global variables in the
  // globals graph.
  const DSScalarMap &GGSM = GlobalsGraph->getScalarMap();
  svset<DSNode*> Visited;
  for (DSScalarMap::global_iterator I=GGSM.global_begin(), E=GGSM.global_end();
       I != E; ++I) {
    DSNode *N = GGSM.find(*I)->second.getNode();
    if (N->isIncompleteNode())
      markReachableFunctionsExternallyAccessible(N, Visited);
  }

  // Loop over unresolved call nodes.  Any functions passed into (but not
  // returned!) from unresolvable call nodes may be invoked outside of the
  // current module.
  for (DSGraph::afc_iterator I = GlobalsGraph->afc_begin(),
         E = GlobalsGraph->afc_end(); I != E; ++I)
    for (unsigned arg = 0, e = I->getNumPtrArgs(); arg != e; ++arg)
      markReachableFunctionsExternallyAccessible(I->getPtrArg(arg).getNode(),
                                                 Visited);
  Visited.clear();

  // Clear Aux of Globals Graph to be refilled in later by post-TD unresolved 
  // functions
  GlobalsGraph->getAuxFunctionCalls().clear();

  // Functions without internal linkage also have unknown incoming arguments!
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && !I->hasInternalLinkage())
      ArgsRemainIncomplete.insert(I);

  // We want to traverse the call graph in reverse post-order.  To do this, we
  // calculate a post-order traversal, then reverse it.
  svset<DSGraph*> VisitedGraph;
  std::vector<DSGraph*> PostOrder;

{TIME_REGION(XXX, "td:Compute postorder");

  // Calculate top-down from main...
  if (Function *F = M.getFunction("main"))
    ComputePostOrder(*F, VisitedGraph, PostOrder);

  // Next calculate the graphs for each unreachable function...
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration())
      ComputePostOrder(*I, VisitedGraph, PostOrder);

  VisitedGraph.clear();   // Release memory!
}

{TIME_REGION(XXX, "td:Inline stuff");

  // Visit each of the graphs in reverse post-order now!
  while (!PostOrder.empty()) {
    InlineCallersIntoGraph(PostOrder.back());
    PostOrder.pop_back();
  }
}

  // Free the IndCallMap.
  while (!IndCallMap.empty()) {
    delete IndCallMap.begin()->second;
    IndCallMap.erase(IndCallMap.begin());
  }

  formGlobalECs();

  ArgsRemainIncomplete.clear();
  GlobalsGraph->removeTriviallyDeadNodes();

  // CBU contains the correct call graph.
  // Restore it, so that subsequent passes and clients can get it.
  restoreCorrectCallGraph();
  return false;
}


void TDDataStructures::ComputePostOrder(const Function &F,
                                        svset<DSGraph*> &Visited,
                                        std::vector<DSGraph*> &PostOrder) {
  if (F.isDeclaration()) return;
  DSGraph* G = getOrCreateGraph(&F);
  if (Visited.count(G)) return;
  Visited.insert(G);

  // Recursively traverse all of the callee graphs.
  for (DSGraph::fc_iterator CI = G->fc_begin(), CE = G->fc_end(); CI != CE; ++CI)
    for (DSCallGraph::callee_iterator I = callgraph.callee_begin(CI->getCallSite()),
           E = callgraph.callee_end(CI->getCallSite()); I != E; ++I)
      ComputePostOrder(**I, Visited, PostOrder);

  PostOrder.push_back(G);
}

/// InlineCallersIntoGraph - Inline all of the callers of the specified DS graph
/// into it, then recompute completeness of nodes in the resultant graph.
void TDDataStructures::InlineCallersIntoGraph(DSGraph* DSG) {
  // Inline caller graphs into this graph.  First step, get the list of call
  // sites that call into this graph.
  std::vector<CallerCallEdge> EdgesFromCaller;
  std::map<DSGraph*, std::vector<CallerCallEdge> >::iterator
    CEI = CallerEdges.find(DSG);
  if (CEI != CallerEdges.end()) {
    std::swap(CEI->second, EdgesFromCaller);
    CallerEdges.erase(CEI);
  }

  // Sort the caller sites to provide a by-caller-graph ordering.
  std::sort(EdgesFromCaller.begin(), EdgesFromCaller.end());


  // Merge information from the globals graph into this graph.  FIXME: This is
  // stupid.  Instead of us cloning information from the GG into this graph,
  // then having RemoveDeadNodes clone it back, we should do all of this as a
  // post-pass over all of the graphs.  We need to take cloning out of
  // removeDeadNodes and gut removeDeadNodes at the same time first though. :(
  {
    DSGraph* GG = DSG->getGlobalsGraph();
    ReachabilityCloner RC(DSG, GG,
                          DSGraph::DontCloneCallNodes |
                          DSGraph::DontCloneAuxCallNodes);
    for (DSScalarMap::global_iterator
           GI = DSG->getScalarMap().global_begin(),
           E = DSG->getScalarMap().global_end(); GI != E; ++GI)
      RC.getClonedNH(GG->getNodeForValue(*GI));
  }

  DEBUG(errs() << "[TD] Inlining callers into '" 
	<< DSG->getFunctionNames() << "'\n");

  // Iteratively inline caller graphs into this graph.
  while (!EdgesFromCaller.empty()) {
    DSGraph* CallerGraph = EdgesFromCaller.back().CallerGraph;

    // Iterate through all of the call sites of this graph, cloning and merging
    // any nodes required by the call.
    ReachabilityCloner RC(DSG, CallerGraph,
                          DSGraph::DontCloneCallNodes |
                          DSGraph::DontCloneAuxCallNodes);

    // Inline all call sites from this caller graph.
    do {
      const DSCallSite &CS = *EdgesFromCaller.back().CS;
      const Function &CF = *EdgesFromCaller.back().CalledFunction;
      DEBUG(errs() << "   [TD] Inlining graph into Fn '" 
	    << CF.getNameStr() << "' from ");
      if (CallerGraph->getReturnNodes().empty()) {
        DEBUG(errs() << "SYNTHESIZED INDIRECT GRAPH");
      } else {
        DEBUG(errs() << "Fn '" << CS.getCallSite().getInstruction()->
	      getParent()->getParent()->getNameStr() << "'");
      }
      DEBUG(errs() << ": " << CF.getFunctionType()->getNumParams() 
	    << " args\n");

      // Get the formal argument and return nodes for the called function and
      // merge them with the cloned subgraph.
      DSCallSite T1 = DSG->getCallSiteForArguments(CF);
      RC.mergeCallSite(T1, CS);
      ++NumTDInlines;

      EdgesFromCaller.pop_back();
    } while (!EdgesFromCaller.empty() &&
             EdgesFromCaller.back().CallerGraph == CallerGraph);
  }


  {
    DSGraph* GG = DSG->getGlobalsGraph();
    ReachabilityCloner RC(GG, DSG,
                          DSGraph::DontCloneCallNodes |
                          DSGraph::DontCloneAuxCallNodes);
    for (DSScalarMap::global_iterator
           GI = DSG->getScalarMap().global_begin(),
           E = DSG->getScalarMap().global_end(); GI != E; ++GI)
      RC.getClonedNH(DSG->getNodeForValue(*GI));
  }

  // Next, now that this graph is finalized, we need to recompute the
  // incompleteness markers for this graph and remove unreachable nodes.
  DSG->maskIncompleteMarkers();

  // If any of the functions has incomplete incoming arguments, don't mark any
  // of them as complete.
  bool HasIncompleteArgs = false;
  for (DSGraph::retnodes_iterator I = DSG->retnodes_begin(),
         E = DSG->retnodes_end(); I != E; ++I)
    if (ArgsRemainIncomplete.count(I->first)) {
      HasIncompleteArgs = true;
      break;
    }

  // Recompute the Incomplete markers.  Depends on whether args are complete
  unsigned Flags
    = HasIncompleteArgs ? DSGraph::MarkFormalArgs : DSGraph::IgnoreFormalArgs;
  Flags |= DSGraph::IgnoreGlobals | DSGraph::MarkVAStart;
  DSG->markIncompleteNodes(Flags);

  //
  // Delete dead nodes.  Treat globals that are unreachable as dead also.
  //
  // FIXME:
  //  Do not delete unreachable globals as the comment describes.  For its
  //  alignment checks on the results of load instructions, SAFECode must be
  //  able to find the DSNode of both the result of the load as well as the
  //  pointer dereferenced by the load.  If we remove unreachable globals, then
  //  if the dereferenced pointer is a global, its DSNode will not reachable
  //  from the local graph's scalar map, and chaos ensues.
  //
  //  So, for now, just remove dead nodes but leave the globals alone.
  //
#if 0
  DSG->removeDeadNodes(DSGraph::RemoveUnreachableGlobals);
#else
  DSG->removeDeadNodes(0);
#endif

  // We are done with computing the current TD Graph!  Finally, before we can
  // finish processing this function, we figure out which functions it calls and
  // records these call graph edges, so that we have them when we process the
  // callee graphs.
  if (DSG->fc_begin() == DSG->fc_end()) return;

  // Loop over all the call sites and all the callees at each call site, and add
  // edges to the CallerEdges structure for each callee.
  for (DSGraph::fc_iterator CI = DSG->fc_begin(), E = DSG->fc_end();
       CI != E; ++CI) {

    // Handle direct calls efficiently.
    if (CI->isDirectCall()) {
      if (!CI->getCalleeFunc()->isDeclaration() &&
          !DSG->getReturnNodes().count(CI->getCalleeFunc()))
        CallerEdges[getOrCreateGraph(CI->getCalleeFunc())]
          .push_back(CallerCallEdge(DSG, &*CI, CI->getCalleeFunc()));
      continue;
    }

    // For each function in the invoked function list at this call site...
    DSCallGraph::callee_iterator IPI = callgraph.callee_begin(CI->getCallSite()),
            IPE = callgraph.callee_end(CI->getCallSite());

    // Skip over all calls to this graph (SCC calls).
    // Note that Functions that are just declarations are their own SCC
    while (IPI != IPE && !(*IPI)->isDeclaration() && getDSGraph(**IPI) == DSG)
      ++IPI;

    // All SCC calls?
    if (IPI == IPE) continue;

    const Function *FirstCallee = *IPI;
    ++IPI;

    // Skip over more SCC calls.
    while (IPI != IPE && !(*IPI)->isDeclaration() && getDSGraph(**IPI) == DSG)
      ++IPI;

    // If there is exactly one callee from this call site, remember the edge in
    // CallerEdges.
    if (IPI == IPE) {
      if (!FirstCallee->isDeclaration())
        CallerEdges[getOrCreateGraph(FirstCallee)]
          .push_back(CallerCallEdge(DSG, &*CI, FirstCallee));
      continue;
    }

    // Otherwise, there are multiple callees from this call site, so it must be
    // an indirect call.  Chances are that there will be other call sites with
    // this set of targets.  If so, we don't want to do M*N inlining operations,
    // so we build up a new, private, graph that represents the calls of all
    // calls to this set of functions.
    std::vector<const Function*> Callees;
    for (DSCallGraph::callee_iterator I = callgraph.callee_begin(CI->getCallSite()),
         E = callgraph.callee_end(CI->getCallSite()); I != E; ++I)
      if (!(*I)->isDeclaration())
        Callees.push_back(*I);
    
    // If all of the callees are declarations, there is no need to merge the calls.
    if(Callees.empty())
      continue;
    std::sort(Callees.begin(), Callees.end());

    std::map<std::vector<const Function*>, DSGraph*>::iterator IndCallRecI =
      IndCallMap.lower_bound(Callees);

    DSGraph *IndCallGraph;

    // If we already have this graph, recycle it.
    if (IndCallRecI != IndCallMap.end() && IndCallRecI->first == Callees) {
      DEBUG(errs() << "  [TD] *** Reuse of indcall graph for " << Callees.size()
	    << " callees!\n");
      IndCallGraph = IndCallRecI->second;
    } else {
      // Otherwise, create a new DSGraph to represent this.
      IndCallGraph = new DSGraph(DSG->getGlobalECs(), DSG->getTargetData(), *TypeSS);
      // Make a nullary dummy call site, which will eventually get some content
      // merged into it.  The actual callee function doesn't matter here, so we
      // just pass it something to keep the ctor happy.
      std::vector<DSNodeHandle> ArgDummyVec;
      DSCallSite DummyCS(CI->getCallSite(), DSNodeHandle(), DSNodeHandle(),
                         Callees[0]/*dummy*/, ArgDummyVec);
      IndCallGraph->getFunctionCalls().push_back(DummyCS);

      IndCallRecI = IndCallMap.insert(IndCallRecI,
                                      std::make_pair(Callees, IndCallGraph));

      // Additionally, make sure that each of the callees inlines this graph
      // exactly once.
      DSCallSite *NCS = &IndCallGraph->getFunctionCalls().front();
      for (unsigned i = 0, e = Callees.size(); i != e; ++i) {
        DSGraph* CalleeGraph = getDSGraph(*Callees[i]);
        if (CalleeGraph != DSG)
          CallerEdges[CalleeGraph].push_back(CallerCallEdge(IndCallGraph, NCS,
                                                            Callees[i]));
      }
    }

    // Now that we know which graph to use for this, merge the caller
    // information into the graph, based on information from the call site.
    ReachabilityCloner RC(IndCallGraph, DSG, 0);
    RC.mergeCallSite(IndCallGraph->getFunctionCalls().front(), *CI);
  }
}
