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
    typedef std::map<const DSNode*, CompressedPoolInfo> PoolInfoMap;

    bool runOnModule(Module &M);
    void getAnalysisUsage(AnalysisUsage &AU) const;

  private:
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
    const Type *NewTy;
  public:
    CompressedPoolInfo(const DSNode *N) : Pool(N), NewTy(0) {}
    
    /// Initialize - When we know all of the pools in a function that are going
    /// to be compressed, initialize our state based on that data.
    void Initialize(std::map<const DSNode*, CompressedPoolInfo> &Nodes);

    const DSNode *getNode() const { return Pool; }
    const Type *getNewType() const { return NewTy; }

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
                                             CompressedPoolInfo> &Nodes) {
  // First step, compute the type of the compressed node.  This basically
  // replaces all pointers to compressed pools with uints.
  NewTy = ComputeCompressedType(Pool->getType(), 0, Nodes);

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
  class InstructionRewriter : public llvm::InstVisitor<InstructionRewriter> {
    /// OldToNewValueMap - This keeps track of what new instructions we create
    /// for instructions that used to produce pointers into our pool.
    std::map<Value*, Value*> OldToNewValueMap;
  
    /// ForwardRefs - If a value is refered to before it is defined, create a
    /// temporary Argument node as a placeholder.  When the value is really
    /// defined, call replaceAllUsesWith on argument and remove it from this
    /// map.
    std::map<Value*, Argument*> ForwardRefs;

    const PointerCompress::PoolInfoMap &PoolInfo;

    const DSGraph &DSG;
  public:
    InstructionRewriter(const PointerCompress::PoolInfoMap &poolInfo,
                        const DSGraph &dsg)
      : PoolInfo(poolInfo), DSG(dsg) {
    }
    ~InstructionRewriter() {
      assert(ForwardRefs.empty() && "Unresolved forward refs exist!");
    }

    void visitInstruction(Instruction &I) {
      std::cerr << "ERROR: UNHANDLED INSTRUCTION: " << I;
      //assert(0);
      //abort();
    }
  };
} // end anonymous namespace.




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
  for (unsigned i = 0, e = PoolsToCompressList.size(); i != e; ++i)
    PoolsToCompress.insert(std::make_pair(PoolsToCompressList[i], 
                                          PoolsToCompressList[i]));

  // Use these to compute the closure of compression information.  In
  // particular, if one pool points to another, we need to know if the outgoing
  // pointer is compressed.
  std::cerr << "In function '" << F.getName() << "':\n";
  for (std::map<const DSNode*, CompressedPoolInfo>::iterator
         I = PoolsToCompress.begin(), E = PoolsToCompress.end(); I != E; ++I) {
    I->second.Initialize(PoolsToCompress);
    std::cerr << "  COMPRESSING POOL:\nPCS:";
    I->second.dump();
  }
  
  // Finally, rewrite the function body to use compressed pointers!
  InstructionRewriter(PoolsToCompress, DSG).visit(F);
  return true;
}

bool PointerCompress::runOnModule(Module &M) {
  PoolAlloc = &getAnalysis<PoolAllocate>();
  ECG = &getAnalysis<PA::EquivClassGraphs>();

  // Iterate over all functions in the module, looking for compressible data
  // structures.
  bool Changed = false;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    Changed |= CompressPoolsInFunction(*I);

  return Changed;
}
