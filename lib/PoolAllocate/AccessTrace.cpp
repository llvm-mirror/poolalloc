//===-- PoolAccessTrace.cpp - Build trace of loads ------------------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements the -poolaccesstrace pass.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pointercompress"
#include "poolalloc/PoolAllocate.h"
#include "llvm/Analysis/DataStructure/DataStructure.h"
#include "llvm/Analysis/DataStructure/DSGraph.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
using namespace llvm;

namespace {

  /// PoolAccessTrace - This transformation adds instrumentation to the program
  /// to print a trace of pairs containing the address of each load and the pool
  /// descriptor loaded from.
  class PoolAccessTrace : public ModulePass {
    PoolAllocate *PoolAlloc;
    EquivClassGraphs *ECG;
    Function *AccessTraceInitFn, *PoolAccessTraceFn;
    const Type *VoidPtrTy;
  public:

    bool runOnModule(Module &M);

    void getAnalysisUsage(AnalysisUsage &AU) const;

    const DSGraph &getGraphForFunc(PA::FuncInfo *FI) const {
      return ECG->getDSGraph(FI->F);
    }

  private:
    void InitializeLibraryFunctions(Module &M);
    void InstrumentAccess(Instruction *I, Value *Ptr, 
                          PA::FuncInfo *FI, DSGraph &DSG);
  };

  RegisterOpt<PoolAccessTrace>
  X("poolaccesstrace", "Instrument program to print trace of accesses");
}

void PoolAccessTrace::getAnalysisUsage(AnalysisUsage &AU) const {
  // Need information about how pool allocation happened.
  AU.addRequired<PoolAllocatePassAllPools>();

  // Need information from DSA.
  AU.addRequired<EquivClassGraphs>();
}

void PoolAccessTrace::InitializeLibraryFunctions(Module &M) {
  VoidPtrTy = PointerType::get(Type::SByteTy);

  AccessTraceInitFn = M.getOrInsertFunction("poolaccesstraceinit",
                                            Type::VoidTy,NULL);
  PoolAccessTraceFn = M.getOrInsertFunction("poolaccesstrace", Type::VoidTy,
                                            VoidPtrTy, VoidPtrTy, NULL);
}

void PoolAccessTrace::InstrumentAccess(Instruction *I, Value *Ptr, 
                                       PA::FuncInfo *FI, DSGraph &DSG) {
  // Don't trace loads of globals or the stack.
  if (isa<Constant>(Ptr) || isa<AllocaInst>(Ptr)) return;

  Value *MappedPtr = Ptr;
  if (!FI->NewToOldValueMap.empty())
    if ((MappedPtr = FI->MapValueToOriginal(MappedPtr)) == 0) {
      // Value didn't exist in the orig program (pool desc?).
      return;
    }
  DSNode *Node = DSG.getNodeForValue(MappedPtr).getNode();
  if (Node == 0) return;

  Value *PD = FI->PoolDescriptors[Node];
  Ptr = new CastInst(Ptr, VoidPtrTy, Ptr->getName(), I);

  if (PD)
    PD = new CastInst(PD, VoidPtrTy, PD->getName(), I);
  else
    PD = Constant::getNullValue(VoidPtrTy);

  // Insert the trace call.
  new CallInst(PoolAccessTraceFn, Ptr, PD, "", I);
}

bool PoolAccessTrace::runOnModule(Module &M) {
  PoolAlloc = &getAnalysis<PoolAllocatePassAllPools>();
  ECG = &getAnalysis<EquivClassGraphs>();

  // Create the function prototypes for runtime library.
  InitializeLibraryFunctions(M);

  Function *MainFunc = M.getMainFunction();
  if (MainFunc && !MainFunc->isExternal())
    // Insert a call to the library init function into the beginning of main.
    new CallInst(AccessTraceInitFn, "", MainFunc->begin()->begin());

  // Look at all of the loads in the program.
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    if (F->isExternal()) continue;

    PA::FuncInfo *FI = PoolAlloc->getFuncInfoOrClone(*F);
    assert(FI && "DIDN'T FIND POOL INFO!");

    // If this function was cloned, and this is the original function, ignore it
    // (it's dead).  We'll deal with the cloned version later when we run into
    // it again.
    if (FI->Clone && &FI->F == F)
      continue;

    // Get the DSGraph for this function.
    DSGraph &DSG = ECG->getDSGraph(FI->F);

    for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
        if (LoadInst *LI = dyn_cast<LoadInst>(I))
          InstrumentAccess(LI, LI->getOperand(0), FI, DSG);
  }
  return true;
}
