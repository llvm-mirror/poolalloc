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
#include "llvm/Constants.h"
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

#if 0
  //
  // Merge the DSGraphs of functions belonging to an SCC.
  //
  mergeSCCs();
#endif

  //
  // Make sure we have a DSGraph for all declared functions in the Module.
  // While we may not need them in this DSA pass, a later DSA pass may ask us
  // for their DSGraphs, and we want to have them if asked.
  //
  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (!(F->isDeclaration()))
      getOrCreateGraph(F);
  }

  //
  // Do a post-order traversal of the SCC callgraph and do bottom-up inlining.
  //
#if 0
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
#else
  postOrderInline (M);
#endif


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
  GlobalsGraph->computeExternalFlags(DSGraph::DontMarkFormalsExternal);

  //
  // Create equivalence classes for aliasing globals so that we only need to
  // record one global per DSNode.
  //
  formGlobalECs();

  // Merge the globals variables (not the calls) from the globals graph back
  // into the individual function's graph so that changes made to globals during
  // BU can be reflected. This is specifically needed for correct call graph

  for (Module::iterator F = M.begin(); F != M.end(); ++F) {
    if (!(F->isDeclaration())){
      DSGraph *Graph  = getOrCreateGraph(F);
      cloneGlobalsInto(Graph);
      Graph->buildCallGraph(callgraph, filterCallees);
      Graph->maskIncompleteMarkers();
      Graph->markIncompleteNodes(DSGraph::MarkFormalArgs |
                                   DSGraph::IgnoreGlobals);
      Graph->computeExternalFlags(DSGraph::DontMarkFormalsExternal);
    }
  }

  NumCallEdges += callgraph.size();

  //
  // Put the callgraph into canonical form by finding SCCs.  It has been
  // updated since we did this last.
  //
  callgraph.buildSCCs();
  callgraph.buildRoots();

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
// Function: applyCallsiteFilter
//
// Description:
//  Given a DSCallSite, and a list of functions, filter out the ones
//  that aren't callable from the given Callsite.
//
//  Does no filtering if 'filterCallees' is set to false.
//
void BUDataStructures::
applyCallsiteFilter(const DSCallSite &DCS, std::vector<const Function*> &Callees) {

  if (!filterCallees) return;

  std::vector<const Function*>::iterator I = Callees.begin();
  CallSite CS = DCS.getCallSite();
  while(I != Callees.end()) {
    if (functionIsCallable(CS, *I)) {
      ++I;
    }
    else {
      I = Callees.erase(I);
    }
  }
}

//
// Function: getAllCallees()
//
// Description:
//  Given a DSCallSite, add to the list the functions that can be called by
//  the call site *if* it is resolvable.  Uses 'applyCallsiteFilter' to
//  only add the functions that are valid targets of this callsite.
//
void BUDataStructures::
getAllCallees(const DSCallSite &CS, std::vector<const Function*> &Callees) {
  //
  // FIXME: Should we check for the Unknown flag on indirect call sites?
  //
  // Direct calls to functions that have bodies are always resolvable.
  // Indirect function calls that are for a complete call site (the analysis
  // knows everything about the call site) and do not target external functions
  // are also resolvable.
  //
  if (CS.isDirectCall()) {
    if (!CS.getCalleeFunc()->isDeclaration())
      Callees.push_back(CS.getCalleeFunc());
  } else if (!CS.getCalleeNode()->isIncompleteNode()) {
    // Get all callees.
    if (!CS.getCalleeNode()->isExternFuncNode()) {
      // Get all the callees for this callsite
      std::vector<const Function *> tempCallees;
      CS.getCalleeNode()->addFullFunctionList(tempCallees);
      // Filter out the ones that are invalid targets with respect
      // to this particular callsite.
      applyCallsiteFilter(CS, tempCallees);
      // Insert the remaining callees (legal ones, if we're filtering)
      // into the master 'Callees' list
      Callees.insert(Callees.end(),tempCallees.begin(),tempCallees.end());
    }
  }
}

//
// Function: getAllAuxCallees()
//
// Description:
//  Return a list containing all of the resolvable callees in the auxiliary
//  list for the specified graph in the Callees vector.
//
// Inputs:
//  G - The DSGraph for which the callers wants a list of resolvable call
//      sites.
//
// Outputs:
//  Callees - A list of all functions that can be called from resolvable call
//            sites.  This list is always cleared by this function before any
//            functions are added to it.
//
void BUDataStructures::
getAllAuxCallees (DSGraph* G, std::vector<const Function*> & Callees) {
  //
  // Clear out the list of callees.
  //
  Callees.clear();
  for (DSGraph::afc_iterator I = G->afc_begin(), E = G->afc_end(); I != E; ++I)
    getAllCallees(*I, Callees);
}

//
// Method: postOrderInline()
//
// Description:
//  This methods does a post order traversal of the call graph and performs
//  bottom-up inlining of the DSGraphs.
//
void
BUDataStructures::postOrderInline (Module & M) {
  // Variables used for Tarjan SCC-finding algorithm.  These are passed into
  // the recursive function used to find SCCs.
  std::vector<const Function*> Stack;
  std::map<const Function*, unsigned> ValMap;
  unsigned NextID = 1;


  // do post order traversal on the global ctors. Use this information to update
  // the globals graph.
  const char *Name = "llvm.global_ctors";
  GlobalVariable *GV = M.getNamedGlobal(Name);
  if (GV && !(GV->isDeclaration()) && !(GV->hasLocalLinkage())){
  // Should be an array of '{ int, void ()* }' structs.  The first value is
  // the init priority, which we ignore.
    ConstantArray *InitList = dyn_cast<ConstantArray>(GV->getInitializer());
    if (InitList) {
      for (unsigned i = 0, e = InitList->getNumOperands(); i != e; ++i)
        if (ConstantStruct *CS = dyn_cast<ConstantStruct>(InitList->getOperand(i))) {
          if (CS->getNumOperands() != 2) 
            break; // Not array of 2-element structs.
          Constant *FP = CS->getOperand(1);
          if (FP->isNullValue())
            break;  // Found a null terminator, exit.
   
          if (ConstantExpr *CE = dyn_cast<ConstantExpr>(FP))
            if (CE->isCast())
              FP = CE->getOperand(0);
          if (Function *F = dyn_cast<Function>(FP)) {
           calculateGraphs(F, Stack, NextID, ValMap);
           CloneAuxIntoGlobal(getDSGraph(*F));
          }
        }
        // propogte information calculated 
        // from the globals graph to the other graphs.
        for (Module::iterator F = M.begin(); F != M.end(); ++F) {
          if (!(F->isDeclaration())){
            DSGraph *Graph  = getDSGraph(*F);
            cloneGlobalsInto(Graph);
          }
        }
      }
  }
 
  //
  // Start the post order traversal with the main() function.  If there is no
  // main() function, don't worry; we'll have a separate traversal for inlining
  // graphs for functions not reachable from main().
  //
  Function *MainFunc = M.getFunction ("main");
  if (MainFunc && !MainFunc->isDeclaration()) {
    calculateGraphs(MainFunc, Stack, NextID, ValMap);
    CloneAuxIntoGlobal(getDSGraph(*MainFunc));
  }

  //
  // Calculate the graphs for any functions that are unreachable from main...
  //
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && !ValMap.count(I)) {
      if (MainFunc)
        DEBUG(errs() << debugname << ": Function unreachable from main: "
        << I->getName() << "\n");
      calculateGraphs(I, Stack, NextID, ValMap);     // Calculate all graphs.
      CloneAuxIntoGlobal(getDSGraph(*I));
    }

  return;
}

//
// Method: calculateGraphs()
//
// Description:
//  Perform recursive bottom-up inlining of DSGraphs from callee to caller.
//
// Inputs:
//  F - The function which should have its callees' DSGraphs merged into its
//      own DSGraph.
//  Stack - The stack used for Tarjan's SCC-finding algorithm.
//  NextID - The nextID value used for Tarjan's SCC-finding algorithm.
//  ValMap - The map used for Tarjan's SCC-finding algorithm.
//
// Return value:
//
unsigned
BUDataStructures::calculateGraphs (const Function *F,
                                   TarjanStack & Stack,
                                   unsigned & NextID,
                                   TarjanMap & ValMap) {
  assert(!ValMap.count(F) && "Shouldn't revisit functions!");
  unsigned Min = NextID++, MyID = Min;
  ValMap[F] = Min;
  Stack.push_back(F);

  //
  // FIXME: This test should be generalized to be any function that we have
  // already processed in the case when there isn't a main() or there are
  // unreachable functions!
  //
  if (F->isDeclaration()) {   // sprintf, fprintf, sscanf, etc...
    // No callees!
    Stack.pop_back();
    ValMap[F] = ~0;
    return Min;
  }

  //
  // Get the DSGraph of the current function.  Make one if one doesn't exist.
  //
  DSGraph* Graph = getOrCreateGraph(F);

  //
  // Find all callee functions.  Use the DSGraph for this (do not use the call
  // graph (DSCallgraph) as we're still in the process of constructing it).
  //
  std::vector<const Function*> CalleeFunctions;
  getAllAuxCallees(Graph, CalleeFunctions);

  //
  // Iterate through each call target (these are the edges out of the current
  // node (i.e., the current function) in Tarjan graph parlance).  Find the
  // minimum assigned ID.
  //
  for (unsigned i = 0, e = CalleeFunctions.size(); i != e; ++i) {
    const Function *Callee = CalleeFunctions[i];
    unsigned M;
    //
    // If we have not visited this callee before, visit it now (this is the
    // post-order component of the Bottom-Up algorithm).  Otherwise, look up
    // the assigned ID value from the Tarjan Value Map.
    //
    TarjanMap::iterator It = ValMap.find(Callee);
    if (It == ValMap.end())  // No, visit it now.
      M = calculateGraphs(Callee, Stack, NextID, ValMap);
    else                    // Yes, get it's number.
      M = It->second;

    //
    // If we've found a function with a smaller ID than this funtion, record
    // that ID as the minimum ID.
    //
    if (M < Min) Min = M;
  }

  assert(ValMap[F] == MyID && "SCC construction assumption wrong!");

  //
  // If the minimum ID found is not this function's ID, then this function is
  // part of a larger SCC.
  //
  if (Min != MyID)
    return Min;

  //
  // If this is a new SCC, process it now.
  //
  if (Stack.back() == F) {           // Special case the single "SCC" case here.
    DEBUG(errs() << "Visiting single node SCC #: " << MyID << " fn: "
	  << F->getName() << "\n");
    Stack.pop_back();
    DEBUG(errs() << "  [BU] Calculating graph for: " << F->getName()<< "\n");
    calculateGraph (Graph);
    DEBUG(errs() << "  [BU] Done inlining: " << F->getName() << " ["
	  << Graph->getGraphSize() << "+" << Graph->getAuxFunctionCalls().size()
	  << "]\n");

    if (MaxSCC < 1) MaxSCC = 1;

    //
    // Should we revisit the graph?  Only do it if there are now new resolvable
    // callees.
    getAllAuxCallees (Graph, CalleeFunctions);
    if (!CalleeFunctions.empty()) {
      DEBUG(errs() << "Recalculating " << F->getName() << " due to new knowledge\n");
      ValMap.erase(F);
      return calculateGraphs(F, Stack, NextID, ValMap);
    } else {
      ValMap[F] = ~0U;
    }
    return MyID;
  } else {
    //
    // SCCFunctions - Keep track of the functions in the current SCC
    //
    std::vector<DSGraph*> SCCGraphs;

    unsigned SCCSize = 1;
    const Function *NF = Stack.back();
    ValMap[NF] = ~0U;
    DSGraph* SCCGraph = getDSGraph(*NF);

    //
    // First thing first: collapse all of the DSGraphs into a single graph for
    // the entire SCC.  Splice all of the graphs into one and discard all of
    // the old graphs.
    //
    while (NF != F) {
      Stack.pop_back();
      NF = Stack.back();
      ValMap[NF] = ~0U;

      DSGraph* NFG = getDSGraph(*NF);

      if (NFG != SCCGraph) {
        // Update the Function -> DSG map.
        for (DSGraph::retnodes_iterator I = NFG->retnodes_begin(),
               E = NFG->retnodes_end(); I != E; ++I)
          setDSGraph(*I->first, SCCGraph);
        
        SCCGraph->spliceFrom(NFG);
        delete NFG;
        ++SCCSize;
      }
    }
    Stack.pop_back();

    DEBUG(errs() << "Calculating graph for SCC #: " << MyID << " of size: "
	  << SCCSize << "\n");

    // Compute the Max SCC Size.
    if (MaxSCC < SCCSize)
      MaxSCC = SCCSize;

    // Clean up the graph before we start inlining a bunch again...
    SCCGraph->removeDeadNodes(DSGraph::KeepUnreachableGlobals);

    // Now that we have one big happy family, resolve all of the call sites in
    // the graph...
    calculateGraph(SCCGraph);
    DEBUG(errs() << "  [BU] Done inlining SCC  [" << SCCGraph->getGraphSize()
	  << "+" << SCCGraph->getAuxFunctionCalls().size() << "]\n"
	  << "DONE with SCC #: " << MyID << "\n");

    // We never have to revisit "SCC" processed functions...
    return MyID;
  }

  return MyID;  // == Min
}

#if 0
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
  G->buildCallGraph(callgraph,filterCallees);

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

#endif

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
    if (!CS.isIndirectCall() && CS.getRetVal().isNull() && CS.getNumPtrArgs() == 0 && !CS.isVarArg()) {
      TempFCs.erase(TempFCs.begin());
      continue;
    }

    // Find all callees for this callsite, according to the DSGraph!
    // Do *not* use the callgraph, because we're updating that as we go!
    std::vector<const Function*> CalledFuncs;
    getAllCallees(CS,CalledFuncs);


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
        //
        // Get the list of callees associated with the DSNode and remove those
        // that are external functions (i.e., have no function body).
        //
        std::vector<const Function*> NodeCallees;
        CS.getCalleeNode()->addFullFunctionList(NodeCallees);
        std::vector<const Function*>::iterator ErasePoint =
                std::remove_if(NodeCallees.begin(), NodeCallees.end(),
                               std::mem_fun(&Function::isDeclaration));
        NodeCallees.erase(ErasePoint, NodeCallees.end());

        // Remove callees that aren't legally called from this callsite.
        // We're done with the callsite if all /legal/ callees have been
        // taken care of already.  We remove them because they won't
        // be part of the callgraph (not because of this callsite anyway)
        // and so we shouldn't expect them to be.
        applyCallsiteFilter(CS,NodeCallees);

        //
        // Only erase this call site if there's nothing left to do for it.
        // This means that all of the function targets recorded in the DSNode
        // have already been incorporated into the call graph that we've been
        // constructing.
        //
        std::sort(CalledFuncs.begin(), CalledFuncs.end());
        std::sort(NodeCallees.begin(), NodeCallees.end());
        eraseCS = std::includes(CalledFuncs.begin(), CalledFuncs.end(),
                                NodeCallees.begin(), NodeCallees.end());
      }

      //
      // Update the statistics on resolved indirect function calls.
      //
      if (eraseCS)
        ++NumIndResolved;
      else
        ++NumIndUnresolved;
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
  Graph->computeExternalFlags(DSGraph::DontMarkFormalsExternal);
  
  //
  // Update the callgraph with the new information that we have gleaned.
  // NOTE : This must be called before removeDeadNodes, so that no 
  // information is lost due to deletion of DSCallNodes.
  Graph->buildCallGraph(callgraph,filterCallees);
  
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

