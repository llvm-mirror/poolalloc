//===-- TransformFunctionBody.cpp - Pool Function Transformer -------------===//
//
// This file defines the PoolAllocate::TransformBody method.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "PoolAllocation"
#include "poolalloc/PoolAllocate.h"
#include "llvm/Analysis/DataStructure.h"
#include "llvm/Analysis/DSGraph.h"
#include "llvm/Function.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/InstVisitor.h"
#include "Support/Debug.h"
#include "Support/VectorExtras.h"
using namespace PA;

namespace {
  /// FuncTransform - This class implements transformation required of pool
  /// allocated functions.
  struct FuncTransform : public InstVisitor<FuncTransform> {
    PoolAllocate &PAInfo;
    DSGraph &G;      // The Bottom-up DS Graph
    DSGraph &TDG;    // The Top-down DS Graph
    FuncInfo &FI;

    // PoolUses - For each pool (identified by the pool descriptor) keep track
    // of which blocks require the memory in the pool to not be freed.  This
    // does not include poolfree's.
    std::set<Value*, BasicBlock*> &PoolUses;

    // PoolDestroys - For each pool, keep track of the actual poolfree calls
    // inserted into the code.  This is seperated out from PoolUses.
    std::set<Value*, CallInst*>   &PoolFrees;

    FuncTransform(PoolAllocate &P, DSGraph &g, DSGraph &tdg, FuncInfo &fi,
                  std::set<Value*, BasicBlock*> &poolUses,
                  std::set<Value*, CallInst*> &poolFrees)
      : PAInfo(P), G(g), TDG(tdg), FI(fi),
        PoolUses(poolUses), PoolFrees(poolFrees) {
    }

    void visitMallocInst(MallocInst &MI);
    void visitFreeInst(FreeInst &FI);
    void visitCallSite(CallSite CS);
    void visitCallInst(CallInst &CI) { visitCallSite(&CI); }
    void visitInvokeInst(InvokeInst &II) { visitCallSite(&II); }
    
    // The following instructions are never modified by pool allocation
    void visitBranchInst(BranchInst &I) { }
    void visitUnwindInst(UnwindInst &I) { }
    void visitBinaryOperator(Instruction &I) { }
    void visitShiftInst (ShiftInst &I) { }
    void visitSwitchInst (SwitchInst &I) { }
    void visitCastInst (CastInst &I) { }
    void visitAllocaInst(AllocaInst &I) { }
    void visitLoadInst(LoadInst &I) { }
    void visitGetElementPtrInst (GetElementPtrInst &I) { }

    void visitReturnInst(ReturnInst &I);
    void visitStoreInst (StoreInst &I);
    void visitPHINode(PHINode &I);

    void visitVAArgInst(VAArgInst &I) { }
    void visitVANextInst(VANextInst &I) { }

    void visitInstruction(Instruction &I) {
      std::cerr << "PoolAllocate does not recognize this instruction:\n" << I;
      abort();
    }

  private:
    DSNodeHandle& getDSNodeHFor(Value *V) {
      if (!FI.NewToOldValueMap.empty()) {
        // If the NewToOldValueMap is in effect, use it.
        std::map<Value*,const Value*>::iterator I = FI.NewToOldValueMap.find(V);
        if (I != FI.NewToOldValueMap.end())
          V = (Value*)I->second;
      }

      return G.getScalarMap()[V];
    }

    DSNodeHandle& getTDDSNodeHFor(Value *V) {
      if (!FI.NewToOldValueMap.empty()) {
        // If the NewToOldValueMap is in effect, use it.
        std::map<Value*,const Value*>::iterator I = FI.NewToOldValueMap.find(V);
        if (I != FI.NewToOldValueMap.end())
          V = (Value*)I->second;
      }

      return TDG.getScalarMap()[V];
    }

    Value *getPoolHandle(Value *V) {
      DSNode *Node = getDSNodeHFor(V).getNode();
      // Get the pool handle for this DSNode...
      std::map<DSNode*, Value*>::iterator I = FI.PoolDescriptors.find(Node);

      if (I != FI.PoolDescriptors.end()) {
	// Check that the node pointed to by V in the TD DS graph is not
	// collapsed
	DSNode *TDNode = getTDDSNodeHFor(V).getNode();
	if (TDNode->getType() != Type::VoidTy)
	  return I->second;
	else {
	  PAInfo.CollapseFlag = 1;
	  return 0;
	}
      }
      else
	return 0;
	  
    }
    
    bool isFuncPtr(Value *V);

    Function* getFuncClass(Value *V);

    Value* retCloneIfFunc(Value *V);
  };
}

void PoolAllocate::TransformBody(DSGraph &g, DSGraph &tdg, PA::FuncInfo &fi,
                                 std::set<Value*, BasicBlock*> &poolUses,
                                 std::set<Value*, CallInst*> &poolFrees,
                                 Function &F) {
  FuncTransform(*this, g, tdg, fi, poolUses, poolFrees).visit(F);
}


// Returns true if V is a function pointer
bool FuncTransform::isFuncPtr(Value *V) {
  if (const PointerType *PTy = dyn_cast<PointerType>(V->getType()))
     return isa<FunctionType>(PTy->getElementType());
  return false;
}

// Given a function pointer, return the function eq. class if one exists
Function* FuncTransform::getFuncClass(Value *V) {
  // Look at DSGraph and see if the set of of functions it could point to
  // are pool allocated.

  if (!isFuncPtr(V))
    return 0;

  // Two cases: 
  // if V is a constant
  if (Function *theFunc = dyn_cast<Function>(V)) {
    if (!PAInfo.FuncECs.findClass(theFunc))
      // If this function does not belong to any equivalence class
      return 0;
    if (PAInfo.EqClass2LastPoolArg.count(PAInfo.FuncECs.findClass(theFunc)))
      return PAInfo.FuncECs.findClass(theFunc);
    else
      return 0;
  }

  // if V is not a constant
  DSNode *DSN = TDG.getNodeForValue(V).getNode();
  if (!DSN) {
    return 0;
  }
  const std::vector<GlobalValue*> &Callees = DSN->getGlobals();
  if (Callees.size() > 0) {
    Function *calledF = dyn_cast<Function>(*Callees.begin());
    assert(PAInfo.FuncECs.findClass(calledF) && "should exist in some eq. class");
    if (PAInfo.EqClass2LastPoolArg.count(PAInfo.FuncECs.findClass(calledF)))
      return PAInfo.FuncECs.findClass(calledF);
  }

  return 0;
}

// Returns the clone if  V is a static function (not a pointer) and belongs 
// to an equivalence class i.e. is pool allocated
Value* FuncTransform::retCloneIfFunc(Value *V) {
  if (Function *fixedFunc = dyn_cast<Function>(V))
    if (getFuncClass(V))
      return PAInfo.getFuncInfo(*fixedFunc)->Clone;
  
  return 0;
}

void FuncTransform::visitReturnInst (ReturnInst &RI) {
  if (RI.getNumOperands())
    if (Value *clonedFunc = retCloneIfFunc(RI.getOperand(0))) {
      // Cast the clone of RI.getOperand(0) to the non-pool-allocated type
      CastInst *CastI = new CastInst(clonedFunc, RI.getOperand(0)->getType(), 
				     "tmp", &RI);
      // Insert return instruction that returns the casted value
      ReturnInst *RetI = new ReturnInst(CastI, &RI);

      // Remove original return instruction
      RI.getParent()->getInstList().erase(&RI);

      if (!FI.NewToOldValueMap.empty()) {
	std::map<Value*,const Value*>::iterator II =
	  FI.NewToOldValueMap.find(&RI);
	assert(II != FI.NewToOldValueMap.end() && 
	       "RI not found in clone?");
	FI.NewToOldValueMap.insert(std::make_pair(RetI, II->second));
	FI.NewToOldValueMap.erase(II);
      }
    }
}

void FuncTransform::visitStoreInst (StoreInst &SI) {
  // Check if a constant function is being stored
  if (Value *clonedFunc = retCloneIfFunc(SI.getOperand(0))) {
    CastInst *CastI = new CastInst(clonedFunc, SI.getOperand(0)->getType(), 
				   "tmp", &SI);
    StoreInst *StoreI = new StoreInst(CastI, SI.getOperand(1), &SI);
    SI.getParent()->getInstList().erase(&SI);
    
    // Update the NewToOldValueMap if this is a clone
    if (!FI.NewToOldValueMap.empty()) {
      std::map<Value*,const Value*>::iterator II =
	FI.NewToOldValueMap.find(&SI);
      assert(II != FI.NewToOldValueMap.end() && 
	     "SI not found in clone?");
      FI.NewToOldValueMap.insert(std::make_pair(StoreI, II->second));
      FI.NewToOldValueMap.erase(II);
    }
  }
}

void FuncTransform::visitPHINode(PHINode &PI) {
  // If any of the operands of the PHI node is a constant function pointer
  // that is cloned, the cast instruction has to be inserted at the end of the
  // previous basic block
  
  if (isFuncPtr(&PI)) {
    PHINode *V = new PHINode(PI.getType(), PI.getName(), &PI);
    for (unsigned i = 0 ; i < PI.getNumIncomingValues(); ++i) {
      if (Value *clonedFunc = retCloneIfFunc(PI.getIncomingValue(i))) {
	// Insert CastInst at the end of  PI.getIncomingBlock(i)
	BasicBlock::iterator BBI = --PI.getIncomingBlock(i)->end();
	// BBI now points to the terminator instruction of the basic block.
	CastInst *CastI = new CastInst(clonedFunc, PI.getType(), "tmp", BBI);
	V->addIncoming(CastI, PI.getIncomingBlock(i));
      } else {
	V->addIncoming(PI.getIncomingValue(i), PI.getIncomingBlock(i));
      }
      
    }
    PI.replaceAllUsesWith(V);
    PI.getParent()->getInstList().erase(&PI);
    
    DSGraph::ScalarMapTy &SM = G.getScalarMap();
    DSGraph::ScalarMapTy::iterator PII = SM.find(&PI); 

    // Update Scalar map of DSGraph if this is one of the original functions
    // Otherwise update the NewToOldValueMap
    if (PII != SM.end()) {
      SM.insert(std::make_pair(V, PII->second));
      SM.erase(PII);                     // Destroy the PHINode
    } else {
      std::map<Value*,const Value*>::iterator II =
	FI.NewToOldValueMap.find(&PI);
      assert(II != FI.NewToOldValueMap.end() && 
	     "PhiI not found in clone?");
      FI.NewToOldValueMap.insert(std::make_pair(V, II->second));
      FI.NewToOldValueMap.erase(II);
    }
  }
}

void FuncTransform::visitMallocInst(MallocInst &MI) {
  // Get the pool handle for the node that this contributes to...
  Value *PH = getPoolHandle(&MI);
  
  // NB: PH is zero even if the node is collapsed
  if (PH == 0) return;

  std::string Name = MI.getName(); MI.setName("");

  // Insert a call to poolalloc
  TargetData &TD = PAInfo.getAnalysis<TargetData>();
  Value *AllocSize =
    ConstantUInt::get(Type::UIntTy, TD.getTypeSize(MI.getAllocatedType()));

  if (MI.isArrayAllocation())
    AllocSize = BinaryOperator::create(Instruction::Mul, AllocSize,
                                       MI.getOperand(0), "sizetmp", &MI);

  Value *V = new CallInst(PAInfo.PoolAlloc, make_vector(PH, AllocSize, 0),
                          Name, &MI);

  const Type *phtype = MI.getType()->getElementType();
  std::map<const Value*, const Type*> &PoolDescType = FI.PoolDescType;
  if (!PoolDescType.count(PH))
    PoolDescType[PH] = phtype;

  Value *Casted = V;

  // Cast to the appropriate type if necessary
  if (V->getType() != MI.getType())
    Casted = new CastInst(V, MI.getType(), V->getName(), &MI);
    
  // Update def-use info
  MI.replaceAllUsesWith(Casted);

  // Remove old malloc instruction
  MI.getParent()->getInstList().erase(&MI);
  
  DSGraph::ScalarMapTy &SM = G.getScalarMap();
  DSGraph::ScalarMapTy::iterator MII = SM.find(&MI);
  
  // If we are modifying the original function, update the DSGraph... 
  if (MII != SM.end()) {
    // V and Casted now point to whatever the original malloc did...
    SM.insert(std::make_pair(V, MII->second));
    if (V != Casted)
      SM.insert(std::make_pair(Casted, MII->second));
    SM.erase(MII);                     // The malloc is now destroyed
  } else {             // Otherwise, update the NewToOldValueMap
    std::map<Value*,const Value*>::iterator MII =
      FI.NewToOldValueMap.find(&MI);
    assert(MII != FI.NewToOldValueMap.end() && "MI not found in clone?");
    FI.NewToOldValueMap.insert(std::make_pair(V, MII->second));
    if (V != Casted)
      FI.NewToOldValueMap.insert(std::make_pair(Casted, MII->second));
    FI.NewToOldValueMap.erase(MII);
  }
}

void FuncTransform::visitFreeInst(FreeInst &FrI) {
  Value *Arg = FrI.getOperand(0);
  Value *PH = getPoolHandle(Arg);  // Get the pool handle for this DSNode...
  if (PH == 0) return;

  const Type *phtype = 0;
  if (const PointerType * ptype = dyn_cast<PointerType>(Arg->getType())) {
    phtype = ptype->getElementType();
  }
  assert((phtype != 0) && "Needs to be implemented \n ");
  std::map<const Value*, const Type*> &PoolDescType = FI.PoolDescType;
  if (!PoolDescType.count(PH))
    PoolDescType[PH] = phtype;
  
  // Insert a cast and a call to poolfree...
  Value *Casted = Arg;
  if (Arg->getType() != PointerType::get(Type::SByteTy))
    Casted = new CastInst(Arg, PointerType::get(Type::SByteTy),
				 Arg->getName()+".casted", &FrI);

  CallInst *FreeI = new CallInst(PAInfo.PoolFree, make_vector(PH, Casted, 0), 
				 "", &FrI);
  // Delete the now obsolete free instruction...
  FrI.getParent()->getInstList().erase(&FrI);
  
  // Update the NewToOldValueMap if this is a clone
  if (!FI.NewToOldValueMap.empty()) {
    std::map<Value*,const Value*>::iterator II =
      FI.NewToOldValueMap.find(&FrI);
    assert(II != FI.NewToOldValueMap.end() && 
	   "FrI not found in clone?");
    FI.NewToOldValueMap.insert(std::make_pair(FreeI, II->second));
    FI.NewToOldValueMap.erase(II);
  }
}

static void CalcNodeMapping(DSNodeHandle& Caller, DSNodeHandle& Callee,
                            std::map<DSNode*, DSNode*> &NodeMapping) {
  DSNode *CalleeNode = Callee.getNode();
  DSNode *CallerNode = Caller.getNode();

  unsigned CalleeOffset = Callee.getOffset();
  unsigned CallerOffset = Caller.getOffset();

  if (CalleeNode == 0) return;

  // If callee has a node and caller doesn't, then a constant argument was
  // passed by the caller
  if (CallerNode == 0) {
    NodeMapping.insert(NodeMapping.end(), std::make_pair(CalleeNode, 
							 (DSNode *) 0));
  }

  // Map the callee node to the caller node. 
  // NB: The callee node could be of a different type. Eg. if it points to the
  // field of a struct that the caller points to
  std::map<DSNode*, DSNode*>::iterator I = NodeMapping.find(CalleeNode);
  if (I != NodeMapping.end()) {   // Node already in map...
    assert(I->second == CallerNode && 
	   "Node maps to different nodes on paths?");
  } else {
    NodeMapping.insert(I, std::make_pair(CalleeNode, CallerNode));
    
    if (CalleeNode->getType() != CallerNode->getType() && CallerOffset == 0) 
      DEBUG(std::cerr << "NB: Mapping of nodes between different types\n");

    // Recursively map the callee links to the caller links starting from the
    // offset in the node into which they are mapped.
    // Being a BU Graph, the callee ought to have smaller number of links unless
    // there is collapsing in the caller
    unsigned numCallerLinks = CallerNode->getNumLinks() - CallerOffset;
    unsigned numCalleeLinks = CalleeNode->getNumLinks() - CalleeOffset;
    
    if (numCallerLinks > 0) {
      if (numCallerLinks < numCalleeLinks) {
	DEBUG(std::cerr << "Potential node collapsing in caller\n");
	for (unsigned i = 0, e = numCalleeLinks; i != e; ++i)
	  CalcNodeMapping(CallerNode->getLink(((i%numCallerLinks) << DS::PointerShift) + CallerOffset),
                          CalleeNode->getLink((i << DS::PointerShift) + CalleeOffset), NodeMapping);
      } else {
	for (unsigned i = 0, e = numCalleeLinks; i != e; ++i)
	  CalcNodeMapping(CallerNode->getLink((i << DS::PointerShift) + CallerOffset),
                          CalleeNode->getLink((i << DS::PointerShift) + CalleeOffset), NodeMapping);
      }
    } else if (numCalleeLinks > 0) {
      DEBUG(std::cerr << 
	    "Caller has unexpanded node, due to indirect call perhaps!\n");
    }
  }
}

void FuncTransform::visitCallSite(CallSite CS) {
  Function *CF = CS.getCalledFunction();
  Instruction *TheCall = CS.getInstruction();

  // optimization for function pointers that are basically gotten from a cast
  // with only one use and constant expressions with casts in them
  if (!CF) {
    Value *CV = CS.getCalledValue();
    if (CastInst* CastI = dyn_cast<CastInst>(CV)) {
      if (isa<Function>(CastI->getOperand(0)) && 
	  CastI->getOperand(0)->getType() == CastI->getType())
	CF = dyn_cast<Function>(CastI->getOperand(0));
    } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CV)) {
      if (CE->getOpcode() == Instruction::Cast)
	if (ConstantPointerRef *CPR
            = dyn_cast<ConstantPointerRef>(CE->getOperand(0)))
          CF = dyn_cast<Function>(CPR->getValue());
    }
  }

  DSGraph &CallerG = G;

  std::vector<Value*> Args;  

  // We need to figure out which local pool descriptors correspond to the pool
  // descriptor arguments passed into the function call.  Calculate a mapping
  // from callee DSNodes to caller DSNodes.  We construct a partial isomophism
  // between the graphs to figure out which pool descriptors need to be passed
  // in.  The roots of this mapping is found from arguments and return values.
  //
  std::map<DSNode*, DSNode*> NodeMapping;

  if (!CF) {   // Indirect call
    DEBUG(std::cerr << "  Handling call: " << *TheCall);
    
    std::map<unsigned, Value*> PoolArgs;
    Function *FuncClass;
    
    std::pair<std::multimap<CallSite, Function*>::iterator,
              std::multimap<CallSite, Function*>::iterator> Targets =
      PAInfo.CallSiteTargets.equal_range(CS);
    for (std::multimap<CallSite, Function*>::iterator TFI = Targets.first,
	   TFE = Targets.second; TFI != TFE; ++TFI) {
      if (TFI == Targets.first) {
	FuncClass = PAInfo.FuncECs.findClass(TFI->second);
	// Nothing to transform if there are no pool arguments in this
	// equivalence class of functions.
	if (!PAInfo.EqClass2LastPoolArg.count(FuncClass))
	  return;
      }
      
      FuncInfo *CFI = PAInfo.getFuncInfo(*TFI->second);

      if (!CFI->ArgNodes.size()) continue;  // Nothing to transform...
      
      DSGraph &CG = PAInfo.getBUDataStructures().getDSGraph(*TFI->second);  
      
      {
        Function::aiterator FAI = TFI->second->abegin(),E = TFI->second->aend();
        CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
        for ( ; FAI != E && AI != AE; ++FAI, ++AI)
          if (!isa<Constant>(*AI))
            CalcNodeMapping(getDSNodeHFor(*AI), CG.getScalarMap()[FAI],
                            NodeMapping);
        assert(AI == AE && "Varargs calls not handled yet!");
      }
      
      if (TheCall->getType() != Type::VoidTy)
        CalcNodeMapping(getDSNodeHFor(TheCall),
                        CG.getReturnNodeFor(*TFI->second), NodeMapping);
      
      // Map the nodes that are pointed to by globals.
      // For all globals map getDSNodeForGlobal(g)->CG.getDSNodeForGlobal(g)
      for (DSGraph::ScalarMapTy::iterator SMI = G.getScalarMap().begin(), 
             SME = G.getScalarMap().end(); SMI != SME; ++SMI)
        if (isa<GlobalValue>(SMI->first)) { 
          CalcNodeMapping(SMI->second, 
                          CG.getScalarMap()[SMI->first], NodeMapping);
        }

      unsigned idx = CFI->PoolArgFirst;

      // The following loop determines the pool pointers corresponding to 
      // CFI.
      for (unsigned i = 0, e = CFI->ArgNodes.size(); i != e; ++i, ++idx) {
        if (NodeMapping.count(CFI->ArgNodes[i])) {
          assert(NodeMapping.count(CFI->ArgNodes[i]) && "Node not in mapping!");
          DSNode *LocalNode = NodeMapping.find(CFI->ArgNodes[i])->second;
          if (LocalNode) {
            assert(FI.PoolDescriptors.count(LocalNode) &&
                   "Node not pool allocated?");
            PoolArgs[idx] = FI.PoolDescriptors.find(LocalNode)->second;
          }
          else
            // LocalNode is null when a constant is passed in as a parameter
            PoolArgs[idx] = Constant::getNullValue(PoolAllocate::PoolDescPtrTy);
        } else {
          PoolArgs[idx] = Constant::getNullValue(PoolAllocate::PoolDescPtrTy);
        }
      }
    }
    
    // Push the pool arguments into Args.
    if (PAInfo.EqClass2LastPoolArg.count(FuncClass)) {
      for (int i = 0; i <= PAInfo.EqClass2LastPoolArg[FuncClass]; ++i) {
        if (PoolArgs.find(i) != PoolArgs.end())
          Args.push_back(PoolArgs[i]);
        else
          Args.push_back(Constant::getNullValue(PoolAllocate::PoolDescPtrTy));
      }
    
      assert(Args.size()== (unsigned) PAInfo.EqClass2LastPoolArg[FuncClass] + 1
             && "Call has same number of pool args as the called function");
    }

    // Add the rest of the arguments (the original arguments of the function)...
    Args.insert(Args.end(), CS.arg_begin(), CS.arg_end());
    
    std::string Name = TheCall->getName(); TheCall->setName("");
    
    Value *NewCall;
    Value *CalledValuePtr = CS.getCalledValue();
    if (Args.size() > (unsigned)CS.arg_size()) {
      // If there are any pool arguments
      CalledValuePtr = new CastInst(CS.getCalledValue(), 
                       PAInfo.getFuncInfo(*FuncClass)->Clone->getType(), "tmp", 
                                    TheCall);
    }

    if (InvokeInst *II = dyn_cast<InvokeInst>(TheCall)) {
      NewCall = new InvokeInst(CalledValuePtr, II->getNormalDest(),
                               II->getExceptionalDest(), Args, Name, TheCall);
    } else {
      NewCall = new CallInst(CalledValuePtr, Args, Name, TheCall);
    }

    TheCall->replaceAllUsesWith(NewCall);
    DEBUG(std::cerr << "  Result Call: " << *NewCall);
      
    if (TheCall->getType() != Type::VoidTy) {
      // If we are modifying the original function, update the DSGraph... 
      DSGraph::ScalarMapTy &SM = G.getScalarMap();
      DSGraph::ScalarMapTy::iterator CII = SM.find(TheCall);
      if (CII != SM.end()) {
	SM.insert(std::make_pair(NewCall, CII->second));
	SM.erase(CII);                     // Destroy the CallInst
      } else { 
	// Otherwise update the NewToOldValueMap with the new CI return value
	std::map<Value*,const Value*>::iterator CII = 
	  FI.NewToOldValueMap.find(TheCall);
	assert(CII != FI.NewToOldValueMap.end() && "CI not found in clone?");
	FI.NewToOldValueMap.insert(std::make_pair(NewCall, CII->second));
	FI.NewToOldValueMap.erase(CII);
      }
    } else if (!FI.NewToOldValueMap.empty()) {
      std::map<Value*,const Value*>::iterator II =
	FI.NewToOldValueMap.find(TheCall);
      assert(II != FI.NewToOldValueMap.end() && 
	     "CI not found in clone?");
      FI.NewToOldValueMap.insert(std::make_pair(NewCall, II->second));
      FI.NewToOldValueMap.erase(II);
    }
  } else {
    FuncInfo *CFI = PAInfo.getFuncInfo(*CF);

    if (CFI == 0 || CFI->Clone == 0) return;  // Nothing to transform...
    
    DEBUG(std::cerr << "  Handling call: " << *TheCall);
    
    DSGraph &CG = PAInfo.getBUDataStructures().getDSGraph(*CF);  // Callee graph

    {
      Function::aiterator FAI = CF->abegin(), E = CF->aend();
      CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
      for ( ; FAI != E && AI != AE; ++FAI, ++AI)
        if (!isa<Constant>(*AI))
          CalcNodeMapping(getDSNodeHFor(*AI), CG.getScalarMap()[FAI],
                          NodeMapping);
      assert(AI == AE && "Varargs calls not handled yet!");
    }

    // Map the return value as well...
    if (TheCall->getType() != Type::VoidTy)
      CalcNodeMapping(getDSNodeHFor(TheCall), CG.getReturnNodeFor(*CF),
		      NodeMapping);

    // Map the nodes that are pointed to by globals.
    // For all globals map getDSNodeForGlobal(g)->CG.getDSNodeForGlobal(g)
    for (DSGraph::ScalarMapTy::iterator SMI = G.getScalarMap().begin(), 
	   SME = G.getScalarMap().end(); SMI != SME; ++SMI)
      if (isa<GlobalValue>(SMI->first)) 
	CalcNodeMapping(SMI->second, 
			CG.getScalarMap()[SMI->first], NodeMapping);

    // Okay, now that we have established our mapping, we can figure out which
    // pool descriptors to pass in...

    // Add an argument for each pool which must be passed in...
    if (CFI->PoolArgFirst != 0) {
      for (int i = 0; i < CFI->PoolArgFirst; ++i)
	Args.push_back(Constant::getNullValue(PoolAllocate::PoolDescPtrTy));  
    }

    for (unsigned i = 0, e = CFI->ArgNodes.size(); i != e; ++i) {
      if (NodeMapping.count(CFI->ArgNodes[i])) {

	DSNode *LocalNode = NodeMapping.find(CFI->ArgNodes[i])->second;
	if (LocalNode) {
	  assert(FI.PoolDescriptors.count(LocalNode) &&
                 "Node not pool allocated?");
	  Args.push_back(FI.PoolDescriptors.find(LocalNode)->second);
	} else
	  Args.push_back(Constant::getNullValue(PoolAllocate::PoolDescPtrTy));
      } else {
	Args.push_back(Constant::getNullValue(PoolAllocate::PoolDescPtrTy));
      }
    }

    Function *FuncClass = PAInfo.FuncECs.findClass(CF);
    
    if (PAInfo.EqClass2LastPoolArg.count(FuncClass))
      for (int i = CFI->PoolArgLast; 
	   i <= PAInfo.EqClass2LastPoolArg[FuncClass]; ++i)
	Args.push_back(Constant::getNullValue(PoolAllocate::PoolDescPtrTy));

    // Add the rest of the arguments...
    Args.insert(Args.end(), CS.arg_begin(), CS.arg_end());
    
    std::string Name = TheCall->getName(); TheCall->setName("");

    Value *NewCall;
    if (InvokeInst *II = dyn_cast<InvokeInst>(TheCall)) {
      NewCall = new InvokeInst(CFI->Clone, II->getNormalDest(),
                               II->getExceptionalDest(), Args, Name, TheCall);
    } else {
      NewCall = new CallInst(CFI->Clone, Args, Name, TheCall);
    }

    TheCall->replaceAllUsesWith(NewCall);
    DEBUG(std::cerr << "  Result Call: " << *NewCall);

    if (TheCall->getType() != Type::VoidTy) {
      // If we are modifying the original function, update the DSGraph... 
      DSGraph::ScalarMapTy &SM = G.getScalarMap();
      DSGraph::ScalarMapTy::iterator CII = SM.find(TheCall); 
      if (CII != SM.end()) {
	SM.insert(std::make_pair(NewCall, CII->second));
	SM.erase(CII);                     // Destroy the CallInst
      } else { 
	// Otherwise update the NewToOldValueMap with the new CI return value
	std::map<Value*,const Value*>::iterator CNII = 
	  FI.NewToOldValueMap.find(TheCall);
	assert(CNII != FI.NewToOldValueMap.end() && CNII->second && 
	       "CI not found in clone?");
	FI.NewToOldValueMap.insert(std::make_pair(NewCall, CNII->second));
	FI.NewToOldValueMap.erase(CNII);
      }
    } else if (!FI.NewToOldValueMap.empty()) {
      std::map<Value*,const Value*>::iterator II =
	FI.NewToOldValueMap.find(TheCall);
      assert(II != FI.NewToOldValueMap.end() && "CI not found in clone?");
      FI.NewToOldValueMap.insert(std::make_pair(NewCall, II->second));
      FI.NewToOldValueMap.erase(II);
    }
  }

  TheCall->getParent()->getInstList().erase(TheCall);
}
