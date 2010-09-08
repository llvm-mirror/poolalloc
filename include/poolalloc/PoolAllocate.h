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

#include "llvm/Argument.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/CallSite.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/VectorExtras.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/CommandLine.h"

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "poolalloc/Config/config.h"

#include <utility>

namespace llvm {
class DSNode;
class DSGraph;
class Type;
class AllocaInst;
class CallTargetFinder;

namespace PA {

  extern cl::opt<bool>  PA_SAFECODE;

  class Heuristic;

  /// FuncInfo - Represent the pool allocation information for one function in
  /// the program.  Note that many functions must actually be cloned in order
  /// for pool allocation to add arguments to the function signature.  In this
  /// case, the Clone and NewToOldValueMap information identify how the clone
  /// maps to the original function...
  ///
  /// FIXME: This structure should implement information hiding.  There should
  ///        be some information hiding.  The design may depend on how we change
  ///        the way in which clients interface with pool allocation analysis
  ///        results.
  ///
  struct FuncInfo {
    FuncInfo(Function &f) : F(f), Clone(0), rev_pool_desc_map_computed(false) {}

    /// MarkedNodes - The set of nodes which are not locally pool allocatable in
    /// the current function.
    ///
    /// FIXME: This field has a non-descriptive name.
    ///
    DenseSet<const DSNode*> MarkedNodes;

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
    /// FIXME: This comment should clearly describe which pools are and are not
    ///        in this data structure.
    std::map<const DSNode*, Value*> PoolDescriptors;

    // Reverse mapping for PoolDescriptors, needed by TPPA
    // FIXME: There can be multiple DSNodes mapped to a single pool descriptor
    std::multimap<Value*, const DSNode*> ReversePoolDescriptors;

    // This is a hack -- a function should be added which maintains these in parallel
    // and all of PoolAlloc and SafeCode should be updated to use it instead of adding
    // to either map directly.
    bool rev_pool_desc_map_computed;
    void calculate_reverse_pool_descriptors()
    {
    	if(rev_pool_desc_map_computed)
    		return;
    	rev_pool_desc_map_computed = true;

    	for(std::map<const DSNode*, Value*>::iterator i = PoolDescriptors.begin(); i!=PoolDescriptors.end(); i++)
    		ReversePoolDescriptors.insert(std::pair<Value*,const DSNode*>(i->second,i->first));
    }

    /// This is a map from Old to New Values (the reverse of NewToOldValueMap).
    /// SAFECode uses this for check insertion.
    /// FIXME: Does SAFECode still use this?  Should a single class handle this map and the
    /// NewToOldValue Map?
    std::map<const Value*, Value*> ValueMap;

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

} // end PA namespace


class PoolAllocateGroup : public ModulePass {
protected:
  DataStructures *Graphs;
  const Type * VoidType;
  const Type * Int8Type;
  const Type * Int32Type;

public:
  static char ID;
  // FIXME: Some of these things should not be public.
  Constant *PoolRegister;

  // FIXME: We want to remove SAFECode specified flags.
  bool SAFECodeEnabled;
  bool BoundsChecksEnabled;

  enum LIE_TYPE {LIE_NONE, LIE_PRESERVE_DSA, LIE_PRESERVE_ALL, LIE_PRESERVE_DEFAULT};
  // FIXME: Try to minimize lying
  LIE_TYPE lie_preserve_passes;
  // FIXME: Let clients choose which DSA pass is used by scheduling it ahead of time.
  enum PASS_TYPE {PASS_EQTD, PASS_BUEQ, PASS_DEFAULT};
  PASS_TYPE dsa_pass_to_use;

                
  PoolAllocateGroup  (intptr_t IDp = (intptr_t) (&ID)) : ModulePass (IDp) { }
  virtual ~PoolAllocateGroup () {return;}

  virtual PA::FuncInfo *getFuncInfo(const Function &F) { return 0;}
  virtual PA::FuncInfo *getFuncInfoOrClone(const Function &F) {return 0;}
  virtual Function *getOrigFunctionFromClone(const Function *F) const {return 0;}

  // FIXME: Clients should be able to specialize pool descriptor type
  virtual const Type * getPoolType(LLVMContext*) {return 0;}

  virtual bool hasDSGraph (const Function & F) const {
    return Graphs->hasDSGraph (F);
  }

  virtual DSGraph*  getDSGraph (const Function & F) const {
    return Graphs->getDSGraph (F);
  }

  virtual DSGraph* getGlobalsGraph () const {
    return Graphs->getGlobalsGraph ();
  }

  // Return value is of type PoolDescPtrTy
  // FIXME: Do we need to have a getGlobalPool() for clients?
  // FIXME: Can we infer the function?  Possibly not since we want to distinguish between
  //        pools in clones and pools in original function.
  // FIXME: We want something like getPool (Value *) if possible.
  virtual Value * getPool (const DSNode * N, Function & F) {return 0;}

  virtual Value * getGlobalPool (const DSNode * Node) {return 0;}

};

/// PoolAllocate - The main pool allocation pass
///
class PoolAllocate : public PoolAllocateGroup {
  /// PassAllArguments - If set to true, we should pass pool descriptor
  /// arguments into any function that loads or stores to a pool, in addition to
  /// those functions that allocate or deallocate.  See also the
  /// PoolAllocatePassAllPools pass below.
  bool PassAllArguments;

  Module *CurModule;

  // FIXME: Where is this used?  Why isn't DSCallGraph used directly?
  CallTargetFinder* CTF;
  
  // Map a cloned function to its original function
  std::map<const Function*, Function*> CloneToOrigMap;
public:

  Constant *PoolInit, *PoolDestroy, *PoolAlloc, *PoolRealloc, *PoolMemAlign, *PoolThreadWrapper;
  Constant *PoolFree;
  Constant *PoolCalloc;
  Constant *PoolStrdup;
  
  static const Type *PoolDescPtrTy;

  PA::Heuristic *CurHeuristic;

  /// GlobalNodes - For each node (with an H marker) in the globals graph, this
  /// map contains the global variable that holds the pool descriptor for the
  /// node.
  std::map<const DSNode*, Value*> GlobalNodes;

protected:
  std::map<const Function*, PA::FuncInfo> FunctionInfo;

 public:
  static char ID;

  PoolAllocate (bool passAllArguments,
                bool SAFECode = true,
                intptr_t IDp = (intptr_t) (&ID))
    : PoolAllocateGroup ((intptr_t)IDp),
      PassAllArguments(passAllArguments)
      {
		  SAFECodeEnabled = BoundsChecksEnabled = SAFECode |  PA::PA_SAFECODE;
		  lie_preserve_passes = SAFECodeEnabled ? LIE_PRESERVE_ALL : LIE_PRESERVE_DSA;
		  dsa_pass_to_use = SAFECodeEnabled ? PASS_EQTD : PASS_BUEQ;
      }

  /*TODO: finish removing the SAFECode flag*/
  PoolAllocate (PASS_TYPE dsa_pass_to_use_ = PASS_DEFAULT,
				LIE_TYPE lie_preserve_passes_ = LIE_PRESERVE_DEFAULT,
				bool passAllArguments = false,
				bool SAFECode = true,
                intptr_t IDp = (intptr_t) (&ID))
      : PoolAllocateGroup ((intptr_t)IDp),
        PassAllArguments(passAllArguments)
        {
  		  SAFECodeEnabled = BoundsChecksEnabled = SAFECode |  PA::PA_SAFECODE;

  		  if(lie_preserve_passes_ == LIE_PRESERVE_DEFAULT)
  			  lie_preserve_passes = SAFECodeEnabled ? LIE_PRESERVE_ALL : LIE_PRESERVE_DSA;
  		  else
  			  lie_preserve_passes = lie_preserve_passes_;

  		  if(dsa_pass_to_use_ == PASS_DEFAULT)
  			  dsa_pass_to_use = SAFECodeEnabled ? PASS_EQTD : PASS_BUEQ;
  		  else
  			  dsa_pass_to_use = dsa_pass_to_use_;
        }

  virtual bool runOnModule(Module &M);
  
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  
  // FIXME: This method is misnamed.
  DataStructures &getGraphs() const { return *Graphs; }

  /// getOrigFunctionFromClone - Given a pointer to a function that was cloned
  /// from another function, return the original function.  If the argument
  /// function is not a clone, return null.
  Function *getOrigFunctionFromClone(const Function *F) const {
    std::map<const Function*, Function*>::const_iterator I = CloneToOrigMap.find(F);
    return I != CloneToOrigMap.end() ? I->second : 0;
  }

  // FIXME: add isClone()

  /// getFuncInfo - Return the FuncInfo object for the specified function.
  ///
  PA::FuncInfo *getFuncInfo(const Function &F) {
    std::map<const Function*, PA::FuncInfo>::iterator I = FunctionInfo.find(&F);
    return I != FunctionInfo.end() ? &I->second : 0;
  }

  /// getFuncInfoOrClone - Return the function info object for for the specified
  /// function.  If this function is a clone of another function, return the
  /// function info object for the original function.
  PA::FuncInfo *getFuncInfoOrClone(const Function &F) {
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

  /// getPoolType - Return the type of a pool descriptor
  /// FIXME: These constants should be chosen by the client
  const Type * getPoolType(LLVMContext* C) {
    const IntegerType * IT = IntegerType::getInt8Ty(*C);
    Type * VoidPtrType = PointerType::getUnqual(IT);
    if (SAFECodeEnabled)
      return ArrayType::get(VoidPtrType, 92);
    else
      return ArrayType::get(VoidPtrType, 16);
  }

  virtual DSGraph* getDSGraph (const Function & F) const {
    return Graphs->getDSGraph (F);
  }

  virtual DSGraph* getGlobalsGraph () const {
    return Graphs->getGlobalsGraph ();
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
  virtual Value * getPool (const DSNode * N, Function & F) {
    //
    // Grab the structure containing information about the function and its
    // clones.
    //
    PA::FuncInfo * FI = getFuncInfoOrClone (F);
    assert (FI && "Function has no FuncInfoOrClone!\n");

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
      //FIXME: handle allocators
      assert ((isa<GlobalVariable>(Pool) ||
               isa<AllocaInst>(Pool) ||
               isa<Argument>(Pool) ||
               isa<Constant>(Pool)) &&
               "Pool of unknown type!\n");
      if ((isa<GlobalVariable>(Pool)) || (isa<Constant>(Pool))) {
          return Pool;
      } else if (AllocaInst * AI = dyn_cast<AllocaInst>(Pool)) {
        if (AI->getParent()->getParent() == &F)
          return Pool;
      } else if (Argument * Arg = dyn_cast<Argument>(Pool)) {
        if (Arg->getParent() == &F)
          return Pool;
      }
    }

    //
    // Perhaps this is a global pool.  If it isn't, then return a NULL
    // pointer.
    //
    return getGlobalPool (N);
  }

  virtual Value * getGlobalPool (const DSNode * Node) {
    std::map<const DSNode *, Value *>::iterator I = GlobalNodes.find (Node);
    if (I == GlobalNodes.end())
      return 0;
    else
      return I->second;
  }

protected:
  
  /// AddPoolPrototypes - Add prototypes for the pool functions to the
  /// specified module and update the Pool* instance variables to point to
  /// them.
  ///
  void AddPoolPrototypes(Module*);

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

  /// FindPoolArgs - Make a pass over the module and find the arguments of each
  /// function that need to have their pools passed in.  This will build a
  /// FunctionInfo for each function.
  void FindPoolArgs (Module & M);

  /// FindFunctionPoolArgs - In the first pass over the program, we decide which
  /// arguments will have to be added for each function, build the FunctionInfo
  /// map and recording this info in the ArgNodes set.
  void FindFunctionPoolArgs (Function &F);   
  
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
  void CreatePools(Function &F, DSGraph* G, 
                   const std::vector<const DSNode*> &NodesToPA,
                   std::map<const DSNode*, Value*> &PoolDescriptors);
  
  void TransformBody(DSGraph* g, PA::FuncInfo &fi,
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
  static char ID;
  PoolAllocatePassAllPools() : PoolAllocate(PASS_DEFAULT, LIE_PRESERVE_DEFAULT, true, false, (intptr_t) &ID) {}
};

/// PoolAllocateSimple - This class modifies the heap allocations so that they
/// use the pool allocator run-time.  However, unlike PoolAllocatePassAllPools,
/// it doesn't involve all of complex machinery of the original pool allocation
/// implementation.
class PoolAllocateSimple : public PoolAllocate {
  Value * TheGlobalPool;
  DSGraph * CombinedDSGraph;
  EquivalenceClasses<const GlobalValue*> GlobalECs;
  TargetData * TD;
  bool CompleteDSA;
public:
  static char ID;
  PoolAllocateSimple(bool passAllArgs=false, bool SAFECode = true, bool CompleteDSA = true)
    : PoolAllocate (PASS_DEFAULT, LIE_PRESERVE_DEFAULT, passAllArgs, SAFECode, (intptr_t)&ID), CompleteDSA(CompleteDSA) {}
  ~PoolAllocateSimple() {return;}
  void getAnalysisUsage(AnalysisUsage &AU) const;
  bool runOnModule(Module &M);
  GlobalVariable *CreateGlobalPool(unsigned RecSize, unsigned Align,
                                   Module& M);
  void ProcessFunctionBodySimple(Function& F, TargetData & TD);


  virtual DSGraph* getDSGraph (const Function & F) const {
    return CombinedDSGraph;
  }

  virtual DSGraph* getGlobalsGraph () const {
    return CombinedDSGraph->getGlobalsGraph();
  }

  virtual Value * getGlobalPool (const DSNode * Node) {
    return TheGlobalPool;
  }

  virtual Value * getPool (const DSNode * N, Function & F) {
    return TheGlobalPool;
  }
};

/// PoolAllocateMultipleGlobalPool
/// Context-insensitive pool allocation. It pool allocates objects into multiple
/// global pools. It does not need to rewrite the functions declarations, which
/// simplifies the implementation a lot. Technically, PoolAllocateSimple, which
/// pool allocates everything into a single global pool, is a
/// special case of PoolAllocateMultipleGlobalPool.
///
/// It requires some work on code clean up to make these two pass integrate
/// nicely.

// FIXME: Is this used?  Should it be removed?
class PoolAllocateMultipleGlobalPool : public PoolAllocate {
  TargetData * TD;
  void ProcessFunctionBodySimple(Function& F, TargetData & TD);
  /// Mapping between DSNodes and Pool descriptors. For this pass, it is a
  /// one-to-one relationship.
  typedef DenseMap<const DSNode *, GlobalVariable *> PoolMapTy;
  PoolMapTy PoolMap;
  void generatePool(unsigned RecSize, unsigned Align,
                    Module& M, BasicBlock * InsertAtEnd, const DSNode * Node);
  Module * currentModule;
public:
  static char ID;
  PoolAllocateMultipleGlobalPool(bool passAllArgs=false, bool SAFECode = true)
    : PoolAllocate (PASS_DEFAULT, LIE_PRESERVE_DEFAULT, passAllArgs, SAFECode, (intptr_t)&ID) {}
  ~PoolAllocateMultipleGlobalPool();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  void CreateGlobalPool(unsigned RecSize, unsigned Align,
                                   Module& M);

  virtual Value * getGlobalPool (const DSNode * Node);
  virtual Value * getPool (const DSNode * N, Function & F);
  virtual void print(llvm::raw_ostream &OS, const Module * M) const;
  virtual void dump() const;
};

}
#endif
