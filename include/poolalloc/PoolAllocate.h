//===-- PoolAllocate.h - Pool allocation pass -------------------*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This transform changes programs so that disjoint data structures are
// allocated out of different pools of memory, increasing locality.  This header
// file exposes information about the pool allocation itself so that follow-on
// passes may extend or use the pool allocation for analysis.
//
//===----------------------------------------------------------------------===//

#ifndef POOLALLOCATE_H
#define POOLALLOCATE_H

#include "llvm/Pass.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/CallSite.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/VectorExtras.h"
#include "llvm/ADT/hash_set"
#include <set>

namespace llvm {

class DSNode;
class DSGraph;
class Type;
class AllocaInst;

namespace PA {

  class EquivClassGraphs;
  class Heuristic;

  /// FuncInfo - Represent the pool allocation information for one function in
  /// the program.  Note that many functions must actually be cloned in order
  /// for pool allocation to add arguments to the function signature.  In this
  /// case, the Clone and NewToOldValueMap information identify how the clone
  /// maps to the original function...
  ///
  struct FuncInfo {
    FuncInfo(Function &f) : F(f), Clone(0) {}

    /// MarkedNodes - The set of nodes which are not locally pool allocatable in
    /// the current function.
    ///
    hash_set<const DSNode*> MarkedNodes;

    /// F - The function this FuncInfo corresponds to.
    ///
    Function &F;

    /// Clone - The cloned version of the function, if applicable.
    ///
    Function *Clone;

    /// ArgNodes - The list of DSNodes which have pools passed in as arguments.
    /// 
    std::vector<const DSNode*> ArgNodes;

    /// NodesToPA - The list of nodes which are to be pool allocated locally in
    /// this function.  This only includes heap nodes.
    std::vector<const DSNode*> NodesToPA;

    /// PoolDescriptors - The Value* which defines the pool descriptor for this
    /// DSNode.  Note: This does not necessarily include pool arguments that are
    /// passed in because of indirect function calls that are not used in the
    /// function.
    std::map<const DSNode*, Value*> PoolDescriptors;

    /// NewToOldValueMap - When and if a function needs to be cloned, this map
    /// contains a mapping from all of the values in the new function back to
    /// the values they correspond to in the old function.
    ///
    typedef std::map<Value*, const Value*> NewToOldValueMapTy;
    NewToOldValueMapTy NewToOldValueMap;

    /// MapValueToOriginal - Given a value in the cloned version of this
    /// function, map it back to the original.  If the specified value did not
    /// exist in the original function (e.g. because it's a pool descriptor),
    /// return null.
    Value *MapValueToOriginal(Value *V) const {
      NewToOldValueMapTy::const_iterator I = NewToOldValueMap.find(V);
      return I != NewToOldValueMap.end() ? const_cast<Value*>(I->second) : 0;
    }
  };

}; // end PA namespace



/// PoolAllocate - The main pool allocation pass
///
class PoolAllocate : public ModulePass {
  /// PassAllArguments - If set to true, we should pass pool descriptor
  /// arguments into any function that loads or stores to a pool, in addition to
  /// those functions that allocate or deallocate.  See also the
  /// PoolAllocatePassAllPools pass below.
  bool PassAllArguments;

  Module *CurModule;
  PA::EquivClassGraphs *ECGraphs;

  std::map<Function*, PA::FuncInfo> FunctionInfo;
  std::map<Function*, Function*> CloneToOrigMap;
public:

  Function *PoolInit, *PoolDestroy, *PoolAlloc, *PoolRealloc, *PoolMemAlign;
  Function *PoolFree;
  static const Type *PoolDescPtrTy;

  PA::Heuristic *CurHeuristic;

  /// GlobalNodes - For each node (with an H marker) in the globals graph, this
  /// map contains the global variable that holds the pool descriptor for the
  /// node.
  std::map<const DSNode*, Value*> GlobalNodes;

 public:
  PoolAllocate(bool passAllArguments = false) 
    : PassAllArguments(passAllArguments) {}

  bool runOnModule(Module &M);
  
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  
  PA::EquivClassGraphs &getECGraphs() const { return *ECGraphs; }
  
  /// getOrigFunctionFromClone - Given a pointer to a function that was cloned
  /// from another function, return the original function.  If the argument
  /// function is not a clone, return null.
  Function *getOrigFunctionFromClone(Function *F) const {
    std::map<Function*, Function*>::const_iterator I = CloneToOrigMap.find(F);
    return I != CloneToOrigMap.end() ? I->second : 0;
  }

  /// getFuncInfo - Return the FuncInfo object for the specified function.
  ///
  PA::FuncInfo *getFuncInfo(Function &F) {
    std::map<Function*, PA::FuncInfo>::iterator I = FunctionInfo.find(&F);
    return I != FunctionInfo.end() ? &I->second : 0;
  }

  /// getFuncInfoOrClone - Return the function info object for for the specified
  /// function.  If this function is a clone of another function, return the
  /// function info object for the original function.
  PA::FuncInfo *getFuncInfoOrClone(Function &F) {
    // If it is cloned or not check it out.
    if (PA::FuncInfo *FI = getFuncInfo(F))
      return FI;
    // Maybe this is a function clone?
    if (Function *FC = getOrigFunctionFromClone(&F))
      return getFuncInfo(*FC);
    return 0;
  }
  

  /// releaseMemory - When the pool allocator is no longer used, release
  /// resources used by it.
  virtual void releaseMemory() {
    FunctionInfo.clear();
    GlobalNodes.clear();
    CloneToOrigMap.clear();
  }


  Module *getCurModule() { return CurModule; }

  /// CreateGlobalPool - Create a global pool descriptor, initialize it in main,
  /// and return a pointer to the global for it.
  GlobalVariable *CreateGlobalPool(unsigned RecSize, unsigned Alignment,
                                   Instruction *IPHint = 0);

 private:
  
  /// AddPoolPrototypes - Add prototypes for the pool functions to the
  /// specified module and update the Pool* instance variables to point to
  /// them.
  ///
  void AddPoolPrototypes();

  /// MicroOptimizePoolCalls - Apply any microoptimizations to calls to pool
  /// allocation function calls that we can.
  void MicroOptimizePoolCalls();
  
  /// BuildIndirectFunctionSets - Iterate over the module looking for indirect
  /// calls to functions
  void BuildIndirectFunctionSets(Module &M);   

  /// SetupGlobalPools - Create global pools for all DSNodes in the globals
  /// graph which contain heap objects.  If a global variable points to a piece
  /// of memory allocated from the heap, this pool gets a global lifetime.
  ///
  /// This method returns true if correct pool allocation of the module cannot
  /// be performed because there is no main function for the module and there
  /// are global pools.
  bool SetupGlobalPools(Module &M);

  /// FindFunctionPoolArgs - In the first pass over the program, we decide which
  /// arguments will have to be added for each function, build the FunctionInfo
  /// map and recording this info in the ArgNodes set.
  void FindFunctionPoolArgs(Function &F);   
  
  /// MakeFunctionClone - If the specified function needs to be modified for
  /// pool allocation support, make a clone of it, adding additional arguments
  /// as neccesary, and return it.  If not, just return null.
  ///
  Function *MakeFunctionClone(Function &F);
  
  /// ProcessFunctionBody - Rewrite the body of a transformed function to use
  /// pool allocation where appropriate.
  ///
  void ProcessFunctionBody(Function &Old, Function &New);
  
  /// CreatePools - This inserts alloca instruction in the function for all
  /// pools specified in the NodesToPA list.  This adds an entry to the
  /// PoolDescriptors map for each DSNode.
  ///
  void CreatePools(Function &F, DSGraph &G, 
                   const std::vector<const DSNode*> &NodesToPA,
                   std::map<const DSNode*, Value*> &PoolDescriptors);
  
  void TransformBody(DSGraph &g, PA::FuncInfo &fi,
                     std::multimap<AllocaInst*, Instruction*> &poolUses,
                     std::multimap<AllocaInst*, CallInst*> &poolFrees,
                     Function &F);

  /// InitializeAndDestroyPools - This inserts calls to poolinit and pooldestroy
  /// into the function to initialize and destroy the pools in the NodesToPA
  /// list.
  void InitializeAndDestroyPools(Function &F,
                                 const std::vector<const DSNode*> &NodesToPA,
                      std::map<const DSNode*, Value*> &PoolDescriptors,
                      std::multimap<AllocaInst*, Instruction*> &PoolUses,
                      std::multimap<AllocaInst*, CallInst*> &PoolFrees);

  void InitializeAndDestroyPool(Function &F, const DSNode *Pool,
                            std::map<const DSNode*, Value*> &PoolDescriptors,
                            std::multimap<AllocaInst*, Instruction*> &PoolUses,
                            std::multimap<AllocaInst*, CallInst*> &PoolFrees);

  void CalculateLivePoolFreeBlocks(std::set<BasicBlock*> &LiveBlocks,Value *PD);
};


/// PoolAllocatePassAllPools - This class is the same as the pool allocator,
/// except that it passes pool descriptors into functions that do not do
/// allocations or deallocations.  This is needed by the pointer compression
/// pass, which requires a pool descriptor to be available for a pool if any
/// load or store to that pool is performed.
struct PoolAllocatePassAllPools : public PoolAllocate {
  PoolAllocatePassAllPools() : PoolAllocate(true) {}
};

}

#endif
