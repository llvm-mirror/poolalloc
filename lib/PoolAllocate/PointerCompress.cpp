//===-- PointerCompress.cpp - Pointer Compression Pass --------------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements the -pointercompress pass.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pointercompress"
#include "EquivClassGraphs.h"
#include "PoolAllocate.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstVisitor.h"
using namespace llvm;

namespace {
  Statistic<> NumCompressed("pointercompress",
                            "Number of pools pointer compressed");
  Statistic<> NumNotCompressed("pointercompress",
                               "Number of pools not compressible");

  class CompressedPoolInfo;

  /// PointerCompress - This transformation hacks on type-safe pool allocated
  /// data structures to reduce the size of pointers in the program.
  class PointerCompress : public ModulePass {
    PoolAllocate *PoolAlloc;
    PA::EquivClassGraphs *ECG;
  public:
    Function *PoolInitPC, *PoolDestroyPC, *PoolAllocPC, *PoolFreePC;
    typedef std::map<const DSNode*, CompressedPoolInfo> PoolInfoMap;

    bool runOnModule(Module &M);
    void getAnalysisUsage(AnalysisUsage &AU) const;

  private:
    void InitializePoolLibraryFunctions(Module &M);
    bool CompressPoolsInFunction(Function &F);
    void FindPoolsToCompress(std::vector<const DSNode*> &Pools, Function &F,
                             DSGraph &DSG, PA::FuncInfo *FI);
  };

  RegisterOpt<PointerCompress>
  X("pointercompress", "Compress type-safe data structures");
}

//===----------------------------------------------------------------------===//
//               CompressedPoolInfo Class and Implementation
//===----------------------------------------------------------------------===//

namespace {
  /// CompressedPoolInfo - An instance of this structure is created for each
  /// pool that is compressed.
  class CompressedPoolInfo {
    const DSNode *Pool;
    Value *PoolDesc;
    const Type *NewTy;
    unsigned NewSize;
  public:
    CompressedPoolInfo(const DSNode *N, Value *PD)
      : Pool(N), PoolDesc(PD), NewTy(0) {}
    
    /// Initialize - When we know all of the pools in a function that are going
    /// to be compressed, initialize our state based on that data.
    void Initialize(std::map<const DSNode*, CompressedPoolInfo> &Nodes,
                    const TargetData &TD);

    const DSNode *getNode() const { return Pool; }
    const Type *getNewType() const { return NewTy; }

    /// getNewSize - Return the size of each node after compression.
    ///
    unsigned getNewSize() const { return NewSize; }
    
    /// getPoolDesc - Return the Value* for the pool descriptor for this pool.
    ///
    Value *getPoolDesc() const { return PoolDesc; }

    // dump - Emit a debugging dump of this pool info.
    void dump() const;

  private:
    const Type *ComputeCompressedType(const Type *OrigTy, unsigned NodeOffset,
                           std::map<const DSNode*, CompressedPoolInfo> &Nodes);
  };
}

/// Initialize - When we know all of the pools in a function that are going
/// to be compressed, initialize our state based on that data.
void CompressedPoolInfo::Initialize(std::map<const DSNode*, 
                                             CompressedPoolInfo> &Nodes,
                                    const TargetData &TD) {
  // First step, compute the type of the compressed node.  This basically
  // replaces all pointers to compressed pools with uints.
  NewTy = ComputeCompressedType(Pool->getType(), 0, Nodes);

  // Get the compressed type size.
  NewSize = TD.getTypeSize(NewTy);
}


/// ComputeCompressedType - Recursively compute the new type for this node after
/// pointer compression.  This involves compressing any pointers that point into
/// compressed pools.
const Type *CompressedPoolInfo::
ComputeCompressedType(const Type *OrigTy, unsigned NodeOffset,
                      std::map<const DSNode*, CompressedPoolInfo> &Nodes) {
  if (const PointerType *PTY = dyn_cast<PointerType>(OrigTy)) {
    // FIXME: check to see if this pointer is actually compressed!
    return Type::UIntTy;
  } else if (OrigTy->isFirstClassType())
    return OrigTy;

  // Okay, we have an aggregate type.
  assert(0 && "FIXME: Unhandled aggregate type!");
}

/// dump - Emit a debugging dump for this pool info.
///
void CompressedPoolInfo::dump() const {
  std::cerr << "Node: "; getNode()->dump();
  std::cerr << "New Type: " << *NewTy << "\n";
}


//===----------------------------------------------------------------------===//
//                    PointerCompress Implementation
//===----------------------------------------------------------------------===//

void PointerCompress::getAnalysisUsage(AnalysisUsage &AU) const {
  // Need information about how pool allocation happened.
  AU.addRequired<PoolAllocate>();

  // Need information from DSA.
  AU.addRequired<PA::EquivClassGraphs>();
}

/// PoolIsCompressible - Return true if we can pointer compress this node.
/// If not, we should DEBUG print out why.
static bool PoolIsCompressible(const DSNode *N, Function &F) {
  assert(!N->isForwarding() && "Should not be dealing with merged nodes!");
  if (N->isNodeCompletelyFolded()) {
    DEBUG(std::cerr << "Node is not type-safe:\n");
    return false;
  }

  // If this has no pointer fields, don't compress.
  bool HasFields = false;
  for (DSNode::const_edge_iterator I = N->edge_begin(), E = N->edge_end();
       I != E; ++I)
    if (!I->isNull()) {
      HasFields = true;
      if (I->getNode() != N) {
        // We currently only handle trivially self cyclic DS's right now.
        DEBUG(std::cerr << "Node points to nodes other than itself:\n");
        return false;
      }        
    }

  if (!HasFields) {
    DEBUG(std::cerr << "Node does not contain any pointers to compress:\n");
    return false;
  }

  if (N->isArray()) {
    DEBUG(std::cerr << "Node is an array (not yet handled!):\n");
    return false;
  }

  if ((N->getNodeFlags() & DSNode::Composition) != DSNode::HeapNode) {
    DEBUG(std::cerr << "Node contains non-heap values:\n");
    return false;
  }

  return true;
}

/// FindPoolsToCompress - Inspect the specified function and find pools that are
/// compressible that are homed in that function.  Return those pools in the
/// Pools vector.
void PointerCompress::FindPoolsToCompress(std::vector<const DSNode*> &Pools,
                                          Function &F, DSGraph &DSG,
                                          PA::FuncInfo *FI) {
  hash_set<const DSNode*> ReachableFromCalls;
  
  // If a data structure is passed into a call, we currently cannot handle it!
  for (DSGraph::fc_iterator I = DSG.fc_begin(), E = DSG.fc_end(); I != E; ++I) {
    //DEBUG(std::cerr << " CALLS: " << I->getCalleeFunc()->getName() << "\n");
    I->markReachableNodes(ReachableFromCalls);
  }
  
  DEBUG(std::cerr << "In function '" << F.getName() << "':\n");
  for (unsigned i = 0, e = FI->NodesToPA.size(); i != e; ++i) {
    const DSNode *N = FI->NodesToPA[i];
    if (ReachableFromCalls.count(N)) {
      DEBUG(std::cerr << "Passed into call:\nPCF: "; N->dump());
      ++NumNotCompressed;
    } else if (PoolIsCompressible(N, F)) {
      Pools.push_back(N);
      ++NumCompressed;
    } else {
      DEBUG(std::cerr << "PCF: "; N->dump());
      ++NumNotCompressed;
    }
  }
}


namespace {
  /// InstructionRewriter - This class implements the rewriting neccesary to
  /// transform a function body from normal pool allocation to pointer
  /// compression.  It is constructed, then the 'visit' method is called on a
  /// function.  If is responsible for rewriting all instructions that refer to
  /// pointers into compressed pools.
  class InstructionRewriter : public llvm::InstVisitor<InstructionRewriter> {
    /// OldToNewValueMap - This keeps track of what new instructions we create
    /// for instructions that used to produce pointers into our pool.
    std::map<Value*, Value*> OldToNewValueMap;
  
    const PointerCompress::PoolInfoMap &PoolInfo;

    const DSGraph &DSG;

    PointerCompress &PtrComp;
  public:
    InstructionRewriter(const PointerCompress::PoolInfoMap &poolInfo,
                        const DSGraph &dsg, PointerCompress &ptrcomp)
      : PoolInfo(poolInfo), DSG(dsg), PtrComp(ptrcomp) {
    }

    ~InstructionRewriter();

    /// getTransformedValue - Return the transformed version of the specified
    /// value, creating a new forward ref value as needed.
    Value *getTransformedValue(Value *V) {
      if (isa<ConstantPointerNull>(V))                // null -> uint 0
        return Constant::getNullValue(Type::UIntTy);

      assert(getNodeIfCompressed(V) && "Value is not compressed!");
      Value *&RV = OldToNewValueMap[V];
      if (RV) return RV;

      RV = new Argument(Type::UIntTy);
      return RV;
    }

    /// setTransformedValue - When we create a new value, this method sets it as
    /// the current value.
    void setTransformedValue(Instruction &Old, Value *New) {
      Value *&EV = OldToNewValueMap[&Old];
      if (EV) {
        assert(isa<Argument>(EV) && "Not a forward reference!");
        EV->replaceAllUsesWith(New);
        delete EV;
      }
      EV = New;
    }

    /// getNodeIfCompressed - If the specified value is a pointer that will be
    /// compressed, return the DSNode corresponding to the pool it belongs to.
    const DSNode *getNodeIfCompressed(Value *V) {
      if (!isa<PointerType>(V->getType()) || isa<ConstantPointerNull>(V))
        return false;
      DSNode *N = DSG.getNodeForValue(V).getNode();
      return PoolInfo.count(N) ? N : 0;
    }

    /// getPoolInfo - Return the pool info for the specified compressed pool.
    ///
    const CompressedPoolInfo &getPoolInfo(const DSNode *N) {
      assert(N && "Pool not compressed!");
      PointerCompress::PoolInfoMap::const_iterator I = PoolInfo.find(N);
      assert(I != PoolInfo.end() && "Pool is not compressed!");
      return I->second;
    }

    /// getPoolInfo - Return the pool info object for the specified value if the
    /// pointer points into a compressed pool, otherwise return null.
    const CompressedPoolInfo *getPoolInfo(Value *V) {
      if (const DSNode *N = getNodeIfCompressed(V))
        return &getPoolInfo(N);
      return 0;
    }

    //===------------------------------------------------------------------===//
    // Visitation methods.  These do all of the heavy lifting for the various
    // cases we have to handle.

    void visitCastInst(CastInst &CI);
    void visitPHINode(PHINode &PN);
    void visitLoadInst(LoadInst &LI);
    void visitStoreInst(StoreInst &SI);

    void visitCallInst(CallInst &CI);
    void visitPoolInit(CallInst &CI);
    void visitPoolDestroy(CallInst &CI);
    void visitPoolAlloc(CallInst &CI);
    void visitPoolFree(CallInst &CI);

    void visitInstruction(Instruction &I) {
#ifndef NDEBUG
      bool Unhandled = !!getNodeIfCompressed(&I);
      for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i)
        Unhandled |= !!getNodeIfCompressed(I.getOperand(i));

      if (Unhandled) {
        std::cerr << "ERROR: UNHANDLED INSTRUCTION: " << I;
        //assert(0);
        //abort();
      }
#endif
    }
  };
} // end anonymous namespace.


InstructionRewriter::~InstructionRewriter() {
  // Nuke all of the old values from the program.
  for (std::map<Value*, Value*>::iterator I = OldToNewValueMap.begin(),
         E = OldToNewValueMap.end(); I != E; ++I) {
    assert((!isa<Argument>(I->second) || cast<Argument>(I->second)->getParent())
           && "ERROR: Unresolved value still left in the program!");
    // If there is anything still using this, provide a temporary value.
    if (!I->first->use_empty())
      I->first->replaceAllUsesWith(UndefValue::get(I->first->getType()));

    // Finally, remove it from the program.
    cast<Instruction>(I->first)->eraseFromParent();
  }
}


void InstructionRewriter::visitCastInst(CastInst &CI) {
  if (!isa<PointerType>(CI.getType())) return;

  const CompressedPoolInfo *PI = getPoolInfo(&CI);
  if (!PI) return;
  assert(getPoolInfo(CI.getOperand(0)) == PI && "Not cast from ptr -> ptr?");

  // A cast from one pointer to another turns into a cast from uint -> uint,
  // which is a noop.
  setTransformedValue(CI, getTransformedValue(CI.getOperand(0)));
}

void InstructionRewriter::visitPHINode(PHINode &PN) {
  const CompressedPoolInfo *DestPI = getPoolInfo(&PN);
  if (DestPI == 0) return;

  PHINode *New = new PHINode(Type::UIntTy, PN.getName(), &PN);
  New->reserveOperandSpace(PN.getNumIncomingValues());

  for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i)
    New->addIncoming(getTransformedValue(PN.getIncomingValue(i)),
                     PN.getIncomingBlock(i));
  setTransformedValue(PN, New);
}

void InstructionRewriter::visitLoadInst(LoadInst &LI) {
  if (isa<ConstantPointerNull>(LI.getOperand(0))) return; // load null ??

  const CompressedPoolInfo *SrcPI = getPoolInfo(LI.getOperand(0));
  if (SrcPI == 0) {
    assert(getPoolInfo(&LI) == 0 &&
           "Cannot load a compressed pointer from non-compressed memory!");
    return;
  }

  // We care about two cases, here:
  //  1. Loading a normal value from a ptr compressed data structure.
  //  2. Loading a compressed ptr from a ptr compressed data structure.
  bool LoadingCompressedPtr = getNodeIfCompressed(&LI) != 0;
  
  // Get the pool base pointer.
  Constant *Zero = Constant::getNullValue(Type::UIntTy);
  Value *BasePtrPtr = new GetElementPtrInst(SrcPI->getPoolDesc(), Zero, Zero,
                                            "poolbaseptrptr", &LI);
  Value *BasePtr = new LoadInst(BasePtrPtr, "poolbaseptr", &LI);

  // Get the pointer to load from.
  std::vector<Value*> Ops;
  Ops.push_back(getTransformedValue(LI.getOperand(0)));
  Value *SrcPtr = new GetElementPtrInst(BasePtr, Ops,
                                        LI.getOperand(0)->getName()+".pp", &LI);
  const Type *DestTy = LoadingCompressedPtr ? Type::UIntTy : LI.getType();
  SrcPtr = new CastInst(SrcPtr, PointerType::get(DestTy),
                        SrcPtr->getName(), &LI);
  std::string OldName = LI.getName(); LI.setName("");
  Value *NewLoad = new LoadInst(SrcPtr, OldName, &LI);

  if (LoadingCompressedPtr) {
    setTransformedValue(LI, NewLoad);
  } else {
    LI.replaceAllUsesWith(NewLoad);
    LI.eraseFromParent();
  }
}



void InstructionRewriter::visitStoreInst(StoreInst &SI) {
  const CompressedPoolInfo *DestPI = getPoolInfo(SI.getOperand(1));
  if (DestPI == 0) {
    assert(getPoolInfo(SI.getOperand(0)) == 0 &&
           "Cannot store a compressed pointer into non-compressed memory!");
    return;
  }

  // We care about two cases, here:
  //  1. Storing a normal value into a ptr compressed data structure.
  //  2. Storing a compressed ptr into a ptr compressed data structure.  Note
  //     that we cannot use the src value to decide if this is a compressed
  //     pointer if it's a null pointer.  We have to try harder.
  //
  Value *SrcVal = SI.getOperand(0);
  if (!isa<ConstantPointerNull>(SrcVal)) {
    const CompressedPoolInfo *SrcPI = getPoolInfo(SrcVal);
    if (SrcPI)     // If the stored value is compressed, get the xformed version
      SrcVal = getTransformedValue(SrcVal);
  } else {
    // FIXME: This assumes that all pointers are compressed!
    SrcVal = getTransformedValue(SrcVal);
  }
  
  // Get the pool base pointer.
  Constant *Zero = Constant::getNullValue(Type::UIntTy);
  Value *BasePtrPtr = new GetElementPtrInst(DestPI->getPoolDesc(), Zero, Zero,
                                            "poolbaseptrptr", &SI);
  Value *BasePtr = new LoadInst(BasePtrPtr, "poolbaseptr", &SI);

  // Get the pointer to store to.
  std::vector<Value*> Ops;
  Ops.push_back(getTransformedValue(SI.getOperand(1)));
  Value *DestPtr = new GetElementPtrInst(BasePtr, Ops,
                                         SI.getOperand(1)->getName()+".pp",
                                         &SI);
  DestPtr = new CastInst(DestPtr, PointerType::get(SrcVal->getType()),
                         DestPtr->getName(), &SI);
  new StoreInst(SrcVal, DestPtr, &SI);

  // Finally, explicitly remove the store from the program, as it does not
  // produce a pointer result.
  SI.eraseFromParent();
}


void InstructionRewriter::visitPoolInit(CallInst &CI) {
  // Transform to poolinit_pc if necessary.
}

void InstructionRewriter::visitPoolDestroy(CallInst &CI) {
  // Transform to pooldestroy_pc if necessary.
  /* TODO */
}
void InstructionRewriter::visitPoolAlloc(CallInst &CI) {
  // Transform to poolalloc_pc if necessary.
  const CompressedPoolInfo *PI = getPoolInfo(&CI);
  if (PI == 0) return;  // Not a compressed pool!

  std::vector<Value*> Ops;
  Ops.push_back(CI.getOperand(1));
  Ops.push_back(ConstantUInt::get(Type::UIntTy, PI->getNewSize()));
  Value *New = new CallInst(PtrComp.PoolAllocPC, Ops, CI.getName(), &CI);
  setTransformedValue(CI, New);
}

void InstructionRewriter::visitPoolFree(CallInst &CI) {
  // Transform to poolfree_pc if necessary.
}

void InstructionRewriter::visitCallInst(CallInst &CI) {
  if (Function *F = CI.getCalledFunction())
    if (F->getName() == "poolinit") {
      visitPoolInit(CI);
      return;
    } else if (F->getName() == "pooldestroy") {
      visitPoolDestroy(CI);
      return;
    } else if (F->getName() == "poolalloc") {
      visitPoolAlloc(CI);
      return;
    } else if (F->getName() == "poolfree") {
      visitPoolFree(CI);
      return;
    }
  
  visitInstruction(CI);
}


/// CompressPoolsInFunction - Find all pools that are compressible in this
/// function and compress them.
bool PointerCompress::CompressPoolsInFunction(Function &F) {
  if (F.isExternal()) return false;

  PA::FuncInfo *FI = PoolAlloc->getFuncInfoOrClone(F);
  if (FI == 0) {
    std::cerr << "DIDN'T FIND POOL INFO FOR: "
              << *F.getType() << F.getName() << "!\n";
    return false;
  }

  // If this function was cloned, and this is the original function, ignore it
  // (it's dead).  We'll deal with the cloned version later when we run into it
  // again.
  if (FI->Clone && &FI->F == &F)
    return false;

  // There are no pools in this function.
  if (FI->NodesToPA.empty())
    return false;

  // Get the DSGraph for this function.
  DSGraph &DSG = ECG->getDSGraph(FI->F);

  // Compute the set of compressible pools in this function.
  std::vector<const DSNode*> PoolsToCompressList;
  FindPoolsToCompress(PoolsToCompressList, F, DSG, FI);

  if (PoolsToCompressList.empty()) return false;

  // Compute the initial collection of compressed pointer infos.
  std::map<const DSNode*, CompressedPoolInfo> PoolsToCompress;
  for (unsigned i = 0, e = PoolsToCompressList.size(); i != e; ++i) {
    const DSNode *N = PoolsToCompressList[i];
    Value *PD = FI->PoolDescriptors[N];
    assert(PD && "No pool descriptor available for this pool???");
    PoolsToCompress.insert(std::make_pair(N, CompressedPoolInfo(N, PD)));
  }

  // Use these to compute the closure of compression information.  In
  // particular, if one pool points to another, we need to know if the outgoing
  // pointer is compressed.
  const TargetData &TD = DSG.getTargetData();
  std::cerr << "In function '" << F.getName() << "':\n";
  for (std::map<const DSNode*, CompressedPoolInfo>::iterator
         I = PoolsToCompress.begin(), E = PoolsToCompress.end(); I != E; ++I) {
    I->second.Initialize(PoolsToCompress, TD);
    std::cerr << "  COMPRESSING POOL:\nPCS:";
    I->second.dump();
  }
  
  // Finally, rewrite the function body to use compressed pointers!
  InstructionRewriter(PoolsToCompress, DSG, *this).visit(F);
  return true;
}


/// InitializePoolLibraryFunctions - Create the function prototypes for pointer
/// compress runtime library functions.
void PointerCompress::InitializePoolLibraryFunctions(Module &M) {
  const Type *VoidPtrTy = PointerType::get(Type::SByteTy);
  const Type *PoolDescPtrTy = PointerType::get(ArrayType::get(VoidPtrTy, 16));

  PoolInitPC = M.getOrInsertFunction("poolinit_pc", Type::VoidTy,
                                     PoolDescPtrTy, Type::UIntTy,
                                     Type::UIntTy, 0);
  PoolDestroyPC = M.getOrInsertFunction("pooldestroy_pc", Type::VoidTy,
                                        PoolDescPtrTy, 0);
  PoolAllocPC = M.getOrInsertFunction("poolalloc_pc",  Type::UIntTy,
                                      PoolDescPtrTy, Type::UIntTy, 0);
  PoolFreePC = M.getOrInsertFunction("poolfree_pc",  Type::VoidTy,
                                      PoolDescPtrTy, Type::UIntTy, 0);
  // FIXME: Need bumppointer versions as well as realloc??/memalign??
}  

bool PointerCompress::runOnModule(Module &M) {
  PoolAlloc = &getAnalysis<PoolAllocate>();
  ECG = &getAnalysis<PA::EquivClassGraphs>();

  // Create the function prototypes for pointer compress runtime library
  // functions.
  InitializePoolLibraryFunctions(M);

  // Iterate over all functions in the module, looking for compressible data
  // structures.
  bool Changed = false;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    Changed |= CompressPoolsInFunction(*I);

  return Changed;
}
