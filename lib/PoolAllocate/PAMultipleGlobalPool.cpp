//===-- PAMultipleGlobalPool.cpp - Multiple Global Pool Allocation Pass ---===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// A minimal poolallocator that assignes all allocation to multiple global
// pools.
//
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

char llvm::PoolAllocateMultipleGlobalPool::ID = 0;

namespace {
  RegisterPass<PoolAllocateMultipleGlobalPool>
  X("poolalloc-multi-global-pool", "Pool allocate objects into multiple global pools");

  RegisterAnalysisGroup<PoolAllocateGroup> PAGroup1(X);
}

static inline Value *
castTo (Value * V, const Type * Ty, const std::string & Name, Instruction * InsertPt) {
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

void PoolAllocateMultipleGlobalPool::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetData>();
  AU.addRequired<SteensgaardDataStructures>();
  // It is a big lie.
  AU.setPreservesAll();
}

bool PoolAllocateMultipleGlobalPool::runOnModule(Module &M) {
  currentModule = &M;
  if (M.begin() == M.end()) return false;
  Graphs = &getAnalysis<SteensgaardDataStructures>();
  assert (Graphs && "No DSA pass available!\n");

  TargetData & TD = getAnalysis<TargetData>();

  // Add the pool* prototypes to the module
  AddPoolPrototypes(&M);

  //
  // Create the global pool.
  //
  CreateGlobalPool(32, 1, M);

  //
  // Now that all call targets are available, rewrite the function bodies of the
  // clones.
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    std::string name = I->getName();
    if (name == "__poolalloc_init") continue;
    if (!(I->isDeclaration()))
      ProcessFunctionBodySimple(*I, TD);
  }

  return true;
}

void
PoolAllocateMultipleGlobalPool::ProcessFunctionBodySimple (Function& F, TargetData & TD) {
  std::vector<Instruction*> toDelete;
  std::vector<ReturnInst*> Returns;

  //
  // Create a silly Function Info structure for this function.
  //
  FuncInfo FInfo(F);
  FunctionInfo.insert (std::make_pair(&F, FInfo));

  //
  // Get the DSGraph for this function.
  //
  DSGraph* ECG = Graphs->getDSGraph(F);
  DSScalarMap & SM = ECG->getScalarMap();

  for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i)
    for (BasicBlock::iterator ii = i->begin(), ee = i->end(); ii != ee; ++ii) {
      if (MallocInst * MI = dyn_cast<MallocInst>(ii)) {
        // Associate the global pool decriptor with the DSNode
        DSNode * Node = ECG->getNodeForValue(MI).getNode();
        GlobalVariable * Pool = PoolMap[Node];
        FInfo.PoolDescriptors.insert(std::make_pair(Node,Pool));

        // Mark the malloc as an instruction to delete
        toDelete.push_back(ii);

        // Create instructions to calculate the size of the allocation in
        // bytes
        Value * AllocSize;
        if (MI->isArrayAllocation()) {
          Value * NumElements = MI->getArraySize();
          Value * ElementSize = Context->getConstantInt (Type::Int32Ty,
                                                  TD.getTypeAllocSize(MI->getAllocatedType()));
          AllocSize = BinaryOperator::Create (Instruction::Mul,
                                              ElementSize,
                                              NumElements,
                                              "sizetmp",
                                              MI);
        } else {
          AllocSize = Context->getConstantInt (Type::Int32Ty,
                                        TD.getTypeAllocSize(MI->getAllocatedType()));
        }

        Value* args[] = {Pool, AllocSize};
        Instruction* x = CallInst::Create(PoolAlloc, &args[0], &args[2], MI->getName(), ii);
        Value * casted = castTo(x, ii->getType(), "", ii);
        ii->replaceAllUsesWith(casted);

        // Update scalar map
        DSNodeHandle NH = SM[ii];
        SM.erase(ii);
        SM[casted] = SM[x] = NH;
        
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
          GlobalVariable * Pool = PoolMap[Node];

          FInfo.PoolDescriptors.insert(std::make_pair(Node,Pool));

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
          Value* Opts[3] = {Pool, OldPtr, Size};
          Instruction *V = CallInst::Create (PoolRealloc,
                                         Opts,
                                         Opts + 3,
                                         Name,
                                         InsertPt);
          Value *Casted = castTo(V, CI->getType(), V->getName(), InsertPt);

          // Update def-use info
          CI->replaceAllUsesWith(Casted);

          // Update scalar map
          DSNodeHandle NH = SM[CI];
          SM.erase(CI);
          SM[Casted] = SM[V] = NH;
        } else if (CF && (CF->isDeclaration()) && (CF->getName() == "calloc")) {
          // Associate the global pool decriptor with the DSNode
          DSNode * Node = ECG->getNodeForValue(CI).getNode();
          GlobalVariable * Pool = PoolMap[Node];
          FInfo.PoolDescriptors.insert(std::make_pair(Node,Pool));

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
          Value* Opts[3] = {Pool, NumElements, Size};
          Instruction *V = CallInst::Create (PoolCalloc,
                                             Opts,
                                             Opts + 3,
                                             Name,
                                             InsertPt);

          Value *Casted = castTo(V, CI->getType(), V->getName(), InsertPt);

          // Update def-use info
          CI->replaceAllUsesWith(Casted);

          // Update scalar map
          DSNodeHandle NH = SM[CI];
          SM.erase(CI);
          SM[Casted] = SM[V] = NH;
        } else if (CF && (CF->isDeclaration()) && (CF->getName() == "strdup")) {
          // Associate the global pool decriptor with the DSNode
          DSNode * Node = ECG->getNodeForValue(CI).getNode();
          GlobalVariable * Pool = PoolMap[Node];
          FInfo.PoolDescriptors.insert(std::make_pair(Node, Pool));

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
          Value* Opts[2] = {Pool, OldPtr};
          Instruction *V = CallInst::Create (PoolStrdup,
                                         Opts,
                                         Opts + 2,
                                         Name,
                                         InsertPt);
          Value *Casted = castTo(V, CI->getType(), V->getName(), InsertPt);

          // Update def-use info
          CI->replaceAllUsesWith(Casted);

          // Update scalar map
          DSNodeHandle NH = SM[CI];
          SM.erase(CI);
          SM[Casted] = SM[V] = NH;
        }
      } else if (FreeInst * FI = dyn_cast<FreeInst>(ii)) {
        Type * VoidPtrTy = PointerType::getUnqual(Type::Int8Ty);
        Value * FreedNode = castTo (FI->getPointerOperand(), VoidPtrTy, "cast", ii);
        DSNode * Node = ECG->getNodeForValue(FI->getPointerOperand()).getNode();
        GlobalVariable * Pool = PoolMap[Node];
        assert (Pool && "No Pool Handle for poolfree()!");
        toDelete.push_back(ii);
        Value* args[] = {Pool, FreedNode};
        CallInst * CI = CallInst::Create(PoolFree, &args[0], &args[2], "", ii);
        // Update scalar map
        DSNodeHandle NH = SM[ii];
        SM.erase(ii);
        SM[CI] = NH;
      } else if (isa<ReturnInst>(ii)) {
        Returns.push_back(cast<ReturnInst>(ii));
      }
    }
  
  //delete malloc and alloca insts
  for (unsigned x = 0; x < toDelete.size(); ++x)
    toDelete[x]->eraseFromParent();
}

/// CreateGlobalPool - Create a global pool descriptor object, and insert a
/// poolinit for it into poolalloc.init
void
PoolAllocateMultipleGlobalPool::CreateGlobalPool (unsigned RecSize,
                                      unsigned Align,
                                      Module& M) {

  Function *InitFunc = Function::Create
    (FunctionType::get(Type::VoidTy, std::vector<const Type*>(), false),
    GlobalValue::ExternalLinkage, "__poolalloc_init", &M);

  BasicBlock * BB = BasicBlock::Create("entry", InitFunc);
  
  SteensgaardDataStructures * DS = dynamic_cast<SteensgaardDataStructures*>(Graphs);
  
  assert (DS && "PoolAllocateMultipleGlobalPools requires Steensgaard Data Structure!");

  //
  // Create a pool for each node within the DSGraph.
  //
  DSGraph * G = DS->getResultGraph();
  for (DSGraph::node_const_iterator I = G->node_begin(), 
        E = G->node_end(); I != E; ++I) {
    generatePool (I->getSize(), Align, M, BB, I);
  }

  DSGraph * GG = DS->getGlobalsGraph();
  for(DSGraph::node_const_iterator I = GG->node_begin(), 
        E = GG->node_end(); I != E; ++I) {
    if (I->globals_begin() != I->globals_end()) {
      const GlobalValue * GV = *(I->globals_begin());
      DSNodeHandle NH = G->getNodeForValue(GV);
      if (!NH.isNull()) {
        PoolMap[&*I] = PoolMap[NH.getNode()];
      } else {
        generatePool(I->getSize(), Align, M, BB, I);
      }
    }
  }

  ReturnInst::Create(BB);
}

void
PoolAllocateMultipleGlobalPool::generatePool(unsigned RecSize,
                                             unsigned Align,
                                             Module& M,
                                             BasicBlock * InsertAtEnd, 
                                             const DSNode * Node) {

  if (!PoolMap[Node]) {
    GlobalVariable *GV =
      new GlobalVariable
      (M,
       getPoolType(), false, GlobalValue::ExternalLinkage, 
       ConstantAggregateZero::get(getPoolType()),
       "__poolalloc_GlobalPool");

    Value *ElSize = Context->getConstantInt(Type::Int32Ty, RecSize);
    Value *AlignV = Context->getConstantInt(Type::Int32Ty, Align);
    Value* Opts[3] = {GV, ElSize, AlignV};
    
    CallInst::Create(PoolInit, Opts, Opts + 3, "", InsertAtEnd);
    PoolMap[Node] = GV;
  }
}


Value *
PoolAllocateMultipleGlobalPool::getGlobalPool (const DSNode * Node) {
  Value * Pool = PoolMap[Node];
  assert (Pool && "Every DSNode corresponds to a pool handle!");
  return Pool;
}

Value *
PoolAllocateMultipleGlobalPool::getPool (const DSNode * N, Function & F) {
  return getGlobalPool(N);
}

void
PoolAllocateMultipleGlobalPool::print(std::ostream &OS, const Module * M) const {
  for (PoolMapTy::const_iterator I = PoolMap.begin(), E = PoolMap.end(); I != E; ++I) {
     OS << I->first << " -> " << I->second->getName() << "\n";
  } 
}

void
PoolAllocateMultipleGlobalPool::dump() const {
  print (std::cerr, currentModule);
}

PoolAllocateMultipleGlobalPool::~PoolAllocateMultipleGlobalPool() {}
