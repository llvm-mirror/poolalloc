//===-- Heuristic.h - Interface to PA heuristics ----------------*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This header is the abstract interface used by the pool allocator to access
// the various heuristics supported.
//
//===----------------------------------------------------------------------===//

#ifndef POOLALLOC_HEURISTIC_H
#define POOLALLOC_HEURISTIC_H

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"

#include "llvm/Pass.h"

#include <vector>
#include <map>

namespace llvm {
  class Value;
  class Function;
  class Module;
  class DSGraph;
  class DSNode;
  class PoolAllocate;
  class TargetData;
  class Type;

namespace PA {
  class Heuristic : public ImmutablePass {
  protected:
    Module *M;
    PoolAllocate *PA;

  public:
    //
    // Pass methods and members.
    //
    static char ID;
    Heuristic (intptr_t IDp = (intptr_t) (&ID)) : ImmutablePass (IDp) { }
    virtual ~Heuristic () {return;}
    virtual bool runOnModule (Module & M);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      // We require DSA while this pass is still responding to queries
      AU.addRequiredTransitive<EQTDDataStructures>();

      // This pass does not modify anything when it runs
      AU.setPreservesAll();
    }

    //
    // Other methods.
    //
    void Initialize (PoolAllocate &pa) {
      PA = &pa;
    }

    /// IsRealHeuristic - Return true if this is not a real pool allocation
    /// heuristic.
    virtual bool IsRealHeuristic() { return true; }

    /// OnePool - This represents some number of nodes which are coallesced into
    /// a pool.
    struct OnePool {
      // NodesInPool - The DS nodes to be allocated to this pool.  There may be
      // multiple here if they are being coallesced into the same pool.
      std::vector<const DSNode*> NodesInPool;

      // PoolDesc - If the heuristic wants the nodes allocated to a specific
      // pool descriptor, it can specify it here, otherwise a new pool is
      // created.
      Value *PoolDesc;

      // PoolSize - If the pool is to be created, indicate the "recommended
      // size" for the pool here.  This gets passed into poolinit.
      unsigned PoolSize;
      unsigned PoolAlignment;

      OnePool() : PoolDesc(0), PoolSize(0), PoolAlignment(0) {}

      OnePool(const DSNode *N) : PoolDesc(0), PoolSize(getRecommendedSize(N)), 
                                 PoolAlignment(getRecommendedAlignment(N)) {
        NodesInPool.push_back(N);
      }
      OnePool(const DSNode *N, Value *PD) : PoolDesc(PD), PoolSize(0),
                                            PoolAlignment(0) {
        NodesInPool.push_back(N);
      }
    };

    /// AssignToPools - Partition NodesToPA into a set of disjoint pools,
    /// returning the result in ResultPools.  If this is a function being pool
    /// allocated, F will not be null.
    virtual void AssignToPools(const std::vector<const DSNode*> &NodesToPA,
                               Function *F, DSGraph* G,
                               std::vector<OnePool> &ResultPools) = 0;

    // Hacks for the OnlyOverhead heuristic.
    virtual void HackFunctionBody(Function &F,
                                  std::map<const DSNode*, Value*> &PDs) {}

    /// getRecommendedSize - Return the recommended pool size for this DSNode.
    ///
    static unsigned getRecommendedSize(const DSNode *N);

    /// getRecommendedAlignment - Return the recommended object alignment for
    /// this DSNode.
    ///
    static unsigned getRecommendedAlignment(const DSNode *N);
    static unsigned getRecommendedAlignment(const Type *Ty,
                                            const TargetData &TD);
  };
}
}

#endif
