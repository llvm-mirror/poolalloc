//===-- TransformFunctionBody.cpp - Pool Function Transformer -------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines the PoolAllocate::TransformBody method.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "PoolAllocator"

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "dsa/CallTargets.h"
#include "poolalloc/PoolAllocate.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/VectorExtras.h"
#include <iostream>
using namespace llvm;
using namespace PA;

namespace {
  /// FuncTransform - This class implements transformation required of pool
  /// allocated functions.
  struct FuncTransform : public InstVisitor<FuncTransform> {
    PoolAllocate &PAInfo;
    DSGraph* G;      // The Bottom-up DS Graph
    FuncInfo &FI;

    // PoolUses - For each pool (identified by the pool descriptor) keep track
    // of which blocks require the memory in the pool to not be freed.  This
    // does not include poolfree's.  Note that this is only tracked for pools
    // for which the given function is the pool's home i.e., the pool is an
    // alloca instruction because it is allocated within the current function.
    std::multimap<AllocaInst*, Instruction*> &PoolUses;

    // PoolFrees - For each pool, keep track of the actual poolfree calls
    // inserted into the code.  This is seperated out from PoolUses.
    std::multimap<AllocaInst*, CallInst*> &PoolFrees;

    FuncTransform(PoolAllocate &P, DSGraph* g, FuncInfo &fi,
                  std::multimap<AllocaInst*, Instruction*> &poolUses,
                  std::multimap<AllocaInst*, CallInst*> &poolFrees)
      : PAInfo(P), G(g), FI(fi), 
        PoolUses(poolUses), PoolFrees(poolFrees) {
    }

    template <typename InstType, typename SetType>
    void AddPoolUse(InstType &I, Value *PoolHandle, SetType &Set) {
      // FIXME: Strip away pointer casts
      if (AllocaInst *AI = dyn_cast<AllocaInst>(PoolHandle))
        Set.insert(std::make_pair(AI, &I));
    }

    // FIXME: Factor out assumptions about c stdlib function names
    void visitInstruction(Instruction &I);
    //void visitMallocInst(MallocInst &MI);
    void visitAllocaInst(AllocaInst &MI);
    void visitMallocCall(CallSite & CS);
    void visitCallocCall(CallSite CS);
    void visitReallocCall(CallSite CS);
    void visitMemAlignCall(CallSite CS);
    void visitStrdupCall(CallSite CS);
    void visitRuntimeCheck(CallSite CS);
    //void visitFreeInst(FreeInst &FI);
    void visitFreeCall(CallSite &CS);
    void visitCallSite(CallSite &CS);
    void visitCallInst(CallInst &CI) {
      CallSite CS(&CI);
      visitCallSite(CS);
    }
    void visitInvokeInst(InvokeInst &II) {
      CallSite CS(&II);
      visitCallSite(CS);
    }
    void visitLoadInst(LoadInst &I);
    void visitStoreInst (StoreInst &I);

  private:
    Instruction *TransformAllocationInstr(Instruction *I, Value *Size);
    Instruction *InsertPoolFreeInstr(Value *V, Instruction *Where);

    //
    // Method: UpdateNewToOldValueMap()
    //
    // Description:
    //  This method removes the old mapping indexed by OldVal and inserts one
    //  or two new mappings mapping NewV1 and NewV2 to the value that was
    //  indexed by OldVal.
    //
    void UpdateNewToOldValueMap(Value *OldVal, Value *NewV1, Value *NewV2 = 0) {
      std::map<Value*, const Value*>::iterator I =
        FI.NewToOldValueMap.find(OldVal);
      assert(I != FI.NewToOldValueMap.end() && "OldVal not found in clone?");
      FI.NewToOldValueMap.insert(std::make_pair(NewV1, I->second));
      if (NewV2)
        FI.NewToOldValueMap.insert(std::make_pair(NewV2, I->second));
      FI.NewToOldValueMap.erase(I);
    }

    Value* getOldValueIfAvailable(Value* V) {
      if (!FI.NewToOldValueMap.empty()) {
        // If the NewToOldValueMap is in effect, use it.
        std::map<Value*,const Value*>::iterator I = FI.NewToOldValueMap.find(V);
        if (I != FI.NewToOldValueMap.end())
          V = (Value*)I->second;
      }
      return V;
    }

    DSNodeHandle& getDSNodeHFor(Value *V) {
      return G->getScalarMap()[getOldValueIfAvailable(V)];
    }

    // FIXME: Does this get global (or just local) pools?
    Value *getPoolHandle(Value *V) {
      DSNode *Node = getDSNodeHFor(V).getNode();
      // Get the pool handle for this DSNode...
      std::map<const DSNode*, Value*>::iterator I =
        FI.PoolDescriptors.find(Node);
      return I != FI.PoolDescriptors.end() ? I->second : 0;
    }

    Function* retCloneIfFunc(Value *V);

    void verifyCallees (const std::vector<const Function *> & Functions);
  };
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

void
PoolAllocate::TransformBody (DSGraph* g, PA::FuncInfo &fi,
                             std::multimap<AllocaInst*,Instruction*> &poolUses,
                             std::multimap<AllocaInst*, CallInst*> &poolFrees,
                             Function &F) {
  FuncTransform(*this, g, fi, poolUses, poolFrees).visit(F);
}

//
// Method: verifyCallees()
//
// Description:
//  This method performs various sanity checks on targets of indirect function
//  calls.
//
void
FuncTransform::verifyCallees (const std::vector<const Function *> & Functions) {
  //
  // There's nothing to do if there's no function call targets at all.
  //
  if (Functions.size() == 0)
    return;

  //
  // Get the number of pool arguments for the first function.
  //
  unsigned numPoolArgs = PAInfo.getFuncInfo(*Functions[0])->ArgNodes.size();

  //
  // Get the DSGraph of the first function.
  //
  DataStructures& Graphs = PAInfo.getGraphs();
  DSGraph * firstGraph = Graphs.getDSGraph (*Functions[0]);

  //
  // Scan through all other indirect function call targets.  Ensure that these
  // functions have the same number of pool arguments and the same DSGraph as
  // the first function.
  //
  for (unsigned index = 1; index < Functions.size(); ++index) {
    //
    // Get the function information for the function.
    //
    const Function * F = Functions[index];
    FuncInfo *FI = PAInfo.getFuncInfo(*F);

    //
    // Get the DSGraph.
    //
    DSGraph * G = Graphs.getDSGraph (*Functions[index]);

    //
    // Assert that the graph and number of pool arguments are consistent.
    //
    assert (G == firstGraph);
    assert (FI->ArgNodes.size() == numPoolArgs);
  }

  return;
}

// Returns the clone if  V is a static function (not a pointer) and belongs 
// to an equivalence class i.e. is pool allocated
// FIXME: Rename this to 'getCloneIfFunc' (or similar)?
// FIXME: Strip pointer casts
// FIXME: Comment this?
Function* FuncTransform::retCloneIfFunc(Value *V) {
  if (Function *F = dyn_cast<Function>(V))
    if (FuncInfo *FI = PAInfo.getFuncInfo(*F))
      return FI->Clone;

  return 0;
}

void FuncTransform::visitLoadInst(LoadInst &LI) {
  //
  // Record the use of the pool handle for the pointer being dereferenced.
  //
  if (Value *PH = getPoolHandle(LI.getOperand(0)))
    AddPoolUse(LI, PH, PoolUses);

  //
  // If this is a volatile load, then record a use of the pool handle for the
  // loaded value, even if it is never used.
  //
  if (LI.isVolatile()) {
    if (Value *PH = getPoolHandle(&LI))
      AddPoolUse (LI, PH, PoolUses);
  }

  visitInstruction(LI);
}

void FuncTransform::visitStoreInst(StoreInst &SI) {
  if (Value *PH = getPoolHandle(SI.getOperand(1)))
    AddPoolUse(SI, PH, PoolUses);
  visitInstruction(SI);
}

Instruction *FuncTransform::TransformAllocationInstr(Instruction *I,
                                                     Value *Size) {
  // Ensure that the new instruction has the same name as the old one
  // and the that the old one has no name.
  std::string Name = I->getName(); I->setName("");

  //
  // FIXME: Don't assume allocation sizes are 32-bit; different architectures
  // have different limits on the size of memory objects that they can
  // allocate.
  //
  if (!Size->getType()->isIntegerTy(32))
    Size = CastInst::CreateIntegerCast(Size, Type::getInt32Ty(Size->getType()->getContext()), false, Size->getName(), I);


  // Get the pool handle--
  // Do not change the instruction into a poolalloc() call unless we have a
  // real pool descriptor
  Value *PH = getPoolHandle(I);
  if (PH == 0 || isa<ConstantPointerNull>(PH)) return I;

  // Create call to poolalloc, and record the use of the pool
  Value* Opts[2] = {PH, Size};
  Instruction *V = CallInst::Create(PAInfo.PoolAlloc, Opts, Opts + 2, Name, I);
  AddPoolUse(*V, PH, PoolUses);

  // Cast to the appropriate type if necessary
  // FIXME: Make use of "castTo" utility function
  Instruction *Casted = V;
  if (V->getType() != I->getType())
    Casted = CastInst::CreatePointerCast(V, I->getType(), V->getName(), I);

  // Update def-use info
  I->replaceAllUsesWith(Casted);

  // If we are modifying the original function, update the DSGraph.
  if (!FI.Clone) {
    // V and Casted now point to whatever the original allocation did.
    G->getScalarMap().replaceScalar(I, V);
    if (V != Casted)
      G->getScalarMap()[Casted] = G->getScalarMap()[V];
  } else {             // Otherwise, update the NewToOldValueMap
    UpdateNewToOldValueMap(I, V, V != Casted ? Casted : 0);
  }

  // If this was an invoke, fix up the CFG.
  if (InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    // FIXME: Assert out since we potentially don't handle "invoke" correctly
    BranchInst::Create (II->getNormalDest(), I);
    II->getUnwindDest()->removePredecessor(II->getParent(), true);
  }

  // Remove old allocation instruction.
  I->eraseFromParent();
  return Casted;
}

void FuncTransform::visitAllocaInst(AllocaInst &MI) {
  // FIXME: We should remove SAFECode-specific functionality (and comments)
  // SAFECode will register alloca instructions with the run-time, so do not
  // do that here.
  //
  // FIXME:
  //  There is a chance that we may need to update PoolUses to make sure that
  //  the pool handle is available in this function.
  //
  return;
}

//
// Method: InsertPoolFreeInstr()
//
// Description:
//  Insert a call to poolfree() at the specified point to free the specified
//  value.
//
// Inputs:
//  Arg   - The value that should be freed by the call to poolfree().
//  Where - The instruction before which the poolfree() call should be
//          inserted.
//
// Return value:
//  NULL - No call to poolfree() was inserted.
//      This may be possible if PoolAlloc has decided not to pool-allocate this.
//  Otherwise, a pointer to the call instruction that calls poolfree() will be
//  returned.
//
Instruction *
FuncTransform::InsertPoolFreeInstr (Value *Arg, Instruction *Where){
  //
  // Attempt to get the pool handle of the specified value.  If there is no
  // pool handle, then just return NULL.
  //
  Value *PH = getPoolHandle(Arg);  // Get the pool handle for this DSNode...
  if (PH == 0 || isa<ConstantPointerNull>(PH)) return 0;

  //
  // Cast the pointer to be freed to a void pointer type if necessary.
  //
  // FIXME: Change this to make use of "castTo" utility.
  Value *Casted = Arg;
  if (Arg->getType() != PointerType::getUnqual(Type::getInt8Ty(Arg->getContext()))) {
    Casted = CastInst::CreatePointerCast(Arg, PointerType::getUnqual(Type::getInt8Ty(Arg->getContext())),
				 Arg->getName()+".casted", Where);
    G->getScalarMap()[Casted] = G->getScalarMap()[Arg];
  }

  //
  // Insert a call to poolfree(), and mark that memory was deallocated from the pool.
  //
  Value* Opts[2] = {PH, Casted};
  CallInst *FreeI = CallInst::Create(PAInfo.PoolFree, Opts, Opts + 2, "", Where);
  AddPoolUse(*FreeI, PH, PoolFrees);
  return FreeI;
}

#if 0
void FuncTransform::visitFreeInst(FreeInst &FrI) {
  if (Instruction *I = InsertPoolFreeInstr(FrI.getOperand(0), &FrI)) {
    // Delete the now obsolete free instruction...
    FrI.getParent()->getInstList().erase(&FrI);

    // Update the NewToOldValueMap if this is a clone
    if (!FI.NewToOldValueMap.empty()) {
      std::map<Value*,const Value*>::iterator II =
        FI.NewToOldValueMap.find(&FrI);
      assert(II != FI.NewToOldValueMap.end() && 
             "FrI not found in clone?");
      FI.NewToOldValueMap.insert(std::make_pair(I, II->second));
      FI.NewToOldValueMap.erase(II);
    }
  }
}
#endif

void
FuncTransform::visitFreeCall (CallSite & CS) {
  //
  // Replace the call to the free() function with a call to poolfree().
  //
  Instruction * InsertPt = CS.getInstruction();
  if (Instruction *I = InsertPoolFreeInstr (CS.getArgument(0), InsertPt)) {
    // Delete the now obsolete free instruction...
    // FIXME: use "eraseFromParent"? (Note this might require a refactoring)
    InsertPt->getParent()->getInstList().erase(InsertPt);
 
    // Update the NewToOldValueMap if this is a clone
    // FIXME: Use of utility function UpdateNewToOldValueMap
    if (!FI.NewToOldValueMap.empty()) {
      std::map<Value*,const Value*>::iterator II =
        FI.NewToOldValueMap.find(InsertPt);
      assert(II != FI.NewToOldValueMap.end() && 
             "free call not found in clone?");
      FI.NewToOldValueMap.insert(std::make_pair(I, II->second));
      FI.NewToOldValueMap.erase(II);
    }
  }
}

void
FuncTransform::visitMallocCall(CallSite &CS) {
  //
  // Get the instruction to which the call site refers
  //
  Instruction * MI = CS.getInstruction();

  //
  // Get the pool handle for the node that this contributes to...
  //
  // FIXME: This check may be redundant
  Value *PH = getPoolHandle(MI);
  if (PH == 0 || isa<ConstantPointerNull>(PH)) return;

  //
  // Find the size of the allocation.
  //
  Value *AllocSize = CS.getArgument(0);

  //
  // Transform the allocation site to use poolalloc().
  //
  TransformAllocationInstr(MI, AllocSize);
}


void FuncTransform::visitCallocCall(CallSite CS) {
  TargetData& TD = PAInfo.getAnalysis<TargetData>();
  const Type* Int8Type = Type::getInt8Ty(CS.getInstruction()->getContext());
  const Type* Int32Type = Type::getInt32Ty(CS.getInstruction()->getContext());
  const Type* Int64Type = Type::getInt64Ty(CS.getInstruction()->getContext());

  // FIXME: Ensure that we use 32/64-bit object length sizes consistently
  // FIXME: Rename 'useLong' to something more descriptive?
  // FIXME: Introduce 'ObjectAllocationSize' variable
  //        or similar instead of repeatedly using same expression
  // XXX: Start new review session here ***
  bool useLong = TD.getTypeAllocSize(PointerType::getUnqual(Int8Type)) != 4;

  Module *M = CS.getInstruction()->getParent()->getParent()->getParent();
  assert(CS.arg_end()-CS.arg_begin() == 2 && "calloc takes two arguments!");
  Value *V1 = CS.getArgument(0);
  Value *V2 = CS.getArgument(1);
  if (V1->getType() != V2->getType()) {
    V1 = CastInst::CreateZExtOrBitCast(V1, useLong ? Int64Type : Int32Type, V1->getName(), CS.getInstruction());
    V2 = CastInst::CreateZExtOrBitCast(V2, useLong ? Int64Type : Int32Type, V2->getName(), CS.getInstruction());
  }

  V2 = BinaryOperator::Create(Instruction::Mul, V1, V2, "size",
                              CS.getInstruction());
  if (V2->getType() != (useLong ? Int64Type : Int32Type))
    V2 = CastInst::CreateZExtOrBitCast(V2, useLong ? Int64Type : Int32Type, V2->getName(), CS.getInstruction());

  BasicBlock::iterator BBI =
    TransformAllocationInstr(CS.getInstruction(), V2);
  Value *Ptr = BBI++;

  // We just turned the call of 'calloc' into the equivalent of malloc.  To
  // finish calloc, we need to zero out the memory.
  Constant *MemSet =  M->getOrInsertFunction((useLong ? "llvm.memset.i64" : "llvm.memset.i32"),
                                             Type::getVoidTy(M->getContext()),
                                             PointerType::getUnqual(Int8Type),
                                             Int8Type, (useLong ? Int64Type : Int32Type),
                                             Int32Type, NULL);

  if (Ptr->getType() != PointerType::getUnqual(Int8Type))
    Ptr = CastInst::CreatePointerCast(Ptr, PointerType::getUnqual(Int8Type), Ptr->getName(),
                       BBI);
  
  // We know that the memory returned by poolalloc is at least 4 byte aligned.
  Value* Opts[4] = {Ptr, ConstantInt::get(Int8Type, 0),
                    V2,  ConstantInt::get(Int32Type, 4)};
  CallInst::Create(MemSet, Opts, Opts + 4, "", BBI);
}


void FuncTransform::visitReallocCall(CallSite CS) {
  assert(CS.arg_end()-CS.arg_begin() == 2 && "realloc takes two arguments!");
  Instruction *I = CS.getInstruction();
  Value *PH = getPoolHandle(I);
  Value *OldPtr = CS.getArgument(0);
  Value *Size = CS.getArgument(1);

  // Don't poolallocate if we have no pool handle
  if (PH == 0 || isa<ConstantPointerNull>(PH)) return;

  if (Size->getType() != Type::getInt32Ty(CS.getInstruction()->getContext()))
    Size = CastInst::CreateIntegerCast(Size, Type::getInt32Ty(CS.getInstruction()->getContext()), false, Size->getName(), I);

  static Type *VoidPtrTy = PointerType::getUnqual(Type::getInt8Ty(CS.getInstruction()->getContext()));
  if (OldPtr->getType() != VoidPtrTy)
    OldPtr = CastInst::CreatePointerCast(OldPtr, VoidPtrTy, OldPtr->getName(), I);

  std::string Name = I->getName(); I->setName("");
  Value* Opts[3] = {PH, OldPtr, Size};
  Instruction *V = CallInst::Create(PAInfo.PoolRealloc, Opts, Opts + 3, Name, I);
  Instruction *Casted = V;
  if (V->getType() != I->getType())
    Casted = CastInst::CreatePointerCast(V, I->getType(), V->getName(), I);

  // Update def-use info
  I->replaceAllUsesWith(Casted);

  // If we are modifying the original function, update the DSGraph.
  if (!FI.Clone) {
    // V and Casted now point to whatever the original allocation did.
    G->getScalarMap().replaceScalar(I, V);
    if (V != Casted)
      G->getScalarMap()[Casted] = G->getScalarMap()[V];
  } else {             // Otherwise, update the NewToOldValueMap
    UpdateNewToOldValueMap(I, V, V != Casted ? Casted : 0);
  }

  // If this was an invoke, fix up the CFG.
  if (InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    BranchInst::Create (II->getNormalDest(), I);
    II->getUnwindDest()->removePredecessor(II->getParent(), true);
  }

  // Remove old allocation instruction.
  I->eraseFromParent();
}


/// visitMemAlignCall - Handle memalign and posix_memalign.
///
void FuncTransform::visitMemAlignCall(CallSite CS) {
  Instruction *I = CS.getInstruction();
  Value *ResultDest = 0;
  Value *Align = 0;
  Value *Size = 0;
  Value *PH;

  const Type* Int8Type = Type::getInt8Ty(CS.getInstruction()->getContext());
  const Type* Int32Type = Type::getInt32Ty(CS.getInstruction()->getContext());


  if (CS.getCalledFunction()->getName() == "memalign") {
    Align = CS.getArgument(0);
    Size = CS.getArgument(1);
    PH = getPoolHandle(I);
  } else {
    assert(CS.getCalledFunction()->getName() == "posix_memalign");
    ResultDest = CS.getArgument(0);
    Align   = CS.getArgument(1);
    Size    = CS.getArgument(2);

    assert(0 && "posix_memalign not implemented fully!");
    // We need to get the pool descriptor corresponding to *ResultDest.
    PH = getPoolHandle(I);

    // Return success always.
    const PointerType * PT = dyn_cast<PointerType>(I->getType());
    assert (PT && "memalign() does not return pointer type!\n");
    Value *RetVal = ConstantPointerNull::get(PT);
    I->replaceAllUsesWith(RetVal);

    static const Type *PtrPtr=PointerType::getUnqual(PointerType::getUnqual(Int8Type));
    if (ResultDest->getType() != PtrPtr)
      ResultDest = CastInst::CreatePointerCast(ResultDest, PtrPtr, ResultDest->getName(), I);
  }

  if (!Align->getType()->isIntegerTy(32))
    Align = CastInst::CreateIntegerCast(Align, Int32Type, false, Align->getName(), I);
  if (!Size->getType()->isIntegerTy(32))
    Size = CastInst::CreateIntegerCast(Size, Int32Type, false, Size->getName(), I);

  std::string Name = I->getName(); I->setName("");
  Value* Opts[3] = {PH, Align, Size};
  Instruction *V = CallInst::Create(PAInfo.PoolMemAlign, Opts, Opts + 3, Name, I);

  Instruction *Casted = V;
  if (V->getType() != I->getType())
    Casted = CastInst::CreatePointerCast(V, I->getType(), V->getName(), I);

  if (ResultDest)
    new StoreInst(V, ResultDest, I);
  else
    I->replaceAllUsesWith(Casted);

  // If we are modifying the original function, update the DSGraph.
  if (!FI.Clone) {
    // V and Casted now point to whatever the original allocation did.
    G->getScalarMap().replaceScalar(I, V);
    if (V != Casted)
      G->getScalarMap()[Casted] = G->getScalarMap()[V];
  } else {             // Otherwise, update the NewToOldValueMap
    UpdateNewToOldValueMap(I, V, V != Casted ? Casted : 0);
  }

  // If this was an invoke, fix up the CFG.
  if (InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    BranchInst::Create (II->getNormalDest(), I);
    II->getUnwindDest()->removePredecessor(II->getParent(), true);
  }

  // Remove old allocation instruction.
  I->eraseFromParent();
}

/// visitStrdupCall - Handle strdup().
///
void FuncTransform::visitStrdupCall(CallSite CS) {
  assert(CS.arg_end()-CS.arg_begin() == 1 && "strdup takes one argument!");
  Instruction *I = CS.getInstruction();
  DSNode *Node = getDSNodeHFor(I).getNode();
  assert (Node && "strdup has NULL DSNode!\n");
  Value *PH = getPoolHandle(I);

  const Type* Int8Type = Type::getInt8Ty(CS.getInstruction()->getContext());


#if 0
  assert (PH && "PH for strdup is null!\n");
#else
  if (!PH) {
    errs() << "strdup: NoPH\n";
    return;
  }
#endif
  Value *OldPtr = CS.getArgument(0);

  static Type *VoidPtrTy = PointerType::getUnqual(Int8Type);
  if (OldPtr->getType() != VoidPtrTy)
    OldPtr = CastInst::CreatePointerCast(OldPtr, VoidPtrTy, OldPtr->getName(), I);

  std::string Name = I->getName(); I->setName("");
  Value* Opts[3] = {PH, OldPtr, 0};
  Instruction *V = CallInst::Create(PAInfo.PoolStrdup, Opts, Opts + 2, Name, I);
  Instruction *Casted = V;
  if (V->getType() != I->getType())
    Casted = CastInst::CreatePointerCast(V, I->getType(), V->getName(), I);

  // Update def-use info
  I->replaceAllUsesWith(Casted);

  // If we are modifying the original function, update the DSGraph.
  if (!FI.Clone) {
    // V and Casted now point to whatever the original allocation did.
    G->getScalarMap().replaceScalar(I, V);
    if (V != Casted)
      G->getScalarMap()[Casted] = G->getScalarMap()[V];
  } else {             // Otherwise, update the NewToOldValueMap
    UpdateNewToOldValueMap(I, V, V != Casted ? Casted : 0);
  }

  // If this was an invoke, fix up the CFG.
  if (InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    BranchInst::Create (II->getNormalDest(), I);
    II->getUnwindDest()->removePredecessor(II->getParent(), true);
  }

  // Remove old allocation instruction.
  I->eraseFromParent();
}

//
// Method: visitRuntimeCheck()
//
// Description:
//  Visit a call to a run-time check (or related function) and insert pool
//  arguments where needed.
//
void
FuncTransform::visitRuntimeCheck (CallSite CS) {
  // A run-time check should have at least one argument for a pool
  assert ((CS.arg_size() > 1) && "Runtime check takes more than one argument!");

  //
  // Get the pool handle for the pointer argument.
  //
  Value *PH = getPoolHandle(CS.getArgument(1)->stripPointerCasts());
  //assert (PH && "Pool Handle for run-time checks is null!\n");

  //
  // Insert the pool handle into the run-time check.
  //
  if (PH) {
    const Type * Int8Type  = Type::getInt8Ty(CS.getInstruction()->getContext());
    const Type * VoidPtrTy = PointerType::getUnqual(Int8Type);
    PH = castTo (PH, VoidPtrTy, PH->getName(), CS.getInstruction());
    CS.setArgument (0, PH);

    //
    // Record that we've used the pool here.
    //
    AddPoolUse (*(CS.getInstruction()), PH, PoolUses);
  }
}

//
// Method: visitCallSite()
//
// Description:
//  This method transforms a call site.  A call site may either be a call
//  instruction or an invoke instruction.
//
// Inputs:
//  CS - The call site representing the instruction that should be transformed.
//
void FuncTransform::visitCallSite(CallSite& CS) {
  const Function *CF = CS.getCalledFunction();
  Instruction *TheCall = CS.getInstruction();
  bool thread_creation_point = false;

  //
  // Get the value that is called at this call site.  Strip away any pointer
  // casts that do not change the representation of the data (i.e., are
  // lossless casts).
  //
  Value * CalledValue = CS.getCalledValue()->stripPointerCasts();

  //
  // The CallSite::getCalledFunction() method is not guaranteed to strip off
  // pointer casts.  If no called function was found, manually strip pointer
  // casts off of the called value and see if we get a function.  If so, this
  // is a direct call, and we want to update CF accordingly.
  //
  if (!CF) CF = dyn_cast<Function>(CalledValue);

  //
  // Do not change any inline assembly code.
  //
  if (isa<InlineAsm>(TheCall->getOperand(0))) {
    errs() << "INLINE ASM: ignoring.  Hoping that's safe.\n";
    return;
  }

  //
  // Ignore calls to NULL pointers or undefined values.
  //
  if ((isa<ConstantPointerNull>(CalledValue)) ||
      (isa<UndefValue>(CalledValue))) {
    errs() << "WARNING: Ignoring call using NULL/Undef function pointer.\n";
    return;
  }

  // If this function is one of the memory manipulating functions built into
  // libc, emulate it with pool calls as appropriate.
  if (CF && CF->isDeclaration()) {
    if (CF->getName() == "free") {
      visitFreeCall(CS);
      return;
    } else if (CF->getName() == "malloc") {
      visitMallocCall(CS);
      return;
    } else if (CF->getName() == "calloc") {
      visitCallocCall(CS);
      return;
    } else if (CF->getName() == "realloc") {
      visitReallocCall(CS);
      return;
    } else if (CF->getName() == "memalign" ||
               CF->getName() == "posix_memalign") {
      visitMemAlignCall(CS);
      return;
    } else if (CF->getName() == "strdup") {
      visitStrdupCall(CS);
      return;
    } else if (CF->getName() == "valloc") {
      errs() << "VALLOC USED BUT NOT HANDLED!\n";
      abort();
    } else if ((CF->getName() == "sc.lscheck") ||
               (CF->getName() == "sc.lscheckui") ||
               (CF->getName() == "sc.lscheckalign") ||
               (CF->getName() == "sc.lscheckalignui") ||
               (CF->getName() == "sc.boundscheck") ||
               (CF->getName() == "sc.boundscheckui") ||
               (CF->getName() == "sc.pool_register_stack") ||
               (CF->getName() == "sc.pool_unregister_stack") ||
               (CF->getName() == "sc.pool_register_global") ||
               (CF->getName() == "sc.pool_unregister_global") ||
               (CF->getName() == "sc.pool_register") ||
               (CF->getName() == "sc.pool_unregister") ||
               (CF->getName() == "sc.get_actual_val")) {
      visitRuntimeCheck (CS);
    } else if (CF->getName() == "pthread_create") {
      thread_creation_point = true;

      //Get DSNode representing the DSNode of the function pointer Value of the pthread_create call
      DSNode* thread_callee_node = G->getNodeForValue(CS.getArgument(2)).getNode();
      if(!thread_callee_node)
      {
    	  assert(0 && "apparently you need this code");
    	  FuncInfo *CFI = PAInfo.getFuncInfo(*CF);
    	  thread_callee_node = G->getNodeForValue(CFI->MapValueToOriginal(CS.getArgument(2))).getNode();
      }

      //Fill in CF with the name of one of the functions in thread_callee_node
      CF = const_cast<Function*>(dyn_cast<Function>(*thread_callee_node->globals_begin()));
    }
  }

  //
  // We need to figure out which local pool descriptors correspond to the pool
  // descriptor arguments passed into the function call.  Calculate a mapping
  // from callee DSNodes to caller DSNodes.  We construct a partial isomophism
  // between the graphs to figure out which pool descriptors need to be passed
  // in.  The roots of this mapping is found from arguments and return values.
  //
  DataStructures& Graphs = PAInfo.getGraphs();
  DSGraph::NodeMapTy NodeMapping;
  Instruction *NewCall;
  Value *NewCallee;
  std::vector<const DSNode*> ArgNodes;
  DSGraph *CalleeGraph;  // The callee graph

  // For indirect callees, find any callee since all DS graphs have been
  // merged.
  if (CF) {   // Direct calls are nice and simple.
    DEBUG(errs() << "  Handling direct call: " << *TheCall << "\n");

    //
    // Do not try to add pool handles to the function if it:
    //  a) Already calls a cloned function; or
    //  b) Calls a function which was never cloned.
    //
    // For such a call, just replace any arguments that take original functions
    // with their cloned function poiner values.
    //
    FuncInfo *CFI = PAInfo.getFuncInfo(*CF);
    if (CFI == 0 || CFI->Clone == 0) {   // Nothing to transform...
      visitInstruction(*TheCall);
      return;
    }

    //
    // Oh, dear.  We must add pool descriptors to this direct call.
    //
    NewCallee = CFI->Clone;
    ArgNodes = CFI->ArgNodes;
    
    assert ((Graphs.hasDSGraph (*CF)) && "Function has no ECGraph!\n");
    CalleeGraph = Graphs.getDSGraph(*CF);
  } else {
    DEBUG(errs() << "  Handling indirect call: " << *TheCall << "\n");
    
    // Here we fill in CF with one of the possible called functions.  Because we
    // merged together all of the arguments to all of the functions in the
    // equivalence set, it doesn't really matter which one we pick.
    // (If the function was cloned, we have to map the cloned call instruction
    // in CS back to the original call instruction.)
    Instruction *OrigInst =
      cast<Instruction>(getOldValueIfAvailable(CS.getInstruction()));

    //
    // Attempt to get one of the function targets of this indirect call site by
    // looking at the call graph constructed by the points-to analysis.  Be
    // sure to use the original call site from the original function; the
    // points-to analysis has no information on the clones we've created.
    //
    // Also, look for the target that has the greatest number of arguments that
    // have associated DSNodes.  This ensures that we pass the maximum number
    // of pools possible and prevents us from eliding a pool because we're
    // examining a target that doesn't need it.
    //
    const DSCallGraph & callGraph = Graphs.getCallGraph();
    unsigned maxArgsWithNodes = 0;
    DSCallGraph::callee_iterator I = callGraph.callee_begin(OrigInst);
    for (; I != callGraph.callee_end(OrigInst); ++I) {
      //
      // Get the information for this function.  Since this is coming from DSA,
      // it should be an original function.
      //
      FuncInfo *CFI = PAInfo.getFuncInfo(**I);
      assert(CFI && "Func Info not found");
      //
      // If this target takes more DSNodes than the last one we found, then
      // make *this* target our canonical target.
      //
      if (CFI->ArgNodes.size() >= maxArgsWithNodes) {
        maxArgsWithNodes = CFI->ArgNodes.size();
        CF = *I;
      }
    }

    //
    // If we didn't find the callee in the constructed call graph, try
    // checking in the DSNode itself.
    // This isn't ideal as it means that this call site didn't have inlining
    // happen.
    //
    if (!CF) {
      DSGraph* dg = Graphs.getDSGraph(*OrigInst->getParent()->getParent());
      DSNode* d = dg->getNodeForValue(OrigInst->getOperand(0)).getNode();
      assert (d && "No DSNode!\n");
      std::vector<const Function*> g;
      d->addFullFunctionList(g);
      
      if(!(d->isIncompleteNode()) && !(d->isExternalNode())) {
      //if(!(d->isIncompleteNode()) && !(d->isExternalNode()) && !(d->isCollapsedNode())) {
      
      //
      // Perform some consistency checks on the callees.
      //
      verifyCallees (g);

      //
      // If we found any callees, grab the first one with a DSGraph and use it.
      // Since we're using EQBU/EQTD, all potential targets should have the
      // same DSGraph, so it doesn't matter which one we use as long as we use
      // a function that *has* a DSGraph.
      //
        for (unsigned index = 0; index < g.size(); ++index) {
          if (Graphs.hasDSGraph (*(g[index]))) {
            CF = g[index];
            break;
          }
        }
      }
    }

    //
    // If we still haven't been able to find a target function of the call site
    // to transform, do nothing.
    //
    // One may be tempted to think that we should always have at least one
    // target, but this is not true.  There are perfectly acceptable (but
    // strange) programs for which no function targets exist.  Function
    // pointers loaded from undef values, for example, will have no targets.
    //
    if (!CF) return;

    //
    // It's possible that this program has indirect call targets that are
    // not defined in this module.  Do not transformation for such functions.
    //
    if (!(Graphs.hasDSGraph(*CF))) return;

    //
    // Get the common graph for the set of functions this call may invoke.
    //
    assert ((Graphs.hasDSGraph(*CF)) && "Function has no DSGraph!\n");
    CalleeGraph = Graphs.getDSGraph(*CF);

#ifndef NDEBUG
    // Verify that all potential callees at call site have the same DS graph.
    /*DSCallGraph::callee_iterator E = PAInfo.getCallGraph().callee_end(OrigInst);
    for (; I != E; ++I) {
      const Function * F = *I;
      assert (F);
      if (!(F)->isDeclaration())
        assert(CalleeGraph == Graphs.getDSGraph(**I) &&
               "Callees at call site do not have a common graph!");
    }*/
#endif    

    // Find the DS nodes for the arguments that need to be added, if any.
    FuncInfo *CFI = PAInfo.getFuncInfo(*CF);
    assert(CFI && "No function info for callee at indirect call?");
    ArgNodes = CFI->ArgNodes;

    if (ArgNodes.empty())
      return;           // No arguments to add?  Transformation is a noop!

    // Cast the function pointer to an appropriate type!
    std::vector<const Type*> ArgTys(ArgNodes.size(),
                                    PoolAllocate::PoolDescPtrTy);
    for (CallSite::arg_iterator I = CS.arg_begin(), E = CS.arg_end();
         I != E; ++I)
      ArgTys.push_back((*I)->getType());
    
    FunctionType *FTy = FunctionType::get(TheCall->getType(), ArgTys, false);
    PointerType *PFTy = PointerType::getUnqual(FTy);
    
    // If there are any pool arguments cast the func ptr to the right type.
    NewCallee = CastInst::CreatePointerCast(CS.getCalledValue(), PFTy, "tmp", TheCall);
  }

  //
  // FIXME: Why do we disable strict checking when calling the
  //        DSGraph::computeNodeMapping() method?
  //
  Function::const_arg_iterator FAI = CF->arg_begin(), E = CF->arg_end();
  CallSite::arg_iterator AI = CS.arg_begin() + (thread_creation_point ? 3 : 0);
  CallSite::arg_iterator AE = CS.arg_end();
  for ( ; FAI != E && AI != AE; ++FAI, ++AI)
    if (!isa<Constant>(*AI)) {
      DSGraph::computeNodeMapping(CalleeGraph->getNodeForValue(FAI),
                                  getDSNodeHFor(*AI), NodeMapping, false);
    }

  //assert(AI == AE && "Varargs calls not handled yet!");

  // Map the return value as well...
  if (isa<PointerType>(TheCall->getType()))
    DSGraph::computeNodeMapping(CalleeGraph->getReturnNodeFor(*CF),
                                getDSNodeHFor(TheCall), NodeMapping, false);

  // This code seems redundant (and crashes occasionally)
  // There is no reason to map globals here, since they are not passed as
  // arguments

//   // Map the nodes that are pointed to by globals.
//    DSScalarMap &CalleeSM = CalleeGraph->getScalarMap();
//    for (DSScalarMap::global_iterator GI = G.getScalarMap().global_begin(), 
//           E = G.getScalarMap().global_end(); GI != E; ++GI)
//      if (CalleeSM.count(*GI))
//        DSGraph::computeNodeMapping(CalleeGraph->getNodeForValue(*GI),
//                                    getDSNodeHFor(*GI),
//                                    NodeMapping, false);

  //
  // Okay, now that we have established our mapping, we can figure out which
  // pool descriptors to pass in...
  //
  // Note:
  // There used to be code here that would create a new pool before the
  // function call and destroy it after the function call.  This could would
  // get triggered if bounds checking was disbled or the DSNode for the
  // argument was an array value.
  //
  // I believe that code was incorrect; an argument may have a NULL pool handle
  // (i.e., no pool handle) because the pool allocation heuristic used simply
  // decided not to assign that value a pool.  The argument may alias data
  // that should not be freed after the function call is complete, so calling
  // pooldestroy() after the call would free data, causing dangling pointer
  // dereference errors.
  //
  std::vector<Value*> Args;
  for (unsigned i = 0, e = ArgNodes.size(); i != e; ++i) {
    Value *ArgVal = Constant::getNullValue(PoolAllocate::PoolDescPtrTy);
    if (NodeMapping.count(ArgNodes[i])) {
      if (DSNode *LocalNode = NodeMapping[ArgNodes[i]].getNode())
        if (FI.PoolDescriptors.count(LocalNode))
          ArgVal = FI.PoolDescriptors.find(LocalNode)->second;
    }
    Args.push_back(ArgVal);
  }

  // Add the rest of the arguments unless we're a thread creation point, in which case we only need the pools
  if(!thread_creation_point)
	  Args.insert(Args.end(), CS.arg_begin(), CS.arg_end());
    
  //
  // There are circumstances where a function is casted to another type and
  // then called (que horible).  We need to perform a similar cast if the
  // type doesn't match the number of arguments.
  //
  if (Function * NewFunction = dyn_cast<Function>(NewCallee)) {
    const FunctionType * NewCalleeType = NewFunction->getFunctionType();
    if (NewCalleeType->getNumParams() != Args.size()) {
      std::vector<const Type *> Types;
      Type * FuncTy = FunctionType::get (NewCalleeType->getReturnType(),
                                         Types,
                                         true);
      FuncTy = PointerType::getUnqual (FuncTy);
      NewCallee = new BitCastInst (NewCallee, FuncTy, "", TheCall);
    }
  }

  std::string Name = TheCall->getName();    TheCall->setName("");

  if(thread_creation_point) {
	Module *M = CS.getInstruction()->getParent()->getParent()->getParent();
	Value* pthread_replacement = M->getFunction("poolalloc_pthread_create");
	std::vector<Value*> thread_args;

	//Push back original thread arguments through the callee
	thread_args.push_back(CS.getArgument(0));
	thread_args.push_back(CS.getArgument(1));
	thread_args.push_back(CS.getArgument(2));

	//Push back the integer argument saying how many uses there are
	thread_args.push_back(Constant::getIntegerValue(llvm::Type::getInt32Ty(M->getContext()),APInt(32,Args.size())));
	thread_args.insert(thread_args.end(),Args.begin(),Args.end());
	thread_args.push_back(CS.getArgument(3));

	//Make the thread creation call
	NewCall = CallInst::Create(pthread_replacement,
							   thread_args.begin(),thread_args.end(),
							   Name,TheCall);
  }
  else if (InvokeInst *II = dyn_cast<InvokeInst>(TheCall)) {
    NewCall = InvokeInst::Create (NewCallee, II->getNormalDest(),
                                  II->getUnwindDest(),
                                  Args.begin(), Args.end(), Name, TheCall);
  } else {
    NewCall = CallInst::Create (NewCallee, Args.begin(), Args.end(), Name,
                                TheCall);
  }

  // Add all of the uses of the pool descriptor
  for (unsigned i = 0, e = ArgNodes.size(); i != e; ++i)
    AddPoolUse(*NewCall, Args[i], PoolUses);

  TheCall->replaceAllUsesWith(NewCall);
  DEBUG(errs() << "  Result Call: " << *NewCall << "\n");

  if (!TheCall->getType()->isVoidTy()) {
    // If we are modifying the original function, update the DSGraph... 
    DSGraph::ScalarMapTy &SM = G->getScalarMap();
    DSGraph::ScalarMapTy::iterator CII = SM.find(TheCall);
    if (CII != SM.end()) {
      SM[NewCall] = CII->second;
      SM.erase(CII);                     // Destroy the CallInst
    } else if (!FI.NewToOldValueMap.empty()) {
      // Otherwise, if this is a clone, update the NewToOldValueMap with the new
      // CI return value.
      UpdateNewToOldValueMap(TheCall, NewCall);
    }
  } else if (!FI.NewToOldValueMap.empty()) {
    UpdateNewToOldValueMap(TheCall, NewCall);
  }

  //
  // Copy over the calling convention and attributes of the original call
  // instruction to the new call instruction.
  //
  CallSite(NewCall).setCallingConv(CallSite(TheCall).getCallingConv());

  TheCall->eraseFromParent();
  visitInstruction(*NewCall);
}

//
// visitInstruction - For all instructions in the transformed function bodies,
// replace any references to the original calls with references to the
// transformed calls.  Many instructions can "take the address of" a function,
// and we must make sure to catch each of these uses, and transform it into a
// reference to the new, transformed, function.
// FIXME: Don't rename uses of function names that escape
// FIXME: Special-case when external user is pthread_create (or similar)?
//
void FuncTransform::visitInstruction(Instruction &I) {
  for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i)
    if (Function *clonedFunc = retCloneIfFunc(I.getOperand(i))) {
      Constant *CF = clonedFunc;
      I.setOperand(i, ConstantExpr::getPointerCast(CF, I.getOperand(i)->getType()));
    }
}
