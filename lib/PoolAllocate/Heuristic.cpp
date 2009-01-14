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
//===----------------------------------------------------------------------===//

#include "Heuristic.h"

#include "dsa/DSGraphTraits.h"
#include "poolalloc/PoolAllocate.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetData.h"
#include <iostream>
using namespace llvm;
using namespace PA;

namespace {
  enum PoolAllocHeuristic {
    NoNodes,
    OnlyOverhead,
    AllInOneGlobalPool,
    SmartCoallesceNodes,
    CyclicNodes,
    AllButUnreachableFromMemory,
    AllNodes
  };
  cl::opt<PoolAllocHeuristic>
  TheHeuristic("poolalloc-heuristic",
    cl::desc("Heuristic to choose which nodes to pool allocate"),
    cl::values(clEnumVal(AllNodes, "  Pool allocate all nodes"),
               clEnumVal(AllButUnreachableFromMemory, "  Pool allocate all reachable from memory objects"),
               clEnumVal(CyclicNodes, "  Pool allocate nodes with cycles"),
               clEnumVal(SmartCoallesceNodes, "  Use the smart node merging heuristic"),
               clEnumVal(AllInOneGlobalPool, "  Use pool library as replacement for malloc/free"),
               clEnumVal(OnlyOverhead, "  Do not pool allocate anything, but induce all overhead from it"),
               clEnumVal(NoNodes, "  Do not pool allocate anything"),
               clEnumValEnd),
    cl::init(AllButUnreachableFromMemory)); 

  cl::opt<bool>
  DisableAlignOpt("poolalloc-disable-alignopt",
                  cl::desc("Force all pool alignment to 8 bytes"));
}

Heuristic::~Heuristic() {}

unsigned Heuristic::getRecommendedSize(const DSNode *N) {
  unsigned PoolSize = 0;
  if (!N->isArray() && N->getType()->isSized()) {
    PoolSize = N->getParentGraph()->getTargetData().getTypePaddedSize(N->getType());
  }
  if (PoolSize == 1) PoolSize = 0;
  return PoolSize;
}

/// Wants8ByteAlignment - FIXME: this is a complete hack for X86 right now.
static bool Wants8ByteAlignment(const Type *Ty, unsigned Offs,
                                const TargetData &TD) {
  if (DisableAlignOpt) return true;

  if ((Offs & 7) == 0) {
    // Doubles always want to be 8-byte aligned.
    if (Ty == Type::DoubleTy) return true;
    
    // If we are on a 64-bit system, we want to align 8-byte integers and
    // pointers.
    if (TD.getPrefTypeAlignment(Ty) == 8)
      return true;
  }

  if (Ty->isFirstClassType())
    return false;

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
    std::cerr << *Ty << "\n";
    assert(0 && "Unknown type!");
  }
  return false;
}

unsigned Heuristic::getRecommendedAlignment(const Type *Ty,
                                            const TargetData &TD) {
  if (Ty == Type::VoidTy)  // Is this void or collapsed?
    return 0;  // No known alignment, let runtime decide.

  return Wants8ByteAlignment(Ty, 0, TD) ? 8 : 4;
}

/// getRecommendedAlignment - Return the recommended object alignment for this
/// DSNode.
///
unsigned Heuristic::getRecommendedAlignment(const DSNode *N) {
  if (N->getType() == Type::VoidTy)  // Is this void or collapsed?
    return 0;  // No known alignment, let runtime decide.

  const TargetData &TD = N->getParentGraph()->getTargetData();

  // If there are no doubles on an 8-byte boundary in this structure, there is
  // no reason to 8-byte align objects in the pool.
  return Wants8ByteAlignment(N->getType(), 0, TD) ? 8 : 4;
}
 

//===-- AllNodes Heuristic ------------------------------------------------===//
//
// This heuristic pool allocates everything possible into separate pools.
//
struct AllNodesHeuristic : public Heuristic {

  void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
                     Function *F, DSGraph* G,
                     std::vector<OnePool> &ResultPools) {
    for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
      ResultPools.push_back(OnePool(NodesToPA[i]));
  }
};


//===-- AllButUnreachableFromMemoryHeuristic Heuristic --------------------===//
//
// This heuristic pool allocates everything possible into separate pools, unless
// the pool is not reachable by other memory objects.  This filters out objects
// that are not cyclic and are only pointed to by scalars: these tend to be
// singular memory allocations that are not worth creating a whole pool for.
//
struct AllButUnreachableFromMemoryHeuristic : public Heuristic {

  void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
                     Function *F, DSGraph* G,
                     std::vector<OnePool> &ResultPools) {
    // Build a set of all nodes that are reachable from another node in the
    // graph.  Here we ignore scalar nodes that are only globals as they are
    // often global pointers to big arrays.
    std::set<const DSNode*> ReachableFromMemory;
    for (DSGraph::node_iterator I = G->node_begin(), E = G->node_end();
         I != E; ++I) {
      DSNode *N = I;
      // Ignore nodes that are just globals and not arrays.
      /*
      if (N->isArray() || N->isHeapNode() || N->isAllocaNode() ||
          N->isUnknownNode())
      */
      // If a node is marked, all children are too.
      if (!ReachableFromMemory.count(N))
        for (DSNode::iterator NI = N->begin(), E = N->end(); NI != E; ++NI)
          for (df_ext_iterator<const DSNode*>
                 DI = df_ext_begin(*NI, ReachableFromMemory),
                 E = df_ext_end(*NI, ReachableFromMemory); DI != E; ++DI)
          /*empty*/;
    }

    // Only pool allocate a node if it is reachable from a memory object (itself
    // included).
    for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
      if (ReachableFromMemory.count(NodesToPA[i]))
        ResultPools.push_back(OnePool(NodesToPA[i]));
  }
};

//===-- CyclicNodes Heuristic ---------------------------------------------===//
//
// This heuristic only pool allocates nodes in an SCC in the DSGraph.
//
struct CyclicNodesHeuristic : public Heuristic {

  void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
                     Function *F, DSGraph* G,
                     std::vector<OnePool> &ResultPools);
};

static bool NodeExistsInCycle(const DSNode *N) {
  for (DSNode::const_iterator I = N->begin(), E = N->end(); I != E; ++I)
    if (*I && std::find(df_begin(*I), df_end(*I), N) != df_end(*I))
      return true;
  return false;
}

void CyclicNodesHeuristic::AssignToPools(const std::vector<const 
                                                           DSNode*> &NodesToPA,
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
struct SmartCoallesceNodesHeuristic : public Heuristic {

  void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
                    Function *F, DSGraph* G,
                    std::vector<OnePool> &ResultPools) {
    // For globals, do not pool allocate unless the node is cyclic and not an
    // array (unless it's collapsed).
    if (F == 0) {
      for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i) {
        const DSNode *Node = NodesToPA[i];
        if ((Node->isNodeCompletelyFolded() || !Node->isArray()) &&
            NodeExistsInCycle(Node))
          ResultPools.push_back(OnePool(Node));
      }
    } else {
      // TODO
    }
  }
};

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
struct AllInOneGlobalPoolHeuristic : public Heuristic {
  // TheGlobalPD - This global pool is the one and only one used when running
  // with Heuristic=AllInOneGlobalPool.
  GlobalVariable *TheGlobalPD;

  AllInOneGlobalPoolHeuristic() : TheGlobalPD(0) {}


  virtual bool IsRealHeuristic() { return false; }

  void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
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
};

//===-- OnlyOverhead Heuristic --------------------------------------------===//
//
// This heuristic is a hack to evaluate how much overhead pool allocation adds
// to a program.  It adds all of the arguments, poolinits and pool destroys to
// the program, but dynamically only passes null into the pool alloc/free
// functions, causing them to allocate from the heap.
//
struct OnlyOverheadHeuristic : public Heuristic {
  virtual bool IsRealHeuristic() { return false; }

  void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
                    Function *F, DSGraph* G,
                    std::vector<OnePool> &ResultPools) {
    // For this heuristic, we assign everything possible to its own pool.
    for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i)
      ResultPools.push_back(OnePool(NodesToPA[i]));
  }

  void HackFunctionBody(Function &F, std::map<const DSNode*, Value*> &PDs);
};

/// getDynamicallyNullPool - Return a PoolDescriptor* that is always dynamically
/// null.  Insert the code necessary to produce it before the specified
/// instruction.
static Value *getDynamicallyNullPool(BasicBlock::iterator I) {
  // Arrange to dynamically pass null into all of the pool functions if we are
  // only checking for overhead.
  static Value *NullGlobal = 0;
  if (!NullGlobal) {
    Module *M = I->getParent()->getParent()->getParent();
    NullGlobal = new GlobalVariable(PoolAllocate::PoolDescPtrTy, false,
                                    GlobalValue::ExternalLinkage,
                         Constant::getNullValue(PoolAllocate::PoolDescPtrTy),
                                    "llvm-poolalloc-null-init", M);
  }
  while (isa<AllocaInst>(I)) ++I;

  return new LoadInst(NullGlobal, "nullpd", I);
}

// HackFunctionBody - This method is called on every transformed function body.
// Basically it replaces all uses of real pool descriptors with dynamically null
// values.  However, it leaves pool init/destroy alone.
void OnlyOverheadHeuristic::HackFunctionBody(Function &F,
                                             std::map<const DSNode*,
                                             Value*> &PDs) {
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
struct NoNodesHeuristic : public Heuristic {
  virtual bool IsRealHeuristic() { return false; }

  void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
                    Function *F, DSGraph* G,
                    std::vector<OnePool> &ResultPools) {
    // Nothing to pool allocate here.
  }
};

//===----------------------------------------------------------------------===//
// Heuristic dispatch support
//

PA::Heuristic *Heuristic::create() {
  switch (TheHeuristic) {
  default: assert(0 && "Unknown heuristic!");
  case AllNodes: return new AllNodesHeuristic();
  case AllButUnreachableFromMemory:
    return new AllButUnreachableFromMemoryHeuristic();
  case CyclicNodes: return new CyclicNodesHeuristic();
  case SmartCoallesceNodes: return new SmartCoallesceNodesHeuristic();
  case AllInOneGlobalPool: return new AllInOneGlobalPoolHeuristic();
  case OnlyOverhead: return new OnlyOverheadHeuristic();
  case NoNodes: return new NoNodesHeuristic();
  }
}
