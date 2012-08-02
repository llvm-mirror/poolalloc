//===-- RunTimeAssociate.h - Runtime Ptr Association Pass -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This transform changes programs so that pointers have an associated handle
// corrosponding to DSA results.  This is a generalization of the Poolalloc
// pass
//
//===----------------------------------------------------------------------===//

#ifndef _RUNTIMEASSOCIATE_H
#define	_RUNTIMEASSOCIATE_H

#include "llvm/Argument.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/CallSite.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CommandLine.h"

#include "dsa/DataStructure.h"

#include <set>
#include <map>
#include <vector>
#include <utility>

namespace llvm {
class DSNode;
class DSGraph;
class Type;
class AllocaInst;
class CallTargetFinder;

namespace rPA {

/// FuncInfo - Represent the pool allocation information for one function in
/// the program.  Note that many functions must actually be cloned in order
/// for pool allocation to add arguments to the function signature.  In this
/// case, the Clone and NewToOldValueMap information identify how the clone
/// maps to the original function...
///

class FuncInfo {
public:
  FuncInfo(const Function* f, DSGraph* g) : F(f), G(g), Clone(0) { }

  /// F - The function this FuncInfo corresponds to.
  ///
  const Function *F;

  DSGraph* G;

  /// Clone - The cloned version of the function, if applicable.
  ///
  Function *Clone;

  /// ArgNodes - The list of DSNodes which have pools passed in as arguments.
  ///
  std::vector<const DSNode*> ArgNodes;

  /// PoolDescriptors - The Value* which defines the pool descriptor for this
  /// DSNode.  Note: This does not necessarily include pool arguments that are
  /// passed in because of indirect function calls that are not used in the
  /// function.
  std::map<const DSNode*, Value*> PoolDescriptors;

  /// NewToOldValueMap - When and if a function needs to be cloned, this map
  /// contains a mapping from all of the values in the new function back to
  /// the values they correspond to in the old function.
  ///
  typedef std::map<const Value*, Value*> ValueMapTy;
  ValueMapTy NewToOldValueMap;

  /// MapValueToOriginal - Given a value in the cloned version of this
  /// function, map it back to the original.  If the specified value did not
  /// exist in the original function (e.g. because it's a pool descriptor),
  /// return null.

  Value * MapValueToOriginal(Value * V) const {
    ValueMapTy::const_iterator I = NewToOldValueMap.find(V);
    return I != NewToOldValueMap.end() ? const_cast<Value*> (I->second) : 0;
  }

  Value* getOldValueIfAvailable(Value* V) {
    if (!NewToOldValueMap.empty()) {
      // If the NewToOldValueMap is in effect, use it.
      ValueMapTy::iterator I = NewToOldValueMap.find(V);
      if (I != NewToOldValueMap.end())
        V = I->second;
    }
    return V;
  }

  void UpdateNewToOldValueMap(Value *OldVal, Value *NewV1, Value *NewV2 = 0) {
    ValueMapTy::iterator I = NewToOldValueMap.find(OldVal);
    assert(I != NewToOldValueMap.end() && "OldVal not found in clone?");
    NewToOldValueMap.insert(std::make_pair(NewV1, I->second));
    if (NewV2)
      NewToOldValueMap.insert(std::make_pair(NewV2, I->second));
    NewToOldValueMap.erase(I);
  }

  DSNodeHandle& getDSNodeHFor(Value* V) {
    return G->getScalarMap()[getOldValueIfAvailable(V)];
  }

};

class PoolInfo {
  Value* PrimDesc;
  
public:

  void addPrimaryDescriptor(Value* P) {
    PrimDesc = P;
  }

  void mergeNodeInfo(const DSNode* N) { }

};

} // end rPA namespace


/// RTAssociate - Associate handles with pointers
///

class RTAssociate : public ModulePass {

  // Type used to represent the pool, will be opaque in this pass
  Type *PoolDescPtrTy, *PoolDescType;

  /// Special Values - Values created by this pass which should be ignored
  std::set<const Value*> SpecialValues;

  /// ValueMap - associate pools with values
  std::map<const Value*, rPA::PoolInfo*> ValueMap;
  
  /// NodePoolMap - Find a pool from a DSNode
  std::map<const DSNode*, rPA::PoolInfo*> NodePoolMap;
  std::map<const Function*, rPA::FuncInfo> FunctionInfo;

  //////////////////////////////////////////////////////////////////////////////

  GlobalVariable* CreateGlobalPool(const DSNode*, Module*);
  AllocaInst*     CreateLocalPool(const DSNode*, Function&);
  Argument*       CreateArgPool(const DSNode*, Argument*);

  /// SetupGlobalPools - Create a global for each global dsnode and associate
  /// a pool with it
  void SetupGlobalPools(Module*, DSGraph*);

  /// setPoolForNode - associate a pool with a dsnode
  void setupPoolForNode(const DSNode*, Value*);

  /// getPoolForNode - return a poolinfo for a dsnode
  rPA::PoolInfo* getPoolForNode(const DSNode* D) {
    return NodePoolMap[D];
  }

  rPA::FuncInfo* getFuncInfo(const Function*);

  rPA::FuncInfo& makeFuncInfo(const Function* F, DSGraph* G);

  Function* MakeFunctionClone(Function& F, rPA::FuncInfo& FI, DSGraph* G);

  /// ProcessCloneBody - Rewrite the body of a transformed function to use
  /// pool allocation where appropriate.
  ///
  void ProcessFunctionBody(Function &Old, Function &New, DSGraph* G,
                           DataStructures* DS);

  void TransformBody(Function& F, rPA::FuncInfo& FI, DataStructures* DS);

  void replaceCall(CallSite CS, rPA::FuncInfo& FI, DataStructures* DS);
  
  //////////////////////////////////////////////////////////////////////////////

public:
  static char ID;

  RTAssociate();
  bool runOnModule(Module &M);
  void getAnalysisUsage(AnalysisUsage &AU) const;

  
};
/*
private:


  /// GlobalNodes - For each node (with an H marker) in the globals graph, this
  /// map contains the global variable that holds the pool descriptor for the
  /// node.
  std::map<const DSNode*, Value*> GlobalNodes;
  std::map<const Function*, Function*> CloneToOrigMap;

public:

  /// GlobalNodes - For each node (with an H marker) in the globals graph, this
  /// map contains the global variable that holds the pool descriptor for the
  /// node.
  //std::map<const DSNode*, Value*> GlobalNodes;

protected:
  std::map<const Function*, rPA::FuncInfo> FunctionInfo;
  DataStructures* Graphs;
  
public:

  //DataStructures &getGraphs() const {
//    return *Graphs;
//  }

  /// getOrigFunctionFromClone - Given a pointer to a function that was cloned
  /// from another function, return the original function.  If the argument
  /// function is not a clone, return null.

  Function *getOrigFunctionFromClone(const Function *F) const {
    std::map<const Function*, Function*>::const_iterator I = CloneToOrigMap.find(F);
    return I != CloneToOrigMap.end() ? I->second : 0;
  }

  /// getFuncInfo - Return the FuncInfo object for the specified function.
  ///

  rPA::FuncInfo *getFuncInfo(const Function &F) {
    std::map<const Function*, rPA::FuncInfo>::iterator I = FunctionInfo.find(&F);
    return I != FunctionInfo.end() ? &I->second : 0;
  }

  /// getFuncInfoOrClone - Return the function info object for for the specified
  /// function.  If this function is a clone of another function, return the
  /// function info object for the original function.

  rPA::FuncInfo *getFuncInfoOrClone(const Function &F) {
    // If it is cloned or not check it out.
    if (rPA::FuncInfo * FI = getFuncInfo(F))
      return FI;
    // Maybe this is a function clone?
    if (Function * FC = getOrigFunctionFromClone(&F))
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

  //
  // Method: getPool()
  //
  // Description:
  //  Returns the pool handle associated with the DSNode in the given function.
  //
  // Inputs:
  //  N - The DSNode of the value for which the caller wants a pool handle.
  //  F - The function in which the value for which we want a pool handle
  //      exists.
  //
  // Notes:
  //  o) The DSNode N may *not* be in the current function.  The caller may
  //     have mapped a value in the cloned function back to the original
  //     function.
  //

  virtual Value * getPool(const DSNode * N, Function & F) {
    //
    // Grab the structure containg information about the function and its
    // clones.
    //
    rPA::FuncInfo * FI = getFuncInfoOrClone(F);
    assert(FI && "Function has no FuncInfoOrClone!\n");

    //
    // Look for a mapping from the DSNode to the pool handle.
    //
    std::map<const DSNode*, Value*>::iterator I = FI->PoolDescriptors.find(N);
    if (I != FI->PoolDescriptors.end()) {
      Value * Pool = I->second;

      //
      // Now the fun part:
      //  The specified function could either be a clone or the original
      //  function.  This means that the pool descriptor that is matched with
      //  the DSNode is:
      //    o) A constant accessible from both the original function and its
      //       clones.
      //    o) A global variable accessible from both the original function and
      //       its clones.
      //    o) An allocation accessible only to the function.
      //    o) A function parameter accessible only to the local function.
      //
      //  In short, we need to filter out the case where we find a pool handle,
      //  but it's only accessible from a clone and not the original function.
      //
      assert((isa<GlobalVariable > (Pool) ||
             isa<AllocationInst > (Pool) ||
             isa<Argument > (Pool) ||
             isa<Constant > (Pool)) &&
             "Pool of unknown type!\n");
      if ((isa<GlobalVariable > (Pool)) || (isa<Constant > (Pool))) {
        return Pool;
      } else if (AllocationInst * AI = dyn_cast<AllocationInst > (Pool)) {
        if (AI->getParent()->getParent() == &F)
          return Pool;
      } else if (Argument * Arg = dyn_cast<Argument > (Pool)) {
        if (Arg->getParent() == &F)
          return Pool;
      }
    }

    //
    // We either do not have a pool, or the pool is not accessible from the
    // specified function.  Return NULL.
    //
    return 0;
  }

  virtual Value * getGlobalPool(const DSNode * Node) {
    std::map<const DSNode *, Value *>::iterator I = GlobalNodes.find(Node);
    if (I == GlobalNodes.end())
      return 0;
    else
      return I->second;
  }

protected:

  /// AddPrototypes - Add prototypes for the pool functions to the
  /// specified module and update the Pool* instance variables to point to
  /// them.
  ///
  void AddPrototypes(Module*);

private:

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

  /// CreatePools - This inserts alloca instruction in the function for all
  /// pools specified in the NodesToPA list.  This adds an entry to the
  /// PoolDescriptors map for each DSNode.
  ///
  void CreatePools(Function &F, DSGraph* G,
                   const std::vector<const DSNode*> &NodesToPA,
                   std::map<const DSNode*, Value*> &PoolDescriptors);

  void TransformBody(DSGraph* g, rPA::FuncInfo &fi,
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

  void CalculateLivePoolFreeBlocks(std::set<BasicBlock*> &LiveBlocks, Value *PD);
};
*/

}

#endif	/* _RUNTIMEASSOCIATE_H */

