//===-- PASimple.cpp - Simple Pool Allocation Pass ------------------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// A minimal poolallocator that assignes all allocation to one common
// global pool.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "poolalloc"

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "dsa/CallTargets.h"
#include "poolalloc/PoolAllocate.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Support/CFG.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Timer.h"

#include <iostream>

using namespace llvm;
using namespace PA;

char llvm::PoolAllocateSimple::ID = 0;

namespace {
  RegisterPass<PoolAllocateSimple>
  X("poolalloc-simple", "Pool allocate everything into a single global pool");

  RegisterAnalysisGroup<PoolAllocateGroup, true> PAGroup1(X);
}

static inline Value *
castTo (Value * V, const Type * Ty, std::string Name, Instruction * InsertPt) {
  //
  // Don't bother creating a cast if it's already the correct type.
  //
  if (V->getType() == Ty)
    return V;

  //
  // If it's a constant, just create a constant expression.
  //
  if (Constant * C = dyn_cast<Constant>(V)) {
    Constant * CE = ConstantExpr::getZExtOrBitCast (C, Ty);
    return CE;
  }

  //
  // Otherwise, insert a cast instruction.
  //
  return CastInst::CreateZExtOrBitCast (V, Ty, Name, InsertPt);
}

void PoolAllocateSimple::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetData>();
  AU.addRequiredTransitive<EQTDDataStructures>();

  AU.addPreserved<EQTDDataStructures>();

  AU.setPreservesAll();
}

static void
MergeNodesInDSGraph (DSGraph & Graph) {
  std::vector<DSNodeHandle> HeapNodes;

  DSGraph::node_iterator i;
  DSGraph::node_iterator e = Graph.node_end();
  for (i = Graph.node_begin(); i != e; ++i) {
    DSNode * Node = i;
    if (Node->isHeapNode())
      HeapNodes.push_back (DSNodeHandle(Node));
  }

  for (unsigned i = 0; i < HeapNodes.size(); ++i)
    HeapNodes[i].getNode()->foldNodeCompletely();
  return;
}

bool PoolAllocateSimple::runOnModule(Module &M) {
  if (M.begin() == M.end()) return false;

  // Get the Target Data information and the Graphs
  Graphs = &getAnalysis<EQTDDataStructures>();   // folded inlined CBU graphs
  assert (Graphs && "No ECGraphs pass available!\n");
  TargetData & TD = getAnalysis<TargetData>();

  // Add the pool* prototypes to the module
  AddPoolPrototypes(&M);

  // Get the main function to insert the poolinit calls.
  Function *MainFunc = (M.getFunction("main") ? M.getFunction("main")
                                              : M.getFunction("MAIN__"));
  if (MainFunc == 0 || MainFunc->isDeclaration()) {
    std::cerr << "Cannot pool allocate this program: it has global "
              << "pools but no 'main' function yet!\n";
    return true;
  }

  //
  // Merge all of the DSNodes in the DSGraphs.
  //
  GlobalECs = Graphs->getGlobalECs();
  CombinedDSGraph = new DSGraph (GlobalECs, TD, Graphs->getGlobalsGraph());
  //CombinedDSGraph.cloneInto (getGlobalsGraph());
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    if (Graphs->hasDSGraph (*I))
      CombinedDSGraph->cloneInto (Graphs->getDSGraph(*I));
  }
  CombinedDSGraph->cloneInto (Graphs->getGlobalsGraph());
  MergeNodesInDSGraph (*CombinedDSGraph);

  //
  // Create the global pool.
  //
  TheGlobalPool = CreateGlobalPool(32, 1, MainFunc->getEntryBlock().begin(), M);

  //
  // Now that all call targets are available, rewrite the function bodies of the
  // clones.
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    if (!(I->isDeclaration()))
      ProcessFunctionBodySimple(*I, TD);
  }

  return true;
}

void
PoolAllocateSimple::ProcessFunctionBodySimple (Function& F, TargetData & TD) {
  std::vector<Instruction*> toDelete;
  std::vector<ReturnInst*> Returns;
  std::vector<Instruction*> ToFree;

  //
  // Create a silly Function Info structure for this function.
  //
  FuncInfo FInfo(F);
  FunctionInfo.insert (std::make_pair(&F, FInfo));

  //
  // Get the DSGraph for this function.
  //
  DSGraph* ECG = Graphs->getDSGraph(F);

  for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i)
    for (BasicBlock::iterator ii = i->begin(), ee = i->end(); ii != ee; ++ii) {
      if (MallocInst * MI = dyn_cast<MallocInst>(ii)) {
        // Associate the global pool decriptor with the DSNode
        DSNode * Node = ECG->getNodeForValue(MI).getNode();
        FInfo.PoolDescriptors.insert(std::make_pair(Node,TheGlobalPool));

        // Mark the malloc as an instruction to delete
        toDelete.push_back(ii);

        // Create instructions to calculate the size of the allocation in
        // bytes
        Value * AllocSize;
        if (MI->isArrayAllocation()) {
          Value * NumElements = MI->getArraySize();
          Value * ElementSize = ConstantInt::get (Type::Int32Ty,
                                                  TD.getTypePaddedSize(MI->getAllocatedType()));
          AllocSize = BinaryOperator::Create (Instruction::Mul,
                                              ElementSize,
                                              NumElements,
                                              "sizetmp",
                                              MI);
        } else {
          AllocSize = ConstantInt::get (Type::Int32Ty,
                                        TD.getTypePaddedSize(MI->getAllocatedType()));
        }

        Value* args[] = {TheGlobalPool, AllocSize};
        Instruction* x = CallInst::Create(PoolAlloc, &args[0], &args[2], MI->getName(), ii);
        ii->replaceAllUsesWith(CastInst::CreatePointerCast(x, ii->getType(), "", ii));
      } else if (CallInst * CI = dyn_cast<CallInst>(ii)) {
        CallSite CS(CI);
        Function *CF = CS.getCalledFunction();
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CS.getCalledValue()))
          if (CE->getOpcode() == Instruction::BitCast &&
              isa<Function>(CE->getOperand(0)))
            CF = cast<Function>(CE->getOperand(0));
        if (CF && (CF->isDeclaration()) && (CF->getName() == "realloc")) {
          // Associate the global pool decriptor with the DSNode
          DSNode * Node = ECG->getNodeForValue(CI).getNode();
          FInfo.PoolDescriptors.insert(std::make_pair(Node,TheGlobalPool));

          // Mark the realloc as an instruction to delete
          toDelete.push_back(ii);

          // Insertion point - Instruction before which all our instructions go
          Instruction *InsertPt = CI;
          Value *OldPtr = CS.getArgument(0);
          Value *Size = CS.getArgument(1);

          // Ensure the size and pointer arguments are of the correct type
          if (Size->getType() != Type::Int32Ty)
            Size = CastInst::CreateIntegerCast (Size,
                                                Type::Int32Ty,
                                                false,
                                                Size->getName(),
                                                InsertPt);

          static Type *VoidPtrTy = PointerType::getUnqual(Type::Int8Ty);
          if (OldPtr->getType() != VoidPtrTy)
            OldPtr = CastInst::CreatePointerCast (OldPtr,
                                                  VoidPtrTy,
                                                  OldPtr->getName(),
                                                  InsertPt);

          std::string Name = CI->getName(); CI->setName("");
          Value* Opts[3] = {TheGlobalPool, OldPtr, Size};
          Instruction *V = CallInst::Create (PoolRealloc,
                                         Opts,
                                         Opts + 3,
                                         Name,
                                         InsertPt);
          Instruction *Casted = V;
          if (V->getType() != CI->getType())
            Casted = CastInst::CreatePointerCast (V, CI->getType(), V->getName(), InsertPt);

          // Update def-use info
          CI->replaceAllUsesWith(Casted);
        } else if (CF && (CF->isDeclaration()) && (CF->getName() == "calloc")) {
          // Associate the global pool decriptor with the DSNode
          DSNode * Node = ECG->getNodeForValue(CI).getNode();
          FInfo.PoolDescriptors.insert(std::make_pair(Node,TheGlobalPool));

          // Mark the realloc as an instruction to delete
          toDelete.push_back(ii);

          // Insertion point - Instruction before which all our instructions go
          Instruction *InsertPt = CI;
          Value *NumElements = CS.getArgument(0);
          Value *Size        = CS.getArgument(1);

          // Ensure the size and pointer arguments are of the correct type
          if (Size->getType() != Type::Int32Ty)
            Size = CastInst::CreateIntegerCast (Size,
                                                Type::Int32Ty,
                                                false,
                                                Size->getName(),
                                                InsertPt);

          if (NumElements->getType() != Type::Int32Ty)
            NumElements = CastInst::CreateIntegerCast (Size,
                                                Type::Int32Ty,
                                                false,
                                                NumElements->getName(),
                                                InsertPt);

          std::string Name = CI->getName(); CI->setName("");
          Value* Opts[3] = {TheGlobalPool, NumElements, Size};
          Instruction *V = CallInst::Create (PoolCalloc,
                                             Opts,
                                             Opts + 3,
                                             Name,
                                             InsertPt);

          Instruction *Casted = V;
          if (V->getType() != CI->getType())
            Casted = CastInst::CreatePointerCast (V, CI->getType(), V->getName(), InsertPt);

          // Update def-use info
          CI->replaceAllUsesWith(Casted);
        } else if (CF && (CF->isDeclaration()) && (CF->getName() == "strdup")) {
          // Associate the global pool decriptor with the DSNode
          DSNode * Node = ECG->getNodeForValue(CI).getNode();
          FInfo.PoolDescriptors.insert(std::make_pair(Node,TheGlobalPool));

          // Mark the realloc as an instruction to delete
          toDelete.push_back(ii);

          // Insertion point - Instruction before which all our instructions go
          Instruction *InsertPt = CI;
          Value *OldPtr = CS.getArgument(0);

          // Ensure the size and pointer arguments are of the correct type
          static Type *VoidPtrTy = PointerType::getUnqual(Type::Int8Ty);
          if (OldPtr->getType() != VoidPtrTy)
            OldPtr = CastInst::CreatePointerCast (OldPtr,
                                                  VoidPtrTy,
                                                  OldPtr->getName(),
                                                  InsertPt);

          std::string Name = CI->getName(); CI->setName("");
          Value* Opts[2] = {TheGlobalPool, OldPtr};
          Instruction *V = CallInst::Create (PoolStrdup,
                                         Opts,
                                         Opts + 2,
                                         Name,
                                         InsertPt);
          Instruction *Casted = V;
          if (V->getType() != CI->getType())
            Casted = CastInst::CreatePointerCast (V, CI->getType(), V->getName(), InsertPt);

          // Update def-use info
          CI->replaceAllUsesWith(Casted);
        }
      } else if (FreeInst * FI = dyn_cast<FreeInst>(ii)) {
        Type * VoidPtrTy = PointerType::getUnqual(Type::Int8Ty);
        Value * FreedNode = castTo (FI->getPointerOperand(), VoidPtrTy, "cast", ii);
        toDelete.push_back(ii);
        Value* args[] = {TheGlobalPool, FreedNode};
        CallInst::Create(PoolFree, &args[0], &args[2], "", ii);
      } else if (isa<ReturnInst>(ii)) {
        Returns.push_back(cast<ReturnInst>(ii));
      }
    }

  //add frees at each return for the allocas
  for (std::vector<ReturnInst*>::iterator i = Returns.begin(), e = Returns.end();
       i != e; ++i)
    for (std::vector<Instruction*>::iterator ii = ToFree.begin(), ee = ToFree.end();
         ii != ee; ++ii) {
        std::vector<Value*> args;
        args.push_back (TheGlobalPool);
        args.push_back (*ii);
        CallInst::Create(PoolFree, args.begin(), args.end(), "", *i);
    }
  
  //delete malloc and alloca insts
  for (unsigned x = 0; x < toDelete.size(); ++x)
    toDelete[x]->eraseFromParent();
}

/// CreateGlobalPool - Create a global pool descriptor object, and insert a
/// poolinit for it into main.  IPHint is an instruction that we should insert
/// the poolinit before if not null.
GlobalVariable *
PoolAllocateSimple::CreateGlobalPool (unsigned RecSize,
                                      unsigned Align,
                                      Instruction *IPHint,
                                      Module& M) {
  GlobalVariable *GV =
    new GlobalVariable(getPoolType(), false, GlobalValue::InternalLinkage, 
                       Constant::getNullValue(getPoolType()), "GlobalPool",
                       &M);

  Function *MainFunc = (M.getFunction("main") ? M.getFunction("main")
                                              : M.getFunction("MAIN__"));
  assert(MainFunc && "No main in program??");

  BasicBlock::iterator InsertPt;
  if (IPHint)
    InsertPt = IPHint;
  else {
    InsertPt = MainFunc->getEntryBlock().begin();
    while (isa<AllocaInst>(InsertPt)) ++InsertPt;
  }

  Value *ElSize = ConstantInt::get(Type::Int32Ty, RecSize);
  Value *AlignV = ConstantInt::get(Type::Int32Ty, Align);
  Value* Opts[3] = {GV, ElSize, AlignV};
  CallInst::Create(PoolInit, Opts, Opts + 3, "", InsertPt);
  return GV;
}

