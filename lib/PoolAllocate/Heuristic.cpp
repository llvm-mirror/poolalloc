//===-- Heuristic.cpp - Interface to PA heuristics ------------------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This implements the various pool allocation heuristics.
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
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Target/TargetData.h"
#include <iostream>

using namespace llvm;
using namespace PA;

namespace {
  cl::opt<bool>
  DisableAlignOpt("poolalloc-disable-alignopt",
                  cl::desc("Force all pool alignment to 8 bytes"));
}

//
// Function: getRecommendedSize()
//
// Description:
//  Determine the recommended allocation size for objects represented by this
//  DSNode.  This is used to determine the size of objects allocated by
//  poolalloc().
//
// Inputs:
//  N - The DSNode representing the objects whose recommended size the caller
//      wants.
//
// Return value:
//  The size, in bytes, that should be used in allocating most objects
//  represented by this DSNode.
//
unsigned Heuristic::getRecommendedSize(const DSNode *N) {
  unsigned PoolSize = 0;

  //
  // If the DSNode only represents singleton objects (i.e., not arrays), then
  // find the size of the singleton object.
  //
  if (!N->isArrayNode()) {
    PoolSize = N->getSize();
  }

  //
  // If the object size is 1, just say that it's zero.  The run-time will
  // figure out how to deal with it.
  //
  if (PoolSize == 1) PoolSize = 0;
  return PoolSize;
}

//
// Function: Wants8ByteAlignment ()
//
// Description:
//  Determine if an object of the specified type should be allocated on an
//  8-byte boundary.  This may either be required by the target platform or may
//  merely improve performance by aligning data the way the processor wants
//  it.
//
// Inputs:
//  Ty   - The type of the object for which alignment should be tested.
//  Offs - The offset of the type within a derived type (e.g., a structure).
//         We will try to align a structure on an 8 byte boundary if one of its
//         elements can/needs to be.
//  TD   - A reference to the TargetData pass.
//
// Return value:
//  true - This type should be allocated on an 8-byte boundary.
//  false - This type does not need to be allocated on an 8-byte boundary.
//
// Notes:
//  FIXME: This is a complete hack for X86 right now.
//  FIXME: This code needs to be updated for x86-64.
//  FIXME: This code needs to handle LLVM first-class structures and vectors.
//
static bool
Wants8ByteAlignment(const Type *Ty, unsigned Offs, const TargetData &TD) {
  //
  // If the user has requested this optimization to be turned off, don't bother
  // doing it.
  //
  if (DisableAlignOpt) return true;

  //
  // If this type is at an align-able offset within its larger data structure,
  // see if we should 8 byte align it.
  //
  if ((Offs & 7) == 0) {
    //
    // Doubles always want to be 8-byte aligned regardless of what TargetData
    // claims.
    //
    if (Ty->isDoubleTy()) return true;

    //
    // If we are on a 64-bit system, we want to align 8-byte integers and
    // pointers.
    //
    if (TD.getPrefTypeAlignment(Ty) == 8)
      return true;
  }

  //
  // If this is a first-class data type, but it is located at an offset within
  // a structure that cannot be 8-byte aligned, then we cannot ever guarantee
  // to 8-byte align it.  Therefore, do not try to force it to 8-byte
  // alignment.
  if (Ty->isFirstClassType())
    return false;

  //
  // If this is a structure or array type, check if any of its elements at
  // 8-byte alignment desire to have 8-byte alignment.  If so, then the entire
  // object wants 8-byte alignment.
  //
  if (const StructType *STy = dyn_cast<StructType>(Ty)) {
    const StructLayout *SL = TD.getStructLayout(STy);
    for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
      if (Wants8ByteAlignment(STy->getElementType(i),
                              Offs+SL->getElementOffset(i), TD))
        return true;
    }
  } else if (const SequentialType *STy = dyn_cast<SequentialType>(Ty)) {
    return Wants8ByteAlignment(STy->getElementType(), Offs, TD);
  } else {
    errs() << *Ty << "\n";
    assert(0 && "Unknown type!");
  }

  return false;
}

unsigned Heuristic::getRecommendedAlignment(const Type *Ty,
                                            const TargetData &TD) {
  if (!Ty || Ty->isVoidTy())  // Is this void or collapsed?
    return 0;  // No known alignment, let runtime decide.

  return Wants8ByteAlignment(Ty, 0, TD) ? 8 : 4;
}

///
/// getRecommendedAlignment - Return the recommended object alignment for this
/// DSNode.
///
/// @Node - The DSNode for which allocation alignment information is requested.
///
/// FIXME: This method assumes an object wants 4-byte or 8-byte alignment.  The
///        new types in LLVM may want larger alignments such as 16-byte
///        alignment.  Need to update this code to handle that.
///
unsigned
Heuristic::getRecommendedAlignment(const DSNode *Node) {
  //
  // Get the TargetData information from the DSNode's DSGraph.
  //
  const TargetData &TD = Node->getParentGraph()->getTargetData();

  //
  // Iterate through all the types that the DSA type-inference algorithm
  // found and determine if any of them should be 8-byte aligned.  If so, then
  // we'll 8-byte align the entire structure.
  //
  DSNode::const_type_iterator tyi;
  for (tyi = Node->type_begin(); tyi != Node->type_end(); ++tyi) {
    for (svset<const Type*>::const_iterator tyii = tyi->second->begin(),
         tyee = tyi->second->end(); tyii != tyee; ++tyii) {
      //
      // Get the type of object allocated.  If there is no type, then it is
      // implicitly of void type.
      //
      const Type * TypeCreated = *tyii;
      if (TypeCreated) {
        //
        // If the type contains a pointer, it must be changed.
        //
        if (Wants8ByteAlignment(TypeCreated, tyi->first, TD)) {
          return 8;
        }
      }
    }
  }

  return 4;
}

//
// Function: GetNodesReachableFromGlobals()
//
// Description:
//  This function finds all DSNodes which are reachable from globals.  It finds
//  DSNodes both within the local DSGraph as well as in the Globals graph that
//  are reachable from globals.
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
static void
GetNodesReachableFromGlobals (DSGraph* G,
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
Heuristic::findGlobalPoolNodes (DSNodeSet_t & Nodes) {
  // Get the globals graph for the program.
  DSGraph* GG = Graphs->getGlobalsGraph();

  // Get all of the nodes reachable from globals.
  DenseSet<const DSNode*> GlobalHeapNodes;
  GetNodesReachableFromGlobals (GG, GlobalHeapNodes);

  //
  // Now find all DSNodes belonging to function-local DSGraphs which are
  // mirrored in the globals graph.  These DSNodes require a global pool, too.
  //
  for (Module::iterator F = M->begin(); F != M->end(); ++F) {
    if (Graphs->hasDSGraph(*F)) {
      DSGraph* G = Graphs->getDSGraph(*F);
      GetNodesReachableFromGlobals (G, GlobalHeapNodes);
    }
  }

  //
  // We do not want to create pools for all memory objects reachable from
  // globals.  We only want those that are or could be heap objects.
  //
  std::vector<const DSNode *> toRemove;
  for (DenseSet<const DSNode*>::iterator I = GlobalHeapNodes.begin(),
         E = GlobalHeapNodes.end(); I != E; ) {
    DenseSet<const DSNode*>::iterator Last = I; ++I;

    //
    // Nodes that escape to external code could be reachable from globals.
    // Nodes that are incomplete could be heap nodes.
    // Unknown nodes could be anything.
    //
    const DSNode *tmp = *Last;
    if (!(tmp->isHeapNode() ||
          tmp->isExternalNode() ||
          tmp->isIncompleteNode() ||
          tmp->isUnknownNode()))
      toRemove.push_back (tmp);
  }
  
  //
  // Remove all globally reachable DSNodes which do not require pools.
  //
  for (unsigned index = 0; index < toRemove.size(); ++index) {
    GlobalHeapNodes.erase(toRemove[index]);
  }

  //
  // Scan through all the local graphs looking for DSNodes which may be
  // reachable by a global.  These nodes may not end up in the globals graph 
  // because of the fact that DSA doesn't actually know what is happening to
  // them.
  //
  for (Module::iterator F = M->begin(); F != M->end(); ++F) {
    if (F->isDeclaration()) continue;
    DSGraph* G = Graphs->getDSGraph(*F);
    for (DSGraph::node_iterator I = G->node_begin(), E = G->node_end();
         I != E;
         ++I) {
      DSNode * Node = I;
      if (Node->isExternalNode() || Node->isIncompleteNode() ||
          Node->isUnknownNode()) {
        GlobalHeapNodes.insert (Node);
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

//
// Method: getGlobalPoolNodes()
//
// Description:
//  This method returns DSNodes that are reachable from globals and that need a
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
// Preconditions:
//  This method assumes that the findGlobalPoolNodes() has been called to find
//  all of the global DSNodes needing a pool.
//
void
Heuristic::getGlobalPoolNodes (std::vector<const DSNode *> & Nodes) {
  //
  // Copy the DSNodes which are globally reachable and need a global pool into
  // the set of DSNodes given to us by the caller.
  //
  Nodes.insert (Nodes.end(), GlobalPoolNodes.begin(), GlobalPoolNodes.end());
  return;
}

//
// Method: getLocalPoolNodes()
//
// Description:
//  For a given function, determine which DSNodes for that function should have
//  local pools created for them.
//
void
Heuristic::getLocalPoolNodes (const Function & F, DSNodeList_t & Nodes) {
  //
  // Get the DSGraph of the specified function.  If the DSGraph has no nodes,
  // then there is nothing we need to do.
  //
  DSGraph* G = Graphs->getDSGraph(F);
  if (G->node_begin() == G->node_end()) return;

  //
  // Calculate which DSNodes are reachable from globals.  If a node is reachable
  // from a global, we will create a global pool for it, so no argument passage
  // is required.
  Graphs->getGlobalsGraph();

  // Map all node reachable from this global to the corresponding nodes in
  // the globals graph.
  DSGraph::NodeMapTy GlobalsGraphNodeMapping;
  G->computeGToGGMapping(GlobalsGraphNodeMapping);

  //
  // Loop over all of the nodes which are non-escaping, adding pool-allocatable
  // ones to the NodesToPA vector.  In other words, scan over the DSGraph and
  // find nodes for which a new pool must be created within this function.
  //
  for (DSGraph::node_iterator I = G->node_begin(), E = G->node_end();
       I != E;
       ++I){
    // Get the DSNode and, if applicable, its mirror in the globals graph
    DSNode * N   = I;
    DSNode * GGN = GlobalsGraphNodeMapping[N].getNode();

    //
    // Only the following nodes are pool allocated:
    //  1) Local Heap nodes
    //  2) Nodes which are mirrored in the globals graph and, in the globals
    //     graph, are heap nodes.
    //
    if ((N->isHeapNode()) || (GGN && GGN->isHeapNode())) {
      if (!(GlobalPoolNodes.count (N) || GlobalPoolNodes.count (GGN))) {
        // Otherwise, if it was not passed in from outside the function, it must
        // be a local pool!
        assert(!N->isGlobalNode() && "Should be in global mapping!");
        Nodes.push_back (N);
      }
    }
  }

  return;
}

//===-- AllNodes Heuristic ------------------------------------------------===//
//
// This heuristic pool allocates everything possible into separate pools.
//

bool
AllNodesHeuristic::runOnModule (Module & Module) {
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
AllNodesHeuristic::AssignToPools (const std::vector<const DSNode*> &NodesToPA,
                                  Function *F, DSGraph* G,
                                  std::vector<OnePool> &ResultPools) {
  for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
    ResultPools.push_back(OnePool(NodesToPA[i]));
}

//===-- AllButUnreachableFromMemoryHeuristic Heuristic --------------------===//
//
// This heuristic pool allocates everything possible into separate pools, unless
// the pool is not reachable by other memory objects.  This filters out objects
// that are not cyclic and are only pointed to by scalars: these tend to be
// singular memory allocations that are not worth creating a whole pool for.
//
bool
AllButUnreachableFromMemoryHeuristic::runOnModule (Module & Module) {
  //
  // Remember which module we are analyzing.
  //
  M = &Module;

  //
  // Get the reference to the DSA Graph.
  //
  Graphs = &getAnalysis<EQTDDataStructures>();   

  //
  // Find DSNodes which are reachable from globals and should be pool
  // allocated.
  //
  findGlobalPoolNodes (GlobalPoolNodes);

  // We never modify anything in this pass
  return false;
}

void
AllButUnreachableFromMemoryHeuristic::AssignToPools (
                                          const DSNodeList_t &NodesToPA,
                                          Function *F, DSGraph* G,
                                          std::vector<OnePool> &ResultPools) {
  // Build a set of all nodes that are reachable from another node in the
  // graph.  Here we ignore scalar nodes that are only globals as they are
  // often global pointers to big arrays.
  std::set<const DSNode*> ReachableFromMemory;
  for (DSGraph::node_iterator I = G->node_begin(), E = G->node_end();
       I != E; ++I) {
    DSNode *N = I;
#if 0
    //
    // Ignore nodes that are just globals and not arrays.
    //
    if (N->isArray() || N->isHeapNode() || N->isAllocaNode() ||
        N->isUnknownNode())
#endif
    // If a node is marked, all children are too.
    if (!ReachableFromMemory.count(N)) {
      for (DSNode::iterator NI = N->begin(), E = N->end(); NI != E; ++NI) {
        //
        // Sometimes this results in a NULL DSNode.  Skip it if that is the
        // case.
        //
        if (!(*NI)) continue;

        //
        // Do a depth-first iteration over the DSGraph starting with this
        // child node.
        //
        for (df_ext_iterator<const DSNode*>
               DI = df_ext_begin(*NI, ReachableFromMemory),
               E = df_ext_end(*NI, ReachableFromMemory); DI != E; ++DI)
        /*empty*/;
      }
    }
  }

  // Only pool allocate a node if it is reachable from a memory object (itself
  // included).
  for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
    if (ReachableFromMemory.count(NodesToPA[i]))
      ResultPools.push_back(OnePool(NodesToPA[i]));
}

//===-- CyclicNodes Heuristic ---------------------------------------------===//
//
// This heuristic only pool allocates nodes in an SCC in the DSGraph.
//

bool
CyclicNodesHeuristic::runOnModule (Module & Module) {
  //
  // Remember which module we are analyzing.
  //
  M = &Module;

  //
  // Get the reference to the DSA Graph.
  //
  Graphs = &getAnalysis<EQTDDataStructures>();   

  //
  // Find DSNodes which are reachable from globals and should be pool
  // allocated.
  //
  findGlobalPoolNodes (GlobalPoolNodes);

  // We never modify anything in this pass
  return false;
}

static bool NodeExistsInCycle(const DSNode *N) {
  for (DSNode::const_iterator I = N->begin(), E = N->end(); I != E; ++I)
    if (*I && std::find(df_begin(*I), df_end(*I), N) != df_end(*I))
      return true;
  return false;
}

void
CyclicNodesHeuristic::AssignToPools(const DSNodeList_t &NodesToPA,
                                    Function *F, DSGraph* G,
                                    std::vector<OnePool> &ResultPools) {
  for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
    if (NodeExistsInCycle(NodesToPA[i]))
      ResultPools.push_back(OnePool(NodesToPA[i]));
}

//===-- SmartCoallesceNodes Heuristic -------------------------------------===//
//
// This heuristic attempts to be smart and coallesce nodes at times.  In
// practice, it doesn't work very well.
//
bool
SmartCoallesceNodesHeuristic::runOnModule (Module & Module) {
  //
  // Remember which module we are analyzing.
  //
  M = &Module;

  //
  // Get the reference to the DSA Graph.
  //
  Graphs = &getAnalysis<EQTDDataStructures>();   

  //
  // Find DSNodes which are reachable from globals and should be pool
  // allocated.
  //
  findGlobalPoolNodes (GlobalPoolNodes);

  // We never modify anything in this pass
  return false;
}

void
SmartCoallesceNodesHeuristic::AssignToPools(const DSNodeList_t &NodesToPA,
                                            Function *F, DSGraph* G,
                                            std::vector<OnePool> &ResultPools) {
  // For globals, do not pool allocate unless the node is cyclic and not an
  // array (unless it's collapsed).
  if (F == 0) {
    for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i) {
      const DSNode *Node = NodesToPA[i];
      if ((Node->isNodeCompletelyFolded() || !Node->isArrayNode()) &&
          NodeExistsInCycle(Node))
        ResultPools.push_back(OnePool(Node));
    }
  } else {
    // TODO
  }
}

#if 0
/// NodeIsSelfRecursive - Return true if this node contains a pointer to itself.
static bool NodeIsSelfRecursive(DSNode *N) {
  for (DSNode::iterator I = N->begin(), E = N->end(); I != E; ++I)
    if (*I == N) return true;
  return false;
}

/// POVisit - This implements functionality found in Support/PostOrderIterator.h
/// but in a way that allows multiple roots to be used.  If PostOrderIterator
/// supported an external set like DepthFirstIterator did I could eliminate this
/// cruft.
///
static void POVisit(DSNode *N, std::set<DSNode*> &Visited,
                    std::vector<DSNode*> &Order) {
  if (!Visited.insert(N).second) return;  // already visited

  // Visit all children before visiting this node.
  for (DSNode::iterator I = N->begin(), E = N->end(); I != E; ++I)
    if (DSNode *C = const_cast<DSNode*>(*I))
      POVisit(C, Visited, Order);
  // Now that we visited all of our children, add ourself to the order.
  Order.push_back(N);
}



  // Heuristic for building per-function pools

  switch (Heuristic) {
  case SmartCoallesceNodes: {
    std::set<DSNode*> NodesToPASet(NodesToPA.begin(), NodesToPA.end());

    // DSGraphs only have unidirectional edges, to traverse or inspect the
    // predecessors of nodes, we must build a mapping of the inverse graph.
    std::map<DSNode*, std::vector<DSNode*> > InverseGraph;

    for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i) {
      DSNode *Node = NodesToPA[i];
      for (DSNode::iterator CI = Node->begin(), E = Node->end(); CI != E; ++CI)
        if (DSNode *Child = const_cast<DSNode*>(*CI))
          if (NodesToPASet.count(Child))
            InverseGraph[Child].push_back(Node);
    }

    // Traverse the heap nodes in reverse-post-order so that we are guaranteed
    // to visit all nodes pointing to another node before we visit that node
    // itself (except with cycles).

    // FIXME: This really should be using the PostOrderIterator.h file stuff,
    // but the routines there do not support external storage!
    std::set<DSNode*> Visited;
    std::vector<DSNode*> Order;
    for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
      POVisit(NodesToPA[i], Visited, Order);

    // We want RPO, not PO, so reverse the order.
    std::reverse(Order.begin(), Order.end());

    // Okay, we have an ordering of the nodes in reverse post order.  Traverse
    // each node in this ordering, noting that there may be nodes in the order
    // that are not in our NodesToPA list.
    for (unsigned i = 0, e = Order.size(); i != e; ++i)
      if (NodesToPASet.count(Order[i])) {        // Only process pa nodes.
        DSNode *N = Order[i];

        // If this node has a backedge to itself, pool allocate it in a new
        // pool.
        if (NodeIsSelfRecursive(N)) {
          // Create a new alloca instruction for the pool...
          Value *AI = new AllocaInst(PoolDescType, 0, "PD", InsertPoint);
        
          // Void types in DS graph are never used
          if (N->isNodeCompletelyFolded())
            std::cerr << "Node collapsing in '" << F.getName() << "'\n";
        
          // Update the PoolDescriptors map
          PoolDescriptors.insert(std::make_pair(N, AI));
#if 1
        } else if (N->isArray() && !N->isNodeCompletelyFolded()) {
          // We never pool allocate array nodes.
          PoolDescriptors[N] =
            Constant::getNullValue(PointerType::getUnqual(PoolDescType));
          ++NumNonprofit;
#endif
        } else {
          // Otherwise the node is not self recursive.  If the node is not an
          // array, we can co-locate it with the pool of a predecessor node if
          // any has been pool allocated, and start a new pool if a predecessor
          // is an array.  If there is a predecessor of this node that has not
          // been visited yet in this RPO traversal, that means there is a
          // cycle, so we choose to pool allocate this node right away.
          //
          // If there multiple predecessors in multiple different pools, we
          // don't pool allocate this at all.

          // Check out each of the predecessors of this node.
          std::vector<DSNode*> &Preds = InverseGraph[N];
          Value *PredPool = 0;
          bool HasUnvisitedPred     = false;
          bool HasArrayPred         = false;
          bool HasMultiplePredPools = false;
          for (unsigned p = 0, e = Preds.size(); p != e; ++p) {
            DSNode *Pred = Preds[p];
            if (!PoolDescriptors.count(Pred))
              HasUnvisitedPred = true;  // no pool assigned to predecessor?
            else if (Pred->isArray() && !Pred->isNodeCompletelyFolded())
              HasArrayPred = true;
            else if (PredPool && PoolDescriptors[Pred] != PredPool)
              HasMultiplePredPools = true;
            else if (!PredPool &&
                     !isa<ConstantPointerNull>(PoolDescriptors[Pred]))
              PredPool = PoolDescriptors[Pred];
            // Otherwise, this predecessor has the same pool as a previous one.
          }

          if (HasMultiplePredPools) {
            // If this node has predecessors that are in different pools, don't
            // pool allocate this node.
            PoolDescriptors[N] =
              Constant::getNullValue(PointerType::getUnqual(PoolDescType));
            ++NumNonprofit;
          } else if (PredPool) {
            // If all of the predecessors of this node are already in a pool,
            // colocate.
            PoolDescriptors[N] = PredPool;
            ++NumColocated;
          } else if (HasArrayPred || HasUnvisitedPred) {
            // If this node has an array predecessor, or if there is a
            // predecessor that has not been visited yet, allocate a new pool
            // for it.
            Value *AI = new AllocaInst(PoolDescType, 0, "PD", InsertPoint);
            if (N->isNodeCompletelyFolded())
              std::cerr << "Node collapsing in '" << F.getName() << "'\n";

            PoolDescriptors[N] = AI;
          } else {
            // If this node has no pool allocated predecessors, and there is no
            // reason to pool allocate it, don't.
            assert(PredPool == 0);
             PoolDescriptors[N] =
              Constant::getNullValue(PointerType::getUnqual(PoolDescType));
            ++NumNonprofit;
          }
        }
      }
  }  // End switch case
  }  // end switch
#endif


//===-- AllInOneGlobalPool Heuristic --------------------------------------===//
//
// This heuristic puts all memory in the whole program into a single global
// pool.  This is not safe, and is not good for performance, but can be used to
// evaluate how good the pool allocator runtime works as a "malloc replacement".
//
bool
AllInOneGlobalPoolHeuristic::runOnModule (Module & Module) {
  //
  // Remember which module we are analyzing.
  //
  M = &Module;

  //
  // Get the reference to the DSA Graph.
  //
  Graphs = &getAnalysis<EQTDDataStructures>();   

  //
  // Find DSNodes which are reachable from globals and should be pool
  // allocated.
  //
  findGlobalPoolNodes (GlobalPoolNodes);

  // We never modify anything in this pass
  return false;
}

void
AllInOneGlobalPoolHeuristic::AssignToPools(const DSNodeList_t &NodesToPA,
                                           Function *F, DSGraph* G,
                                           std::vector<OnePool> &ResultPools) {
  if (TheGlobalPD == 0)
    TheGlobalPD = PA->CreateGlobalPool(0, 0);

  // All nodes allocate from the same global pool.
  OnePool Pool;
  Pool.NodesInPool = NodesToPA;
  Pool.PoolDesc = TheGlobalPD;
  ResultPools.push_back(Pool);
}

//===-- OnlyOverhead Heuristic --------------------------------------------===//
//
// This heuristic is a hack to evaluate how much overhead pool allocation adds
// to a program.  It adds all of the arguments, poolinits and pool destroys to
// the program, but dynamically only passes null into the pool alloc/free
// functions, causing them to allocate from the heap.
//
bool
OnlyOverheadHeuristic::runOnModule (Module & Module) {
  //
  // Remember which module we are analyzing.
  //
  M = &Module;

  //
  // Get the reference to the DSA Graph.
  //
  Graphs = &getAnalysis<EQTDDataStructures>();   

  //
  // Find DSNodes which are reachable from globals and should be pool
  // allocated.
  //
  findGlobalPoolNodes (GlobalPoolNodes);

  // We never modify anything in this pass
  return false;
}

void
OnlyOverheadHeuristic::AssignToPools(const DSNodeList_t &NodesToPA,
                                     Function *F, DSGraph* G,
                                     std::vector<OnePool> &ResultPools) {
  // For this heuristic, we assign everything possible to its own pool.
  for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
    ResultPools.push_back(OnePool(NodesToPA[i]));
}

/// getDynamicallyNullPool - Return a PoolDescriptor* that is always dynamically
/// null.  Insert the code necessary to produce it before the specified
/// instruction.
static Value *
getDynamicallyNullPool(BasicBlock::iterator I) {
  // Arrange to dynamically pass null into all of the pool functions if we are
  // only checking for overhead.
  static Value *NullGlobal = 0;
  if (!NullGlobal) {
    Module *M = I->getParent()->getParent()->getParent();
    const Type * PoolTy = PoolAllocate::PoolDescPtrTy;
    Constant * Init = ConstantPointerNull::get(cast<PointerType>(PoolTy));
    NullGlobal = new GlobalVariable(*M,
                                    PoolAllocate::PoolDescPtrTy, false,
                                    GlobalValue::ExternalLinkage,
                                    Init,
                                    "llvm-poolalloc-null-init");
  }
  while (isa<AllocaInst>(I)) ++I;

  return new LoadInst(NullGlobal, "nullpd", I);
}

// HackFunctionBody - This method is called on every transformed function body.
// Basically it replaces all uses of real pool descriptors with dynamically null
// values.  However, it leaves pool init/destroy alone.
void
OnlyOverheadHeuristic::HackFunctionBody(Function &F,
                                        std::map<const DSNode*, Value*> &PDs) {
  Constant *PoolInit = PA->PoolInit;
  Constant *PoolDestroy = PA->PoolDestroy;

  Value *NullPD = getDynamicallyNullPool(F.front().begin());
  for (std::map<const DSNode*, Value*>::iterator PDI = PDs.begin(),
         E = PDs.end(); PDI != E; ++PDI) {
    Value *OldPD = PDI->second;
    std::vector<User*> OldPDUsers(OldPD->use_begin(), OldPD->use_end());
    for (unsigned i = 0, e = OldPDUsers.size(); i != e; ++i) {
      CallSite PDUser = CallSite::get(cast<Instruction>(OldPDUsers[i]));
      if (PDUser.getCalledValue() != PoolInit &&
          PDUser.getCalledValue() != PoolDestroy) {
        assert(PDUser.getInstruction()->getParent()->getParent() == &F &&
               "Not in cur fn??");
        PDUser.getInstruction()->replaceUsesOfWith(OldPD, NullPD);
      }
    }
  }
}


//===-- NoNodes Heuristic -------------------------------------------------===//
//
// This dummy heuristic chooses to not pool allocate anything.
//
bool
NoNodesHeuristic::runOnModule (Module & Module) {
  //
  // Remember which module we are analyzing.
  //
  M = &Module;

  // We never modify anything in this pass
  return false;
}

//
// Register all of the heuristic passes.
//
static RegisterPass<AllNodesHeuristic>
A ("paheur-AllNodes", "Pool allocate all nodes");

static RegisterPass<AllButUnreachableFromMemoryHeuristic>
B ("paheur-AllButUnreachableFromMemory", "Pool allocate all reachable from memory objects");

static RegisterPass<CyclicNodesHeuristic>
C ("paheur-CyclicNodes", "Pool allocate nodes with cycles");

static RegisterPass<SmartCoallesceNodesHeuristic>
D ("paheur-SmartCoallesceNodes", "Pool allocate using the smart node merging heuristic ");

static RegisterPass<AllInOneGlobalPoolHeuristic>
E ("paheur-AllInOneGlobalPool", "Pool allocate using the pool library to replace malloc/free");

static RegisterPass<OnlyOverheadHeuristic>
F ("paheur-OnlyOverhead", "Do not pool allocate anything, but induce all overhead from it");

static RegisterPass<NoNodesHeuristic>
G ("paheur-NoNodes", "Pool allocate nothing heuristic");

//
// Create the heuristic analysis group.
//
static RegisterAnalysisGroup<Heuristic>
HeuristicGroup ("Pool Allocation Heuristic");

RegisterAnalysisGroup<Heuristic> Heuristic1(A);
RegisterAnalysisGroup<Heuristic> Heuristic2(B);
RegisterAnalysisGroup<Heuristic> Heuristic3(C);
RegisterAnalysisGroup<Heuristic> Heuristic4(D);
RegisterAnalysisGroup<Heuristic> Heuristic5(E);
RegisterAnalysisGroup<Heuristic> Heuristic6(F);
RegisterAnalysisGroup<Heuristic, true> Heuristic7(G);

char Heuristic::ID = 0;
char AllNodesHeuristic::ID = 0;
char AllButUnreachableFromMemoryHeuristic::ID = 0;
char CyclicNodesHeuristic::ID = 0;
char SmartCoallesceNodesHeuristic::ID = 0;
char AllInOneGlobalPoolHeuristic::ID = 0;
char OnlyOverheadHeuristic::ID = 0;
char NoNodesHeuristic::ID = 0;


