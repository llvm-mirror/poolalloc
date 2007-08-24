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

#define DEBUG_TYPE "bu_dsa"
#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Timer.h"
using namespace llvm;

namespace {
  STATISTIC (MaxSCC, "Maximum SCC Size in Call Graph");
  STATISTIC (NumBUInlines, "Number of graphs inlined");
  STATISTIC (NumCallEdges, "Number of 'actual' call edges");

  RegisterPass<BUDataStructures>
  X("dsa-bu", "Bottom-up Data Structure Analysis");
}

char BUDataStructures::ID;

// run - Calculate the bottom up data structure graphs for each function in the
// program.
//
bool BUDataStructures::runOnModule(Module &M) {
  StdLibDataStructures &LocalDSA = getAnalysis<StdLibDataStructures>();
  setGraphSource(&LocalDSA);
  setTargetData(LocalDSA.getTargetData());
  setGraphClone(false);
  GlobalECs = LocalDSA.getGlobalECs();

  GlobalsGraph = new DSGraph(LocalDSA.getGlobalsGraph(), GlobalECs);
  GlobalsGraph->setPrintAuxCalls();

  IndCallGraphMap = new std::map<std::vector<Function*>,
                           std::pair<DSGraph*, std::vector<DSNodeHandle> > >();

  std::vector<Function*> Stack;
  hash_map<Function*, unsigned> ValMap;
  unsigned NextID = 1;

  Function *MainFunc = M.getFunction("main");
  if (MainFunc)
    calculateGraphs(MainFunc, Stack, NextID, ValMap);

  // Calculate the graphs for any functions that are unreachable from main...
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && !DSInfo.count(I)) {
      if (MainFunc)
        DOUT << "*** BU: Function unreachable from main: "
             << I->getName() << "\n";
      calculateGraphs(I, Stack, NextID, ValMap);     // Calculate all graphs.
    }

  // If we computed any temporary indcallgraphs, free them now.
  for (std::map<std::vector<Function*>,
         std::pair<DSGraph*, std::vector<DSNodeHandle> > >::iterator I =
         IndCallGraphMap->begin(), E = IndCallGraphMap->end(); I != E; ++I) {
    I->second.second.clear();  // Drop arg refs into the graph.
    delete I->second.first;
  }
  delete IndCallGraphMap;

  // At the end of the bottom-up pass, the globals graph becomes complete.
  // FIXME: This is not the right way to do this, but it is sorta better than
  // nothing!  In particular, externally visible globals and unresolvable call
  // nodes at the end of the BU phase should make things that they point to
  // incomplete in the globals graph.
  //
  GlobalsGraph->removeTriviallyDeadNodes();
  GlobalsGraph->maskIncompleteMarkers();

  // Mark external globals incomplete.
  GlobalsGraph->markIncompleteNodes(DSGraph::IgnoreGlobals);

  formGlobalECs();

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

  NumCallEdges += ActualCallees.size();

  return false;
}

static void GetAllCallees(const DSCallSite &CS,
                          std::vector<Function*> &Callees) {
  if (CS.isDirectCall()) {
    Callees.push_back(CS.getCalleeFunc());
  } else if (!CS.getCalleeNode()->isIncompleteNode()) {
    // Get all callees.
    CS.getCalleeNode()->addFullFunctionList(Callees);
  }
}

/// GetAllAuxCallees - Return a list containing all of the resolvable callees in
/// the aux list for the specified graph in the Callees vector.
static void GetAllAuxCallees(DSGraph &G, std::vector<Function*> &Callees) {
  Callees.clear();
  for (DSGraph::afc_iterator I = G.afc_begin(), E = G.afc_end(); I != E; ++I)
    GetAllCallees(*I, Callees);
}

unsigned BUDataStructures::calculateGraphs(Function *F,
                                           std::vector<Function*> &Stack,
                                           unsigned &NextID,
                                           hash_map<Function*, unsigned> &ValMap) {
  assert(!ValMap.count(F) && "Shouldn't revisit functions!");
  unsigned Min = NextID++, MyID = Min;
  ValMap[F] = Min;
  Stack.push_back(F);

  DSGraph &Graph = getOrCreateGraph(F);

  // Find all callee functions.
  std::vector<Function*> CalleeFunctions;
  GetAllAuxCallees(Graph, CalleeFunctions);

  // The edges out of the current node are the call site targets...
  for (unsigned i = 0, e = CalleeFunctions.size(); i != e; ++i) {
    Function *Callee = CalleeFunctions[i];
    unsigned M;
    // Have we visited the destination function yet?
    hash_map<Function*, unsigned>::iterator It = ValMap.find(Callee);
    if (It == ValMap.end())  // No, visit it now.
      M = calculateGraphs(Callee, Stack, NextID, ValMap);
    else                    // Yes, get it's number.
      M = It->second;
    if (M < Min) Min = M;
  }

  assert(ValMap[F] == MyID && "SCC construction assumption wrong!");
  if (Min != MyID)
    return Min;         // This is part of a larger SCC!

  // If this is a new SCC, process it now.
  if (Stack.back() == F) {           // Special case the single "SCC" case here.
    DOUT << "Visiting single node SCC #: " << MyID << " fn: "
         << F->getName() << "\n";
    Stack.pop_back();
    DOUT << "  [BU] Calculating graph for: " << F->getName()<< "\n";
    calculateGraph(Graph);
    DOUT << "  [BU] Done inlining: " << F->getName() << " ["
         << Graph.getGraphSize() << "+" << Graph.getAuxFunctionCalls().size()
         << "]\n";

    if (MaxSCC < 1) MaxSCC = 1;

    // Should we revisit the graph?  Only do it if there are now new resolvable
    // callees.
    GetAllAuxCallees(Graph, CalleeFunctions);
    if (!CalleeFunctions.empty()) {
      DOUT << "Recalculating " << F->getName() << " due to new knowledge\n";
      ValMap.erase(F);
      return calculateGraphs(F, Stack, NextID, ValMap);
    } else {
      ValMap[F] = ~0U;
    }
    return MyID;

  } else {
    // SCCFunctions - Keep track of the functions in the current SCC
    //
    std::vector<DSGraph*> SCCGraphs;

    unsigned SCCSize = 1;
    Function *NF = Stack.back();
    ValMap[NF] = ~0U;
    DSGraph &SCCGraph = getDSGraph(*NF);

    // First thing first, collapse all of the DSGraphs into a single graph for
    // the entire SCC.  Splice all of the graphs into one and discard all of the
    // old graphs.
    //
    while (NF != F) {
      Stack.pop_back();
      NF = Stack.back();
      ValMap[NF] = ~0U;

      DSGraph &NFG = getDSGraph(*NF);

      // Update the Function -> DSG map.
      for (DSGraph::retnodes_iterator I = NFG.retnodes_begin(),
             E = NFG.retnodes_end(); I != E; ++I)
        DSInfo[I->first] = &SCCGraph;

      SCCGraph.spliceFrom(NFG);
      delete &NFG;

      ++SCCSize;
    }
    Stack.pop_back();

    DOUT << "Calculating graph for SCC #: " << MyID << " of size: "
         << SCCSize << "\n";

    // Compute the Max SCC Size.
    if (MaxSCC < SCCSize)
      MaxSCC = SCCSize;

    // Clean up the graph before we start inlining a bunch again...
    SCCGraph.removeDeadNodes(DSGraph::KeepUnreachableGlobals);

    // Now that we have one big happy family, resolve all of the call sites in
    // the graph...
    calculateGraph(SCCGraph);
    DOUT << "  [BU] Done inlining SCC  [" << SCCGraph.getGraphSize()
         << "+" << SCCGraph.getAuxFunctionCalls().size() << "]\n"
         << "DONE with SCC #: " << MyID << "\n";

    // We never have to revisit "SCC" processed functions...
    return MyID;
  }

  return MyID;  // == Min
}


// releaseMemory - If the pass pipeline is done with this pass, we can release
// our memory... here...
//
void BUDataStructures::releaseMyMemory() {
  for (hash_map<Function*, DSGraph*>::iterator I = DSInfo.begin(),
         E = DSInfo.end(); I != E; ++I) {
    I->second->getReturnNodes().erase(I->first);
    if (I->second->getReturnNodes().empty())
      delete I->second;
  }

  // Empty map so next time memory is released, data structures are not
  // re-deleted.
  DSInfo.clear();
  delete GlobalsGraph;
  GlobalsGraph = 0;
}

DSGraph &BUDataStructures::CreateGraphForExternalFunction(const Function &Fn) {
  Function *F = const_cast<Function*>(&Fn);
  DSGraph *DSG = new DSGraph(GlobalECs, GlobalsGraph->getTargetData());
  DSInfo[F] = DSG;
  DSG->setGlobalsGraph(GlobalsGraph);
  DSG->setPrintAuxCalls();

  // Add function to the graph.
  DSG->getReturnNodes().insert(std::make_pair(F, DSNodeHandle()));

  if (F->getName() == "free") { // Taking the address of free.

    // Free should take a single pointer argument, mark it as heap memory.
    DSNodeHandle N(new DSNode(0, DSG));
    N.getNode()->setHeapMarker();
    DSG->getNodeForValue(F->arg_begin()).mergeWith(N);

  } else {
    cerr << "Unrecognized external function: " << F->getName() << "\n";
    abort();
  }

  return *DSG;
}

void BUDataStructures::calculateGraph(DSGraph &Graph) {
  // If this graph contains the main function, clone the globals graph into this
  // graph before we inline callees and other fun stuff.
  bool ContainsMain = false;
  DSGraph::ReturnNodesTy &ReturnNodes = Graph.getReturnNodes();

  for (DSGraph::ReturnNodesTy::iterator I = ReturnNodes.begin(),
         E = ReturnNodes.end(); I != E; ++I)
    if (I->first->hasExternalLinkage() && I->first->getName() == "main") {
      ContainsMain = true;
      break;
    }

  // If this graph contains main, copy the contents of the globals graph over.
  // Note that this is *required* for correctness.  If a callee contains a use
  // of a global, we have to make sure to link up nodes due to global-argument
  // bindings.
  if (ContainsMain) {
    const DSGraph &GG = *Graph.getGlobalsGraph();
    ReachabilityCloner RC(Graph, GG,
                          DSGraph::DontCloneCallNodes |
                          DSGraph::DontCloneAuxCallNodes);

    // Clone the global nodes into this graph.
    for (DSScalarMap::global_iterator I = GG.getScalarMap().global_begin(),
           E = GG.getScalarMap().global_end(); I != E; ++I)
      if (isa<GlobalVariable>(*I))
        RC.getClonedNH(GG.getNodeForValue(*I));
  }


  // Move our call site list into TempFCs so that inline call sites go into the
  // new call site list and doesn't invalidate our iterators!
  std::list<DSCallSite> TempFCs;
  std::list<DSCallSite> &AuxCallsList = Graph.getAuxFunctionCalls();
  TempFCs.swap(AuxCallsList);

  bool Printed = false;
  std::vector<Function*> CalledFuncs;
  while (!TempFCs.empty()) {
    DSCallSite &CS = *TempFCs.begin();

    CalledFuncs.clear();

    // Fast path for noop calls.  Note that we don't care about merging globals
    // in the callee with nodes in the caller here.
    if (CS.getRetVal().isNull() && CS.getNumPtrArgs() == 0) {
      TempFCs.erase(TempFCs.begin());
      continue;
    }

    GetAllCallees(CS, CalledFuncs);

    if (CalledFuncs.empty()) {
      // Remember that we could not resolve this yet!
      AuxCallsList.splice(AuxCallsList.end(), TempFCs, TempFCs.begin());
      continue;
    } else {
      DSGraph *GI;
      Instruction *TheCall = CS.getCallSite().getInstruction();

      if (CalledFuncs.size() == 1) {
        Function *Callee = CalledFuncs[0];
        ActualCallees.insert(std::make_pair(TheCall, Callee));

        // Get the data structure graph for the called function.
        GI = &getDSGraph(*Callee);  // Graph to inline
        DOUT << "    Inlining graph for " << Callee->getName()
             << "[" << GI->getGraphSize() << "+"
             << GI->getAuxFunctionCalls().size() << "] into '"
             << Graph.getFunctionNames() << "' [" << Graph.getGraphSize() <<"+"
             << Graph.getAuxFunctionCalls().size() << "]\n";
        Graph.mergeInGraph(CS, *Callee, *GI,
                           DSGraph::StripAllocaBit|DSGraph::DontCloneCallNodes);
        ++NumBUInlines;
      } else {
        if (!Printed)
          DEBUG(std::cerr << "In Fns: " << Graph.getFunctionNames() << "\n");
          DEBUG(std::cerr << "  calls " << CalledFuncs.size()
                  << " fns from site: " << CS.getCallSite().getInstruction()
                  << "  " << *CS.getCallSite().getInstruction());
        DEBUG(std::cerr << "   Fns =");
        unsigned NumPrinted = 0;

        for (std::vector<Function*>::iterator I = CalledFuncs.begin(),
               E = CalledFuncs.end(); I != E; ++I) {
          if (NumPrinted++ < 8) cerr << " " << (*I)->getName();

          // Add the call edges to the call graph.
          ActualCallees.insert(std::make_pair(TheCall, *I));
        }
        cerr << "\n";

        // See if we already computed a graph for this set of callees.
        std::sort(CalledFuncs.begin(), CalledFuncs.end());
        std::pair<DSGraph*, std::vector<DSNodeHandle> > &IndCallGraph =
          (*IndCallGraphMap)[CalledFuncs];

        if (IndCallGraph.first == 0) {
          std::vector<Function*>::iterator I = CalledFuncs.begin(),
            E = CalledFuncs.end();

          // Start with a copy of the first graph.
          GI = IndCallGraph.first = new DSGraph(getDSGraph(**I), GlobalECs);
          GI->setGlobalsGraph(Graph.getGlobalsGraph());
          std::vector<DSNodeHandle> &Args = IndCallGraph.second;

          // Get the argument nodes for the first callee.  The return value is
          // the 0th index in the vector.
          GI->getFunctionArgumentsForCall(*I, Args);

          // Merge all of the other callees into this graph.
          for (++I; I != E; ++I) {
            // If the graph already contains the nodes for the function, don't
            // bother merging it in again.
            if (!GI->containsFunction(*I)) {
              GI->cloneInto(getDSGraph(**I));
              ++NumBUInlines;
            }

            std::vector<DSNodeHandle> NextArgs;
            GI->getFunctionArgumentsForCall(*I, NextArgs);
            unsigned i = 0, e = Args.size();
            for (; i != e; ++i) {
              if (i == NextArgs.size()) break;
              Args[i].mergeWith(NextArgs[i]);
            }
            for (e = NextArgs.size(); i != e; ++i)
              Args.push_back(NextArgs[i]);
          }

          // Clean up the final graph!
          GI->removeDeadNodes(DSGraph::KeepUnreachableGlobals);
        } else {
          cerr << "***\n*** RECYCLED GRAPH ***\n***\n";
        }

        GI = IndCallGraph.first;

        // Merge the unified graph into this graph now.
        DOUT << "    Inlining multi callee graph "
             << "[" << GI->getGraphSize() << "+"
             << GI->getAuxFunctionCalls().size() << "] into '"
             << Graph.getFunctionNames() << "' [" << Graph.getGraphSize() <<"+"
             << Graph.getAuxFunctionCalls().size() << "]\n";

        Graph.mergeInGraph(CS, IndCallGraph.second, *GI,
                           DSGraph::StripAllocaBit |
                           DSGraph::DontCloneCallNodes);
        ++NumBUInlines;
      }
    }
    TempFCs.erase(TempFCs.begin());
  }

  // Recompute the Incomplete markers
  Graph.maskIncompleteMarkers();
  Graph.markIncompleteNodes(DSGraph::MarkFormalArgs);

  // Delete dead nodes.  Treat globals that are unreachable but that can
  // reach live nodes as live.
  Graph.removeDeadNodes(DSGraph::KeepUnreachableGlobals);

  // When this graph is finalized, clone the globals in the graph into the
  // globals graph to make sure it has everything, from all graphs.
  DSScalarMap &MainSM = Graph.getScalarMap();
  ReachabilityCloner RC(*GlobalsGraph, Graph, DSGraph::StripAllocaBit);

  // Clone everything reachable from globals in the function graph into the
  // globals graph.
  for (DSScalarMap::global_iterator I = MainSM.global_begin(),
         E = MainSM.global_end(); I != E; ++I)
    RC.getClonedNH(MainSM[*I]);

  //Graph.writeGraphToFile(cerr, "bu_" + F.getName());
}

static const Function *getFnForValue(const Value *V) {
  if (const Instruction *I = dyn_cast<Instruction>(V))
    return I->getParent()->getParent();
  else if (const Argument *A = dyn_cast<Argument>(V))
    return A->getParent();
  else if (const BasicBlock *BB = dyn_cast<BasicBlock>(V))
    return BB->getParent();
  return 0;
}

/// deleteValue/copyValue - Interfaces to update the DSGraphs in the program.
/// These correspond to the interfaces defined in the AliasAnalysis class.
void BUDataStructures::deleteValue(Value *V) {
  if (const Function *F = getFnForValue(V)) {  // Function local value?
    // If this is a function local value, just delete it from the scalar map!
    getDSGraph(*F).getScalarMap().eraseIfExists(V);
    return;
  }

  if (Function *F = dyn_cast<Function>(V)) {
    assert(getDSGraph(*F).getReturnNodes().size() == 1 &&
           "cannot handle scc's");
    delete DSInfo[F];
    DSInfo.erase(F);
    return;
  }

  assert(!isa<GlobalVariable>(V) && "Do not know how to delete GV's yet!");
}

void BUDataStructures::copyValue(Value *From, Value *To) {
  if (From == To) return;
  if (const Function *F = getFnForValue(From)) {  // Function local value?
    // If this is a function local value, just delete it from the scalar map!
    getDSGraph(*F).getScalarMap().copyScalarIfExists(From, To);
    return;
  }

  if (Function *FromF = dyn_cast<Function>(From)) {
    Function *ToF = cast<Function>(To);
    assert(!DSInfo.count(ToF) && "New Function already exists!");
    DSGraph *NG = new DSGraph(getDSGraph(*FromF), GlobalECs);
    DSInfo[ToF] = NG;
    assert(NG->getReturnNodes().size() == 1 && "Cannot copy SCC's yet!");

    // Change the Function* is the returnnodes map to the ToF.
    DSNodeHandle Ret = NG->retnodes_begin()->second;
    NG->getReturnNodes().clear();
    NG->getReturnNodes()[ToF] = Ret;
    return;
  }

  if (const Function *F = getFnForValue(To)) {
    DSGraph &G = getDSGraph(*F);
    G.getScalarMap().copyScalarIfExists(From, To);
    return;
  }

  cerr << *From;
  cerr << *To;
  assert(0 && "Do not know how to copy this yet!");
  abort();
}
