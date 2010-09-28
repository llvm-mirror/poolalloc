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
  // Type for a container of DSNodes
  typedef std::vector<const DSNode*> DSNodeList_t;

  //
  // Class: Heuristic
  //
  // Description:
  //  This class is a base class for passes implementing a policy for automatic
  //  pool allocation.
  //
  class Heuristic {
  protected:
    Module *M;
    PoolAllocate *PA;
    DataStructures *Graphs;

  public:
    // Virtual Destructor
    virtual ~Heuristic() {}

    // Need by passes that inherit from this class.  This is needed even though
    // this class is not an LLVM pass in and of itself.
    static char ID;
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

    /// Find globally reachable DSNodes that need a pool
    virtual void findGlobalPoolNodes (std::vector<const DSNode *> & Nodes);

    /// AssignToPools - Partition NodesToPA into a set of disjoint pools,
    /// returning the result in ResultPools.  If this is a function being pool
    /// allocated, F will not be null.
    virtual void AssignToPools(const DSNodeList_t & NodesToPA,
                               Function *F, DSGraph* G,
                               std::vector<OnePool> &ResultPools) { }

    // Hacks for the OnlyOverhead heuristic.
    virtual void HackFunctionBody(Function &F,
                                  std::map<const DSNode*, Value*> &PDs) { }

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

  ////////////////////////////////////////////////////////////////////////////
  // Define specific instances of this analysis and make them analysis passes
  ////////////////////////////////////////////////////////////////////////////

  //
  // Class: AllNodesHeuristic 
  //
  // Description:
  //  This class provides a pool allocation heuristic that forces all DSNodes
  //  to be pool allocated.
  //
  class AllNodesHeuristic: public Heuristic, public ModulePass {
    public:
      // Pass ID
      static char ID;

      // Method used to implement analysis groups without C++ inheritance
      virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {
        if (PI->isPassID(&Heuristic::ID))
          return (Heuristic*)this;
        return this;
      }

      AllNodesHeuristic (intptr_t IDp = (intptr_t) (&ID)): ModulePass (IDp) { }
      virtual ~AllNodesHeuristic () {return;}
      virtual bool runOnModule (Module & M);
      virtual const char * getPassName () const {
        return "All Nodes Pool Allocation Heurisitic";
      }

      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We require DSA while this pass is still responding to queries
        AU.addRequiredTransitive<EQTDDataStructures>();

        // Make PassManager happy be requiring the default implementation of
        // this analysis group
        AU.addRequiredTransitive<Heuristic>();

        // This pass does not modify anything when it runs
        AU.setPreservesAll();
      }

      //
      // Interface methods
      //
      virtual void AssignToPools (const DSNodeList_t & NodesToPA,
                                  Function *F, DSGraph* G,
                                  std::vector<OnePool> &ResultPools);
  };

  //===-- AllButUnreachableFromMemoryHeuristic Heuristic ------------------===//
  //
  // This heuristic pool allocates everything possible into separate pools,
  // unless the pool is not reachable by other memory objects.  This filters
  // out objects that are not cyclic and are only pointed to by scalars: these
  // tend to be singular memory allocations for which it is not worth creating
  // a whole pool.
  //
  class AllButUnreachableFromMemoryHeuristic : public Heuristic,
                                               public ModulePass {
    public:
      static char ID;
      virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {
        if (PI->isPassID(&Heuristic::ID))
          return (Heuristic*)this;
        return this;
      }

      AllButUnreachableFromMemoryHeuristic (intptr_t IDp = (intptr_t) (&ID)) :
        ModulePass (IDp) { }
      virtual ~AllButUnreachableFromMemoryHeuristic () {return;}
      virtual bool runOnModule (Module & M);

      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We require DSA while this pass is still responding to queries
        AU.addRequiredTransitive<EQTDDataStructures>();

        // Make PassManager happy be requiring the default implementation of
        // this analysis group
        AU.addRequiredTransitive<Heuristic>();

        // This pass does not modify anything when it runs
        AU.setPreservesAll();
      }

      virtual void AssignToPools(const DSNodeList_t &NodesToPA,
                                 Function *F, DSGraph* G,
                                 std::vector<OnePool> &ResultPools);
  };

  //===-- CyclicNodes Heuristic -------------------------------------------===//
  //
  // This heuristic only pool allocates nodes in an SCC in the DSGraph.
  //
  class CyclicNodesHeuristic : public Heuristic, public ModulePass {
    public:
      static char ID;
      virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {
        if (PI->isPassID(&Heuristic::ID))
          return (Heuristic*)this;
        return this;
      }


      CyclicNodesHeuristic (intptr_t IDp=(intptr_t) (&ID)): ModulePass (IDp) { }
      virtual ~CyclicNodesHeuristic () {return;}
      virtual bool runOnModule (Module & M);

      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We require DSA while this pass is still responding to queries
        AU.addRequiredTransitive<EQTDDataStructures>();

        // Make PassManager happy be requiring the default implementation of
        // this analysis group
        AU.addRequiredTransitive<Heuristic>();

        // This pass does not modify anything when it runs
        AU.setPreservesAll();
      }

      virtual void AssignToPools(const DSNodeList_t &NodesToPA,
                                 Function *F, DSGraph* G,
                                 std::vector<OnePool> &ResultPools);
  };

  //===-- SmartCoallesceNodes Heuristic------------------------------------===//
  //
  // This heuristic attempts to be smart and coallesce nodes at times.  In
  // practice, it doesn't work very well.
  //
  class SmartCoallesceNodesHeuristic : public Heuristic, public ModulePass {
    public:
      static char ID;
      virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {
        if (PI->isPassID(&Heuristic::ID))
          return (Heuristic*)this;
        return this;
      }

      SmartCoallesceNodesHeuristic (intptr_t IDp = (intptr_t) (&ID)) :
        ModulePass (IDp) { }
      virtual ~SmartCoallesceNodesHeuristic () {return;}
      virtual bool runOnModule (Module & M);

      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We require DSA while this pass is still responding to queries
        AU.addRequiredTransitive<EQTDDataStructures>();

        // Make PassManager happy be requiring the default implementation of
        // this analysis group
        AU.addRequiredTransitive<Heuristic>();

        // This pass does not modify anything when it runs
        AU.setPreservesAll();
      }

      virtual void AssignToPools(const DSNodeList_t & NodesToPA,
                                 Function *F, DSGraph* G,
                                 std::vector<OnePool> &ResultPools);
  };

  //===-- AllInOneGlobalPool Heuristic ------------------------------------===//
  //
  // This heuristic puts all memory in the whole program into a single global
  // pool.  This is not safe, and is not good for performance, but can be used
  // to evaluate how good the pool allocator runtime works as a "malloc
  // replacement".
  //
  class AllInOneGlobalPoolHeuristic : public Heuristic, public ModulePass {
    private:
      // TheGlobalPD - This global pool is the one and only one used when
      // running with Heuristic=AllInOneGlobalPool.
      GlobalVariable *TheGlobalPD;

    public:
      static char ID;
      virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {
        if (PI->isPassID(&Heuristic::ID))
          return (Heuristic*)this;
        return this;
      }

      AllInOneGlobalPoolHeuristic(intptr_t IDp = (intptr_t) (&ID)) :
        ModulePass (IDp), TheGlobalPD(0) {}

      virtual bool runOnModule (Module & M);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We require DSA while this pass is still responding to queries
        AU.addRequiredTransitive<EQTDDataStructures>();

        // Make PassManager happy be requiring the default implementation of
        // this analysis group
        AU.addRequiredTransitive<Heuristic>();

        // This pass does not modify anything when it runs
        AU.setPreservesAll();
      }

      virtual void AssignToPools(const DSNodeList_t &NodesToPA,
                                 Function *F, DSGraph* G,
                                 std::vector<OnePool> &ResultPools);
  };

  //===-- OnlyOverhead Heuristic ------------------------------------------===//
  //
  // This heuristic is a hack to evaluate how much overhead pool allocation adds
  // to a program.  It adds all of the arguments, poolinits and pool destroys to
  // the program, but dynamically only passes null into the pool alloc/free
  // functions, causing them to allocate from the heap.
  //
  class OnlyOverheadHeuristic : public Heuristic, public ModulePass {
    public:
      static char ID;
      virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {
        if (PI->isPassID(&Heuristic::ID))
          return (Heuristic*)this;
        return this;
      }

      OnlyOverheadHeuristic(intptr_t IDp = (intptr_t) (&ID)) :
        ModulePass (IDp) {}

      virtual bool runOnModule (Module & M);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // We require DSA while this pass is still responding to queries
        AU.addRequiredTransitive<EQTDDataStructures>();

        // Make PassManager happy be requiring the default implementation of
        // this analysis group
        AU.addRequiredTransitive<Heuristic>();

        // This pass does not modify anything when it runs
        AU.setPreservesAll();
      }

      virtual void AssignToPools(const DSNodeList_t &NodesToPA,
                                 Function *F, DSGraph* G,
                                 std::vector<OnePool> &ResultPools);

      virtual void HackFunctionBody(Function &F, std::map<const DSNode*, Value*> &PDs);
  };

  //===-- NoNodes Heuristic -----------------------------------------------===//
  //
  // This dummy heuristic chooses to not pool allocate anything.
  //
  class NoNodesHeuristic : public Heuristic, public ImmutablePass {
    public:
      static char ID;
      virtual void *getAdjustedAnalysisPointer(const PassInfo *PI) {
        if (PI->isPassID(&Heuristic::ID))
          return (Heuristic*)this;
        return this;
      }

      NoNodesHeuristic(intptr_t IDp = (intptr_t) (&ID)) : ImmutablePass (IDp) {}

      virtual bool runOnModule (Module & M);
      virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        // This pass does not modify anything when it runs
        AU.setPreservesAll();
      }
      virtual void AssignToPools(const DSNodeList_t &NodesToPA,
                                 Function *F, DSGraph* G,
                                 std::vector<OnePool> &ResultPools) { }
  };
}
}
#endif
