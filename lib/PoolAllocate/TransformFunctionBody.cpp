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
    DSGraph &G;      // The Bottom-up DS Graph
    FuncInfo &FI;
    CallTargetFinder* CTF;

    // PoolUses - For each pool (identified by the pool descriptor) keep track
    // of which blocks require the memory in the pool to not be freed.  This
    // does not include poolfree's.  Note that this is only tracked for pools
    // which this is the home of, ie, they are Alloca instructions.
    std::multimap<AllocaInst*, Instruction*> &PoolUses;

    // PoolDestroys - For each pool, keep track of the actual poolfree calls
    // inserted into the code.  This is seperated out from PoolUses.
    std::multimap<AllocaInst*, CallInst*> &PoolFrees;

    FuncTransform(PoolAllocate &P, DSGraph &g, FuncInfo &fi,
                  std::multimap<AllocaInst*, Instruction*> &poolUses,
                  std::multimap<AllocaInst*, CallInst*> &poolFrees,
		  CallTargetFinder* ctf)
      : PAInfo(P), G(g), FI(fi), CTF(ctf),
        PoolUses(poolUses), PoolFrees(poolFrees) {
    }

    template <typename InstType, typename SetType>
    void AddPoolUse(InstType &I, Value *PoolHandle, SetType &Set) {
      if (AllocaInst *AI = dyn_cast<AllocaInst>(PoolHandle))
        Set.insert(std::make_pair(AI, &I));
    }

    void visitInstruction(Instruction &I);
    void visitMallocInst(MallocInst &MI);
    void visitAllocaInst(AllocaInst &MI);
    void visitCallocCall(CallSite CS);
    void visitReallocCall(CallSite CS);
    void visitMemAlignCall(CallSite CS);
    void visitFreeInst(FreeInst &FI);
    void visitCallSite(CallSite CS);
    void visitCallInst(CallInst &CI) { visitCallSite(&CI); }
    void visitInvokeInst(InvokeInst &II) { visitCallSite(&II); }
    void visitLoadInst(LoadInst &I);
    void visitStoreInst (StoreInst &I);

  private:
    Instruction *TransformAllocationInstr(Instruction *I, Value *Size);
    Instruction *InsertPoolFreeInstr(Value *V, Instruction *Where);

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
      return G.getScalarMap()[getOldValueIfAvailable(V)];
    }

    Value *getPoolHandle(Value *V) {
      DSNode *Node = getDSNodeHFor(V).getNode();
      // Get the pool handle for this DSNode...
      std::map<const DSNode*, Value*>::iterator I =
        FI.PoolDescriptors.find(Node);
      return I != FI.PoolDescriptors.end() ? I->second : 0;
    }
    
    Function* retCloneIfFunc(Value *V);
  };
}

void PoolAllocate::TransformBody(DSGraph &g, PA::FuncInfo &fi,
                              std::multimap<AllocaInst*,Instruction*> &poolUses,
                              std::multimap<AllocaInst*, CallInst*> &poolFrees,
                                 Function &F) {
  FuncTransform(*this, g, fi, poolUses, poolFrees, CTF).visit(F);
}


// Returns the clone if  V is a static function (not a pointer) and belongs 
// to an equivalence class i.e. is pool allocated
Function* FuncTransform::retCloneIfFunc(Value *V) {
  if (Function *F = dyn_cast<Function>(V))
    if (FuncInfo *FI = PAInfo.getFuncInfo(*F))
      return FI->Clone;

  return 0;
}

void FuncTransform::visitLoadInst(LoadInst &LI) {
  if (Value *PH = getPoolHandle(LI.getOperand(0)))
    AddPoolUse(LI, PH, PoolUses);
  visitInstruction(LI);
}

void FuncTransform::visitStoreInst(StoreInst &SI) {
  if (Value *PH = getPoolHandle(SI.getOperand(1)))
    AddPoolUse(SI, PH, PoolUses);
  visitInstruction(SI);
}

Instruction *FuncTransform::TransformAllocationInstr(Instruction *I,
                                                     Value *Size) {
  std::string Name = I->getName(); I->setName("");

  if (Size->getType() != Type::Int32Ty)
    Size = CastInst::createZExtOrBitCast(Size, Type::Int32Ty, Size->getName(), I);

  // Insert a call to poolalloc
  Value *PH = getPoolHandle(I);
  
  Instruction *V = new CallInst(PAInfo.PoolAlloc, PH, Size, Name, I);

  AddPoolUse(*V, PH, PoolUses);

  // Cast to the appropriate type if necessary
  Instruction *Casted = V;
  if (V->getType() != I->getType())
    Casted = CastInst::createPointerCast(V, I->getType(), V->getName(), I);
    
  // Update def-use info
  I->replaceAllUsesWith(Casted);

  // If we are modifying the original function, update the DSGraph.
  if (!FI.Clone) {
    // V and Casted now point to whatever the original allocation did.
    G.getScalarMap().replaceScalar(I, V);
    if (V != Casted)
      G.getScalarMap()[Casted] = G.getScalarMap()[V];
  } else {             // Otherwise, update the NewToOldValueMap
    UpdateNewToOldValueMap(I, V, V != Casted ? Casted : 0);
  }

  // If this was an invoke, fix up the CFG.
  if (InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    new BranchInst(II->getNormalDest(), I);
    II->getUnwindDest()->removePredecessor(II->getParent(), true);
  }

  // Remove old allocation instruction.
  I->eraseFromParent();
  return Casted;
}


void FuncTransform::visitMallocInst(MallocInst &MI) {
  // Get the pool handle for the node that this contributes to...
  Value *PH = getPoolHandle(&MI);
  
  if (PH == 0 || isa<ConstantPointerNull>(PH)) return;

  TargetData &TD = PAInfo.getAnalysis<TargetData>();
  Value *AllocSize =
    ConstantInt::get(Type::Int32Ty, TD.getTypeSize(MI.getAllocatedType()));

  if (MI.isArrayAllocation())
    AllocSize = BinaryOperator::create(Instruction::Mul, AllocSize,
                                       MI.getOperand(0), "sizetmp", &MI);
#ifdef SAFECODE
  const MallocInst *originalMalloc = &MI;
  if (FI.NewToOldValueMap.count(&MI)) {
    originalMalloc = cast<MallocInst>(FI.NewToOldValueMap[&MI]);
  }
  //Dinakar to test stack safety & array safety 
  if (PAInfo.CUAPass->ArrayMallocs.find(originalMalloc) ==
      PAInfo.CUAPass->ArrayMallocs.end()) {
    TransformAllocationInstr(&MI, AllocSize);
  } else {
    AllocaInst *AI = new AllocaInst(MI.getType()->getElementType(), MI.getArraySize(), MI.getName(), MI.getNext());
    MI.replaceAllUsesWith(AI);
    MI.getParent()->getInstList().erase(&MI);
    Value *Casted = AI;
    Instruction *aiNext = AI->getNext();
    if (AI->getType() != PointerType::get(Type::Int8Ty))
      Casted = CastInst::createPointerCast(AI, PointerType::get(Type::Int8Ty),
			      AI->getName()+".casted",aiNext);
    
    Instruction *V = new CallInst(PAInfo.PoolRegister,
				  make_vector(PH, AllocSize, Casted, 0), "", aiNext);
    AddPoolUse(*V, PH, PoolUses);
  }
#else
  TransformAllocationInstr(&MI, AllocSize);  
#endif
}

void FuncTransform::visitAllocaInst(AllocaInst &MI) {
#ifdef BOUNDS_CHECK
  // Get the pool handle for the node that this contributes to...
  DSNode *Node = getDSNodeHFor(&MI).getNode();
  if (Node->isArray()) {
    Value *PH = getPoolHandle(&MI);
    if (PH == 0 || isa<ConstantPointerNull>(PH)) return;
    TargetData &TD = PAInfo.getAnalysis<TargetData>();
    Value *AllocSize =
      ConstantInt::get(Type::Int32Ty, TD.getTypeSize(MI.getAllocatedType()));
    
    if (MI.isArrayAllocation())
      AllocSize = BinaryOperator::create(Instruction::Mul, AllocSize,
					 MI.getOperand(0), "sizetmp", &MI);
    
    //  TransformAllocationInstr(&MI, AllocSize);
    Instruction *Casted = CastInst::createPointerCast(&MI, PointerType::get(Type::Int8Ty),
				       MI.getName()+".casted", MI.getNext());
    Instruction *V = new CallInst(PAInfo.PoolRegister,
				  make_vector(PH, Casted, AllocSize, 0), "", Casted->getNext());
    AddPoolUse(*V, PH, PoolUses);
  }
#endif  
}


Instruction *FuncTransform::InsertPoolFreeInstr(Value *Arg, Instruction *Where){
  Value *PH = getPoolHandle(Arg);  // Get the pool handle for this DSNode...
  if (PH == 0 || isa<ConstantPointerNull>(PH)) return 0;

  // Insert a cast and a call to poolfree...
  Value *Casted = Arg;
  if (Arg->getType() != PointerType::get(Type::Int8Ty)) {
    Casted = CastInst::createPointerCast(Arg, PointerType::get(Type::Int8Ty),
				 Arg->getName()+".casted", Where);
    G.getScalarMap()[Casted] = G.getScalarMap()[Arg];
  }

  CallInst *FreeI = new CallInst(PAInfo.PoolFree, PH, Casted, "", Where);
  AddPoolUse(*FreeI, PH, PoolFrees);
  return FreeI;
}


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


void FuncTransform::visitCallocCall(CallSite CS) {
  TargetData& TD = PAInfo.getAnalysis<TargetData>();
  bool useLong = TD.getTypeSize(PointerType::get(Type::Int8Ty)) != 4;
  
  Module *M = CS.getInstruction()->getParent()->getParent()->getParent();
  assert(CS.arg_end()-CS.arg_begin() == 2 && "calloc takes two arguments!");
  Value *V1 = CS.getArgument(0);
  Value *V2 = CS.getArgument(1);
  if (V1->getType() != V2->getType()) {
    V1 = CastInst::createZExtOrBitCast(V1, useLong ? Type::Int64Ty : Type::Int32Ty, V1->getName(), CS.getInstruction());
    V2 = CastInst::createZExtOrBitCast(V2, useLong ? Type::Int64Ty : Type::Int32Ty, V2->getName(), CS.getInstruction());
  }

  V2 = BinaryOperator::create(Instruction::Mul, V1, V2, "size",
                              CS.getInstruction());
  if (V2->getType() != (useLong ? Type::Int64Ty : Type::Int32Ty))
    V2 = CastInst::createZExtOrBitCast(V2, useLong ? Type::Int64Ty : Type::Int32Ty, V2->getName(), CS.getInstruction());

  BasicBlock::iterator BBI =
    TransformAllocationInstr(CS.getInstruction(), V2);
  Value *Ptr = BBI++;

  // We just turned the call of 'calloc' into the equivalent of malloc.  To
  // finish calloc, we need to zero out the memory.
  Constant *MemSet =  M->getOrInsertFunction((useLong ? "llvm.memset.i64" : "llvm.memset.i32"),
                                             Type::VoidTy,
                                             PointerType::get(Type::Int8Ty),
                                             Type::Int8Ty, (useLong ? Type::Int64Ty : Type::Int32Ty),
                                             Type::Int32Ty, NULL);

  if (Ptr->getType() != PointerType::get(Type::Int8Ty))
    Ptr = CastInst::createPointerCast(Ptr, PointerType::get(Type::Int8Ty), Ptr->getName(),
                       BBI);
  
  // We know that the memory returned by poolalloc is at least 4 byte aligned.
  Value* Opts[4] = {Ptr, ConstantInt::get(Type::Int8Ty, 0),
                    V2,  ConstantInt::get(Type::Int32Ty, 4)};
  new CallInst(MemSet, Opts, 4, "", BBI);
}


void FuncTransform::visitReallocCall(CallSite CS) {
  assert(CS.arg_end()-CS.arg_begin() == 2 && "realloc takes two arguments!");
  Instruction *I = CS.getInstruction();
  Value *PH = getPoolHandle(I);
  Value *OldPtr = CS.getArgument(0);
  Value *Size = CS.getArgument(1);

  if (Size->getType() != Type::Int32Ty)
    Size = CastInst::createZExtOrBitCast(Size, Type::Int32Ty, Size->getName(), I);

  static Type *VoidPtrTy = PointerType::get(Type::Int8Ty);
  if (OldPtr->getType() != VoidPtrTy)
    OldPtr = CastInst::createPointerCast(OldPtr, VoidPtrTy, OldPtr->getName(), I);

  std::string Name = I->getName(); I->setName("");
  Value* Opts[3] = {PH, OldPtr, Size};
  Instruction *V = new CallInst(PAInfo.PoolRealloc, Opts, 3, Name, I);
  Instruction *Casted = V;
  if (V->getType() != I->getType())
    Casted = CastInst::createPointerCast(V, I->getType(), V->getName(), I);

  // Update def-use info
  I->replaceAllUsesWith(Casted);

  // If we are modifying the original function, update the DSGraph.
  if (!FI.Clone) {
    // V and Casted now point to whatever the original allocation did.
    G.getScalarMap().replaceScalar(I, V);
    if (V != Casted)
      G.getScalarMap()[Casted] = G.getScalarMap()[V];
  } else {             // Otherwise, update the NewToOldValueMap
    UpdateNewToOldValueMap(I, V, V != Casted ? Casted : 0);
  }

  // If this was an invoke, fix up the CFG.
  if (InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    new BranchInst(II->getNormalDest(), I);
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
    Value *RetVal = Constant::getNullValue(I->getType());
    I->replaceAllUsesWith(RetVal);

    static const Type *PtrPtr=PointerType::get(PointerType::get(Type::Int8Ty));
    if (ResultDest->getType() != PtrPtr)
      ResultDest = CastInst::createPointerCast(ResultDest, PtrPtr, ResultDest->getName(), I);
  }

  if (Align->getType() != Type::Int32Ty)
    Align = CastInst::createZExtOrBitCast(Align, Type::Int32Ty, Align->getName(), I);
  if (Size->getType() != Type::Int32Ty)
    Size = CastInst::createZExtOrBitCast(Size, Type::Int32Ty, Size->getName(), I);

  std::string Name = I->getName(); I->setName("");
  Value* Opts[3] = {PH, Align, Size};
  Instruction *V = new CallInst(PAInfo.PoolMemAlign, Opts, 3, Name, I);

  Instruction *Casted = V;
  if (V->getType() != I->getType())
    Casted = CastInst::createPointerCast(V, I->getType(), V->getName(), I);

  if (ResultDest)
    new StoreInst(V, ResultDest, I);
  else
    I->replaceAllUsesWith(Casted);

  // If we are modifying the original function, update the DSGraph.
  if (!FI.Clone) {
    // V and Casted now point to whatever the original allocation did.
    G.getScalarMap().replaceScalar(I, V);
    if (V != Casted)
      G.getScalarMap()[Casted] = G.getScalarMap()[V];
  } else {             // Otherwise, update the NewToOldValueMap
    UpdateNewToOldValueMap(I, V, V != Casted ? Casted : 0);
  }

  // If this was an invoke, fix up the CFG.
  if (InvokeInst *II = dyn_cast<InvokeInst>(I)) {
    new BranchInst(II->getNormalDest(), I);
    II->getUnwindDest()->removePredecessor(II->getParent(), true);
  }

  // Remove old allocation instruction.
  I->eraseFromParent();
}



void FuncTransform::visitCallSite(CallSite CS) {
  Function *CF = CS.getCalledFunction();
  Instruction *TheCall = CS.getInstruction();

  // If the called function is casted from one function type to another, peer
  // into the cast instruction and pull out the actual function being called.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CS.getCalledValue()))
    if (CE->getOpcode() == Instruction::BitCast &&
        isa<Function>(CE->getOperand(0)))
      CF = cast<Function>(CE->getOperand(0));

  if (isa<InlineAsm>(TheCall->getOperand(0))) {
    std::cerr << "INLINE ASM: ignoring.  Hoping that's safe.\n";
    return;
  }

  // If this function is one of the memory manipulating functions built into
  // libc, emulate it with pool calls as appropriate.
  if (CF && CF->isDeclaration())
    if (CF->getName() == "calloc") {
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
      assert(0 && "strdup should have been linked into the program!");
    } else if (CF->getName() == "valloc") {
      std::cerr << "VALLOC USED BUT NOT HANDLED!\n";
      abort();
    }

  // We need to figure out which local pool descriptors correspond to the pool
  // descriptor arguments passed into the function call.  Calculate a mapping
  // from callee DSNodes to caller DSNodes.  We construct a partial isomophism
  // between the graphs to figure out which pool descriptors need to be passed
  // in.  The roots of this mapping is found from arguments and return values.
  //
  EquivClassGraphs& ECGraphs = PAInfo.getECGraphs();
  DSGraph::NodeMapTy NodeMapping;
  Instruction *NewCall;
  Value *NewCallee;
  std::vector<const DSNode*> ArgNodes;
  DSGraph *CalleeGraph;  // The callee graph

  // For indirect callees find any callee since all DS graphs have been merged.
  if (CF) {   // Direct calls are nice and simple.
    DEBUG(std::cerr << "  Handling direct call: " << *TheCall);
    FuncInfo *CFI = PAInfo.getFuncInfo(*CF);
    if (CFI == 0 || CFI->Clone == 0) {   // Nothing to transform...
      visitInstruction(*TheCall);
      return;
    }
    NewCallee = CFI->Clone;
    ArgNodes = CFI->ArgNodes;
    
    CalleeGraph = &ECGraphs.getDSGraph(*CF);
  } else {
    DEBUG(std::cerr << "  Handling indirect call: " << *TheCall);
    
    // Here we fill in CF with one of the possible called functions.  Because we
    // merged together all of the arguments to all of the functions in the
    // equivalence set, it doesn't really matter which one we pick.
    // (If the function was cloned, we have to map the cloned call instruction
    // in CS back to the original call instruction.)
    Instruction *OrigInst =
      cast<Instruction>(getOldValueIfAvailable(CS.getInstruction()));
    CF = isa<CallInst>(OrigInst)?
      ECGraphs.getSomeCalleeForCallSite(cast<CallInst>(OrigInst)) :
      ECGraphs.getSomeCalleeForCallSite(cast<InvokeInst>(OrigInst));

    if (!CF) 
      for (EquivClassGraphs::callee_iterator I = ECGraphs.callee_begin(OrigInst), 
	     E = ECGraphs.callee_end(OrigInst); I != E; ++I)
	if (I->second) {
	  CF = I->second;
	  break;
	}

    // If we didn't find the callee in the constructed call graph, try
    // checking in the DSNode itself.
    // This isn't ideal as it means that this call site didn't have inlining
    // happen.
    if (!CF) {
      DSGraph* dg = &ECGraphs.getDSGraph(*OrigInst->getParent()->getParent());
      DSNode* d = dg->getNodeForValue(OrigInst->getOperand(0)).getNode();
      const std::vector<GlobalValue*> &g = d->getGlobalsList();
      for(std::vector<GlobalValue*>::const_iterator ii = g.begin(), ee = g.end();
	  !CF && ii != ee; ++ii) {
        EquivalenceClasses< GlobalValue *> & EC = ECGraphs.getGlobalECs();
        for (EquivalenceClasses<GlobalValue *>::member_iterator MI = EC.findLeader(*ii);
             MI != EC.member_end(); ++MI)   // Loop over members in this set.
          if ((CF = dyn_cast<Function>(*MI))) {
	    std::cerr << "\n***\nPA: *** WARNING (FuncTransform::visitCallSite): "
		      << "Using DSNode for callees for call-site in function "
		      << CS.getCaller()->getName() << "\n***\n";
 	    break;
	  }
      }
    }

    if (!CF && CTF) {
      std::cerr << "\nPA: Last Resort TD Indirect Resolve in " << CS.getCaller()->getName() << "\n";
      CF = *CTF->begin(isa<CallInst>(OrigInst)?CallSite(cast<CallInst>(OrigInst))
		       :CallSite(cast<InvokeInst>(OrigInst)));
      if (CF) std::cerr << "TD resolved to " << CF->getName() << "\n";
    }

    if (!CF) {
      // FIXME: Unknown callees for a call-site. Warn and ignore.
      std::cerr << "\n***\nPA: *** WARNING (FuncTransform::visitCallSite): "
                << "Unknown callees for call-site in function "
                << CS.getCaller()->getName() << "\n***\n";
      return;
    }

    // Get the common graph for the set of functions this call may invoke.
    CalleeGraph = &ECGraphs.getDSGraph(*CF);
    
#ifndef NDEBUG
    // Verify that all potential callees at call site have the same DS graph.
    EquivClassGraphs::callee_iterator I =
      ECGraphs.callee_begin(OrigInst), E = ECGraphs.callee_end(OrigInst);
    for (; I != E; ++I)
      if (!I->second->isDeclaration())
        assert(CalleeGraph == &ECGraphs.getDSGraph(*I->second) &&
               "Callees at call site do not have a common graph!");
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
    PointerType *PFTy = PointerType::get(FTy);
    
    // If there are any pool arguments cast the func ptr to the right type.
    NewCallee = CastInst::createPointerCast(CS.getCalledValue(), PFTy, "tmp", TheCall);
  }

  Function::arg_iterator FAI = CF->arg_begin(), E = CF->arg_end();
  CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
  for ( ; FAI != E && AI != AE; ++FAI, ++AI)
    if (!isa<Constant>(*AI))
      DSGraph::computeNodeMapping(CalleeGraph->getNodeForValue(FAI),
                                  getDSNodeHFor(*AI), NodeMapping, false);

  //assert(AI == AE && "Varargs calls not handled yet!");

  // Map the return value as well...
  if (DS::isPointerType(TheCall->getType()))
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

  // Okay, now that we have established our mapping, we can figure out which
  // pool descriptors to pass in...
  std::vector<Value*> Args;
  for (unsigned i = 0, e = ArgNodes.size(); i != e; ++i) {
    Value *ArgVal = Constant::getNullValue(PoolAllocate::PoolDescPtrTy);
    if (NodeMapping.count(ArgNodes[i]))
      if (DSNode *LocalNode = NodeMapping[ArgNodes[i]].getNode())
        if (FI.PoolDescriptors.count(LocalNode))
          ArgVal = FI.PoolDescriptors.find(LocalNode)->second;
    if (isa<Constant>(ArgVal) && cast<Constant>(ArgVal)->isNullValue()) {
#ifdef BOUNDS_CHECK
      if (ArgNodes[i]->isArray()) {
#endif
	if (!isa<InvokeInst>(TheCall)) {
	  //Dinakar we need pooldescriptors for allocas in the callee if it escapes
	  BasicBlock::iterator InsertPt = TheCall->getParent()->getParent()->front().begin();
	  Type *VoidPtrTy = PointerType::get(Type::Int8Ty);
	  ArgVal =  new AllocaInst(ArrayType::get(VoidPtrTy, 16), 0, "PD", InsertPt);
	  Value *ElSize = ConstantInt::get(Type::Int32Ty,0);
	  Value *Align  = ConstantInt::get(Type::Int32Ty,0);
          Value* Opts[3] = {ArgVal, ElSize, Align};
	  new CallInst(PAInfo.PoolInit, Opts, 3,"", TheCall);
          BasicBlock::iterator BBI = TheCall;
          new CallInst(PAInfo.PoolDestroy, ArgVal, "", ++BBI);
	}
	//probably need to update DSG
	//      std::cerr << "WARNING: NULL POOL ARGUMENTS ARE PASSED IN!\n";
#ifdef BOUNDS_CHECK	
      }
#endif
    }
    Args.push_back(ArgVal);
  }

  // Add the rest of the arguments...
  Args.insert(Args.end(), CS.arg_begin(), CS.arg_end());
    
  std::string Name = TheCall->getName(); TheCall->setName("");

  if (InvokeInst *II = dyn_cast<InvokeInst>(TheCall)) {
    NewCall = new InvokeInst(NewCallee, II->getNormalDest(),
                             II->getUnwindDest(), &Args[0], Args.size(), Name, TheCall);
  } else {
    NewCall = new CallInst(NewCallee, &Args[0], Args.size(), Name, TheCall);
  }

  // Add all of the uses of the pool descriptor
  for (unsigned i = 0, e = ArgNodes.size(); i != e; ++i)
    AddPoolUse(*NewCall, Args[i], PoolUses);

  TheCall->replaceAllUsesWith(NewCall);
  DEBUG(std::cerr << "  Result Call: " << *NewCall);

  if (TheCall->getType() != Type::VoidTy) {
    // If we are modifying the original function, update the DSGraph... 
    DSGraph::ScalarMapTy &SM = G.getScalarMap();
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

  TheCall->eraseFromParent();
  visitInstruction(*NewCall);
}


// visitInstruction - For all instructions in the transformed function bodies,
// replace any references to the original calls with references to the
// transformed calls.  Many instructions can "take the address of" a function,
// and we must make sure to catch each of these uses, and transform it into a
// reference to the new, transformed, function.
void FuncTransform::visitInstruction(Instruction &I) {
  for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i)
    if (Function *clonedFunc = retCloneIfFunc(I.getOperand(i))) {
      Constant *CF = clonedFunc;
      I.setOperand(i, ConstantExpr::getPointerCast(CF, I.getOperand(i)->getType()));
    }
}
