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
using namespace llvm;

namespace {
  Statistic<> NumCompressed("pointercompress",
                            "Number of pools pointer compressed");
  Statistic<> NumNotCompressed("pointercompress",
                               "Number of pools not compressible");


  /// PointerCompress - This transformation hacks on type-safe pool allocated
  /// data structures to reduce the size of pointers in the program.
  class PointerCompress : public ModulePass {
    PoolAllocate *PoolAlloc;
    PA::EquivClassGraphs *ECG;
  public:
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
  std::vector<const DSNode*> PoolsToCompress;
  FindPoolsToCompress(PoolsToCompress, F, DSG, FI);

  if (PoolsToCompress.empty()) return false;

  std::cerr << "In function '" << F.getName() << "':\n";
  for (unsigned i = 0, e = PoolsToCompress.size(); i != e; ++i) {
    std::cerr << "  COMPRESSING POOL:\nPCS:";
    PoolsToCompress[i]->dump();
  }
  return false;
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
