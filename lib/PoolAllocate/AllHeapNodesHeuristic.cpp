//===-- Heuristic.cpp - Interface to PA heuristics ------------------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements the old "AllNodes" heuristic which the SAFECode
// heuristic claims only pool allocated heap nodes.
//
// FIXME: It seems that the old alignment heuristics contained in this file
//        were designed for 32-bit x86 processors (which makes sense as
//        poolalloc was developed before x86-64).  We need to revisit this
//        code and decide what the heuristics should do today.
//
//===----------------------------------------------------------------------===//

#include "dsa/DSGraphTraits.h"
#include "poolalloc/Heuristic.h"
#include "poolalloc/PoolAllocate.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/IR/DataLayout.h"
#include <iostream>

using namespace llvm;
using namespace PA;

//
// Function: GetNodesReachableFromGlobals()
//
// Description:
//  This function finds all DSNodes which are reachable from globals.  It finds
//  DSNodes both within the local DSGraph as well as in the Globals graph that
//  are reachable from globals.  It does, however, filter out those DSNodes
//  which are of no interest to automatic pool allocation.
//
// Inputs:
//  G - The DSGraph for which to find DSNodes which are reachable by globals.
//      This DSGraph can either by a DSGraph associated with a function *or*
//      it can be the globals graph itself.
//
// Outputs:
//  NodesFromGlobals - A reference to a container object in which to record
//                     DSNodes reachable from globals.  DSNodes are *added* to
//                     this container; it is not cleared by this function.
//                     DSNodes from both the local and globals graph are added.
void
AllHeapNodesHeuristic::GetNodesReachableFromGlobals (DSGraph* G,
                              DenseSet<const DSNode*> &NodesFromGlobals) {
  //
  // Get the globals graph associated with this DSGraph.  If the globals graph
  // is NULL, then the graph that was passed in *is* the globals graph.
  //
  DSGraph * GlobalsGraph = G->getGlobalsGraph();
  if (!GlobalsGraph)
    GlobalsGraph = G;

  //
  // Find all DSNodes which are reachable in the globals graph.
  //
  for (DSGraph::node_iterator NI = GlobalsGraph->node_begin();
       NI != GlobalsGraph->node_end();
       ++NI) {
    NI->markReachableNodes(NodesFromGlobals);
  }

  //
  // Remove those global nodes which we know will never be pool allocated.
  //
  
  std::vector<const DSNode *> toRemove;
  for (DenseSet<const DSNode*>::iterator I = NodesFromGlobals.begin(),
         E = NodesFromGlobals.end(); I != E; ) {
    DenseSet<const DSNode*>::iterator Last = I; ++I;

    const DSNode *tmp = *Last;
    if (!(tmp->isHeapNode())) 
      toRemove.push_back (tmp);
    // Do not poolallocate nodes that are cast to Int.
    // As we do not track through ints, these could be escaping
    if (tmp->isPtrToIntNode())
      toRemove.push_back(tmp);
  }
 
  //
  // Remove all globally reachable DSNodes which do not require pools.
  //
  for (unsigned index = 0; index < toRemove.size(); ++index) {
    NodesFromGlobals.erase(toRemove[index]);
  }

  //
  // Now the fun part.  Find DSNodes in the local graph that correspond to
  // those nodes reachable in the globals graph.  Add them to the set of
  // reachable nodes, too.
  //
  if (G->getGlobalsGraph()) {
    //
    // Compute a mapping between local DSNodes and DSNodes in the globals
    // graph.
    //
    DSGraph::NodeMapTy NodeMap;
    G->computeGToGGMapping (NodeMap);

    //
    // Scan through all DSNodes in the local graph.  If a local DSNode has a
    // corresponding DSNode in the globals graph that is reachable from a 
    // global, then add the local DSNode to the set of DSNodes reachable from a
    // global.
    //
    // FIXME: A node's existance within the global DSGraph is probably
    //        sufficient evidence that it is reachable from a global.
    //

    DSGraph::node_iterator ni = G->node_begin();
    for (; ni != G->node_end(); ++ni) {
      DSNode * N = ni;
      if (NodesFromGlobals.count (NodeMap[N].getNode()))
        NodesFromGlobals.insert (N);
    }
  }
}

//
// Method: findGlobalPoolNodes()
//
// Description:
//  This method finds DSNodes that are reachable from globals and that need a
//  pool.  The Automatic Pool Allocation transform will use the returned
//  information to build global pools for the DSNodes in question.
//
//  Note that this method does not assign DSNodes to pools; it merely decides
//  which DSNodes are reachable from globals and will need a pool of global
//  scope.
//
// Outputs:
//  Nodes - The DSNodes that are both reachable from globals and which should
//          have global pools will be *added* to this container.
//
void
AllHeapNodesHeuristic::findGlobalPoolNodes (DSNodeSet_t & Nodes) {
  // Get the globals graph for the program.
  DSGraph* GG = Graphs->getGlobalsGraph();

  // Get all of the nodes reachable from globals.
  DenseSet<const DSNode*> GlobalHeapNodes;
  GetNodesReachableFromGlobals (GG, GlobalHeapNodes);
  //
  // Create a global pool for each global DSNode.
  //
  for (DenseSet<const DSNode *>::iterator NI = GlobalHeapNodes.begin();
              NI != GlobalHeapNodes.end();++NI) {
    const DSNode * N = *NI;
    PoolMap[N] = OnePool(N);
  }

  //
  // Now find all DSNodes belonging to function-local DSGraphs which are
  // mirrored in the globals graph.  These DSNodes require a global pool, too.
  //
  for (Module::iterator F = M->begin(); F != M->end(); ++F) {
    if (Graphs->hasDSGraph(*F)) {
      DSGraph* G = Graphs->getDSGraph(*F);
      DSGraph::NodeMapTy NodeMap;
      G->computeGToGGMapping (NodeMap);
      //
      // Scan through all DSNodes in the local graph.  If a local DSNode has a
      // corresponding DSNode in the globals graph that is reachable from a 
      // global, then add the local DSNode to the set of DSNodes reachable from
      // a global.
      //
      DSGraph::node_iterator ni = G->node_begin();
      for (; ni != G->node_end(); ++ni) {
        DSNode * N = ni;
        DSNode * GGN = NodeMap[N].getNode();
        
        //assert (!GGN || GlobalHeapNodes.count (GGN));
        if (GGN && GlobalHeapNodes.count (GGN))
          PoolMap[GGN].NodesInPool.push_back (N);
      }
    }
  }

  //
  // Copy the values into the output container.  Note that DenseSet has no
  // iterator traits (or whatever allows us to treat DenseSet has a generic
  // container), so we have to use a loop to copy values from the DenseSet into
  // the output container.
  //
  for (DenseSet<const DSNode*>::iterator I = GlobalHeapNodes.begin(),
         E = GlobalHeapNodes.end(); I != E; ++I) {
    Nodes.insert (*I);
  }

  return;
}

//===-- AllHeapNodes Heuristic ------------------------------------------------===//
//
// This heuristic pool allocates everything possible into separate pools.
//

bool
AllHeapNodesHeuristic::runOnModule (Module & Module) {
  //
  // Remember which module we are analyzing.
  //
  M = &Module;

  //
  // Get the reference to the DSA Graph.
  //
  Graphs = &getAnalysis<EQTDDataStructures>();   
  assert (Graphs && "No DSGraphs!\n");

  //
  // Find DSNodes which are reachable from globals and should be pool
  // allocated.
  //
  findGlobalPoolNodes (GlobalPoolNodes);

  // We never modify anything in this pass
  return false;
}

void
AllHeapNodesHeuristic::AssignToPools (const std::vector<const DSNode*> &NodesToPA,
                                  Function *F, DSGraph* G,
                                  std::vector<OnePool> &ResultPools) {
  for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i){
     if (PoolMap.find (NodesToPA[i]) != PoolMap.end())           
       ResultPools.push_back(PoolMap[NodesToPA[i]]);
     else
       ResultPools.push_back (OnePool(NodesToPA[i]));
  }
}


//
// Register all of the heuristic passes.
//
static RegisterPass<AllHeapNodesHeuristic>
A ("paheur-AllHeapNodes", "Pool allocate all (heap?) nodes");

RegisterAnalysisGroup<Heuristic> Heuristic1(A);

char AllHeapNodesHeuristic::ID = 0;


