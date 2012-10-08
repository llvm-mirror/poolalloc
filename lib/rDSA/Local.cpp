//===- Local.cpp - Compute a local data structure graph for a function ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Compute the local version of the data structure graph for a function.  The
// external interface to this file is the DSGraph constructor.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "dsa-local"

#include "rdsa/DataStructure.h"
#include "rdsa/DSGraph.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Intrinsics.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/DataLayout.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/ADT/Statistic.h"

using namespace llvm;

static RegisterPass<LocalDataStructures>
X("dsa-local", "Local Data Structure Analysis");

namespace {

  //===--------------------------------------------------------------------===//
  //  GraphBuilderBase Class
  //===--------------------------------------------------------------------===//
  //
  /// This class contains the common graph building coe.
  ///
  class GraphBuilderBase {
    DSGraph* G;
  protected:
    /// createNode - Create a new DSNode, ensuring that it is properly added to
    /// the graph.
    ///
    DSNode* createNode(const Type *Ty = 0) {   
      return new DSNode(Ty, G);
    }
    DSNode* createNode(const Type *Ty, GlobalVariable* GV) {   
      DSNode* N = new DSNode(Ty, G);
      N->addGlobal(GV);
      return N;
    }
    DSNode* createNode(const Type *Ty, GlobalAlias* GA) {   
      DSNode* N = new DSNode(Ty, G);
      N->addGlobal(GA);
      return N;
    }
    DSNode* createNode(const Type *Ty, Function* FV) {   
      DSNode* N = new DSNode(Ty, G);
      N->addFunction(FV);
      return N;
    }

    DSNodeHandle& getNodeForValue(const Value *V) { 
      return G->getScalarMap()[V];
    }

    bool hasNodeForValue(const Value* V) const {
      return G->hasNodeForValue(V);
    }
    void eraseNodeForValue(const Value* V) {
      G->eraseNodeForValue(V);
    }

    void addDSCallSite(const DSCallSite& DSCS) {
      G->getFunctionCalls().push_back(DSCS);
    }

    /// getValueDest - Return the DSNode that the actual value points to.
    ///
    DSNodeHandle getValueDest(Value &V);
    DSNodeHandle getValueDest(GlobalVariable &V);
    DSNodeHandle getValueDest(GlobalAlias &V);
    DSNodeHandle getValueDest(Function &V);

    /// setDestTo - Set the ScalarMap entry for the specified value to point to
    /// the specified destination.  If the Value already points to a node, make
    /// sure to merge the two destinations together.
    ///
    void setDestTo(Value &V, const DSNodeHandle &NH);

    void visitGetElementPtrInst(User &GEP);

    GraphBuilderBase(DSGraph* _G) :G(_G) {}

  };

  //===--------------------------------------------------------------------===//
  //  GraphBuilderGlobal Class
  //===--------------------------------------------------------------------===//
  //
  /// This class is the builder class that constructs the local data structure
  /// graph by performing a single pass over the function in question.
  ///
  class GraphBuilderGlobal : GraphBuilderBase {

    void MergeConstantInitIntoNode(DSNodeHandle &NH, const Type* Ty, Constant *C);

  public:
    // GraphBuilderGlobal ctor for working on the globals graph
    explicit GraphBuilderGlobal(DSGraph* g)
      :GraphBuilderBase(g)
    {}

    void mergeGlobal(GlobalVariable *GV);
    void mergeAlias(GlobalAlias *GV);
    void mergeFunction(Function* F);

  };

 
  //===--------------------------------------------------------------------===//
  //  GraphBuilderLocal Class
  //===--------------------------------------------------------------------===//
  //
  /// This class is the builder class that constructs the local data structure
  /// graph by performing a single pass over the function in question.
  ///
  class GraphBuilderLocal : GraphBuilderBase, InstVisitor<GraphBuilderLocal> {
    DataStructures* DS;
    DSNodeHandle&  RetNode;

    ////////////////////////////////////////////////////////////////////////////
    // Helper functions used to implement the visitation functions...

    /// getLink - This method is used to return the specified link in the
    /// specified node if one exists.  If a link does not already exist (it's
    /// null), then we create a new node, link it, then return it.
    ///
    DSNodeHandle &getLink(const DSNodeHandle &Node, unsigned Link = 0);

    ////////////////////////////////////////////////////////////////////////////
    // Visitor functions, used to handle each instruction type we encounter...
    friend class InstVisitor<GraphBuilderLocal>;

    //the simple ones
    void visitMallocInst(MallocInst &MI);
    void visitAllocaInst(AllocaInst &AI);
    void visitFreeInst(FreeInst &FI);
    void visitPHINode(PHINode &PN);
    void visitSelectInst(SelectInst &SI);
    void visitLoadInst(LoadInst &LI);
    void visitStoreInst(StoreInst &SI);
    void visitReturnInst(ReturnInst &RI);
    void visitVAArgInst(VAArgInst   &I);
    void visitIntToPtrInst(IntToPtrInst &I);
    void visitPtrToIntInst(PtrToIntInst &I);
    void visitBitCastInst(BitCastInst &I);
    void visitCmpInst(CmpInst &I);

    void visitExtractValueInst(ExtractValueInst &I);
    void visitInsertValueInst(InsertValueInst &I);

    //the nasty ones
    void visitCallInst(CallInst &CI);
    void visitInvokeInst(InvokeInst &II);
    void visitInstruction(Instruction &I);
    void visitGetElementPtrInst(User &GEP) { 
      GraphBuilderBase::visitGetElementPtrInst(GEP);
    }

    bool visitIntrinsic(CallSite CS, Function* F);
    void visitCallSite(CallSite CS);

  public:
    GraphBuilderLocal(Function& f, DSNodeHandle& retnode, DSGraph* g, 
                      DataStructures& DSi);
  };
}

GraphBuilderLocal::GraphBuilderLocal(Function& f, DSNodeHandle& retnode, DSGraph* g, DataStructures& DSi)
  : GraphBuilderBase(g), DS(&DSi), RetNode(retnode) {

  // Create scalar nodes for all pointer arguments...
  for (Function::arg_iterator I = f.arg_begin(), E = f.arg_end();
       I != E; ++I) {
    if (isa<PointerType>(I->getType())) {
      DSNode * Node = getValueDest(*I).getNode();
      
      if (!f.hasInternalLinkage())
        Node->NodeType.setEscapedNode();
      
    }
  }
  
  visit(f);  // Single pass over the function
  
  // If there are any constant globals referenced in this function, merge
  // their initializers into the local graph from the globals graph.
  if (g->getScalarMap().global_begin() != g->getScalarMap().global_end()) {
    ReachabilityCloner RC(g, g->getGlobalsGraph(), 0);
    
    for (DSScalarMap::global_iterator I = g->getScalarMap().global_begin();
         I != g->getScalarMap().global_end(); ++I)
      if (const GlobalVariable *GV = dyn_cast<GlobalVariable>(*I))
        if (!GV->isDeclaration() && GV->isConstant())
          RC.merge(g->getNodeForValue(GV), g->getGlobalsGraph()->getNodeForValue(GV));
  }
  
  g->markIncompleteNodes(DSGraph::MarkFormalArgs);
  
  // Remove any nodes made dead due to merging...
  g->removeDeadNodes(DSGraph::KeepUnreachableGlobals);
}

//===----------------------------------------------------------------------===//
// Helper method implementations...
//


///
/// getValueDest - Return the DSNode that the actual value points to.
///
DSNodeHandle GraphBuilderBase::getValueDest(Value &Val) {
  Value *V = &Val;
  if (isa<Constant>(V) && cast<Constant>(V)->isNullValue()) 
    return 0;  // Null doesn't point to anything, don't add to ScalarMap!

  DSNodeHandle &NH = getNodeForValue(V);
  if (!NH.isNull())
    return NH;     // Already have a node?  Just return it...

  // Otherwise we need to create a new node to point to.

  if (GlobalVariable* GV = dyn_cast<GlobalVariable>(V))
    // Create a new global node for this global variable.
    return NH.setTo(createNode(GV->getType()->getElementType(), GV), 0);

  if (Function* FV = dyn_cast<Function>(V))
    // Create a new global node for this global variable.
    return NH.setTo(createNode(0, FV), 0);

  if (GlobalAlias* GA = dyn_cast<GlobalAlias>(V))
    return NH.setTo(createNode(GA->getType()->getElementType(), GA), 0);

  if (isa<UndefValue>(V)) {
    eraseNodeForValue(V);
    return 0;
  }

  // Check first for constant expressions that must be traversed to
  // extract the actual value.
  DSNode* N;
  if (Constant *C = dyn_cast<Constant>(V)) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
      if (CE->isCast()) {
        if (isa<PointerType>(CE->getOperand(0)->getType()))
          NH = getValueDest(*CE->getOperand(0));
        else {
          NH = createNode();
          NH.getNode()->NodeType.setUnknownNode();
        }
      } else if (CE->getOpcode() == Instruction::GetElementPtr) {
        visitGetElementPtrInst(*CE);
        assert(hasNodeForValue(CE) && "GEP didn't get processed right?");
        NH = getNodeForValue(CE);
      } else {
        // This returns a conservative unknown node for any unhandled ConstExpr
        NH = createNode();
        NH.getNode()->NodeType.setUnknownNode();
        return NH;
      }
      if (NH.isNull()) {  // (getelementptr null, X) returns null
        eraseNodeForValue(V);
        return 0;
      }
      return NH;
    } else {
      DEBUG(errs() << "Unknown constant: " << *C << "\n");
      assert(0 && "Unknown constant type!");
    }
    N = createNode(); // just create a shadow node
  } else {
    // Otherwise just create a shadow node
    N = createNode();
  }

  return NH.setTo(N, 0);      // Remember that we are pointing to it...
}

//Save some type casts and conditionals on programs with many many globals
DSNodeHandle GraphBuilderBase::getValueDest(GlobalVariable &Val) {
  GlobalVariable *V = &Val;
  DSNodeHandle &NH = getNodeForValue(V);
  if (!NH.isNull())
    return NH;     // Already have a node?  Just return it...

  // Otherwise we need to create a new node to point to.
  // Create a new global node for this global variable.
  return NH.setTo(createNode(V->getType()->getElementType(), V), 0);
}
DSNodeHandle GraphBuilderBase::getValueDest(GlobalAlias &Val) {
  GlobalAlias *V = &Val;
  DSNodeHandle &NH = getNodeForValue(V);
  if (!NH.isNull())
    return NH;     // Already have a node?  Just return it...

  // Otherwise we need to create a new node to point to.
  // Create a new global node for this global variable.
  return NH.setTo(createNode(V->getType()->getElementType(), V), 0);
}
DSNodeHandle GraphBuilderBase::getValueDest(Function &Val) {
  Function *V = &Val;
  DSNodeHandle &NH = getNodeForValue(V);
  if (!NH.isNull())
    return NH;     // Already have a node?  Just return it...

  // Otherwise we need to create a new node to point to.
  // Create a new global node for this global variable.
  return NH.setTo(createNode(0, V), 0);
}


/// getLink - This method is used to return the specified link in the
/// specified node if one exists.  If a link does not already exist (it's
/// null), then we create a new node, link it, then return it.  We must
/// specify the type of the Node field we are accessing so that we know what
/// type should be linked to if we need to create a new node.
///
DSNodeHandle &GraphBuilderLocal::getLink(const DSNodeHandle &node, unsigned LinkNo) {
  DSNodeHandle &Node = const_cast<DSNodeHandle&>(node);
  DSNodeHandle &Link = Node.getLink(LinkNo);
  if (Link.isNull()) {
    // If the link hasn't been created yet, make and return a new shadow node
    Link = createNode();
  }
  return Link;
}


/// setDestTo - Set the ScalarMap entry for the specified value to point to the
/// specified destination.  If the Value already points to a node, make sure to
/// merge the two destinations together.
///
void GraphBuilderBase::setDestTo(Value &V, const DSNodeHandle &NH) {
  getNodeForValue(&V).mergeWith(NH);
}


//===----------------------------------------------------------------------===//
// Specific instruction type handler implementations...
//
void GraphBuilderLocal::visitMallocInst(MallocInst &MI) {
  DSNode* N = createNode();
  N->NodeType.setHeapNode();
  setDestTo(MI, N);
}

void GraphBuilderLocal::visitAllocaInst(AllocaInst &AI) {
  DSNode* N = createNode();
  N->NodeType.setAllocaNode();
  setDestTo(AI, N);
}

void GraphBuilderLocal::visitFreeInst(FreeInst &FI) {
  if (DSNode *N = getValueDest(*FI.getOperand(0)).getNode())
    N->NodeType.setHeapNode();
}

// PHINode - Make the scalar for the PHI node point to all of the things the
// incoming values point to... which effectively causes them to be merged.
//
void GraphBuilderLocal::visitPHINode(PHINode &PN) {
  if (!isa<PointerType>(PN.getType())) return; // Only pointer PHIs

  DSNodeHandle &PNDest = getNodeForValue(&PN);
  for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i)
    PNDest.mergeWith(getValueDest(*PN.getIncomingValue(i)));
}

void GraphBuilderLocal::visitSelectInst(SelectInst &SI) {
  if (!isa<PointerType>(SI.getType()))
    return; // Only pointer Selects

  DSNodeHandle &Dest = getNodeForValue(&SI);
  DSNodeHandle S1 = getValueDest(*SI.getOperand(1));
  DSNodeHandle S2 = getValueDest(*SI.getOperand(2));
  Dest.mergeWith(S1);
  Dest.mergeWith(S2);
}

void GraphBuilderLocal::visitLoadInst(LoadInst &LI) {
  DSNodeHandle Ptr = getValueDest(*LI.getOperand(0));

  if (Ptr.isNull()) return; // Load from null

  // Make that the node is read from...
  Ptr.getNode()->NodeType.setReadNode();

  // Ensure a typerecord exists...
  Ptr.getNode()->mergeTypeInfo(LI.getType(), Ptr.getOffset(), false);

  if (isa<PointerType>(LI.getType()))
    setDestTo(LI, getLink(Ptr));
}

void GraphBuilderLocal::visitStoreInst(StoreInst &SI) {
  const Type *StoredTy = SI.getOperand(0)->getType();
  DSNodeHandle Dest = getValueDest(*SI.getOperand(1));
  if (Dest.isNull()) return;

  // Mark that the node is written to...
  Dest.getNode()->NodeType.setModifiedNode();

  // Ensure a type-record exists...
  Dest.getNode()->mergeTypeInfo(StoredTy, Dest.getOffset());

  // Avoid adding edges from null, or processing non-"pointer" stores
  if (isa<PointerType>(StoredTy))
    Dest.addEdgeTo(getValueDest(*SI.getOperand(0)));
}

void GraphBuilderLocal::visitReturnInst(ReturnInst &RI) {
  if (RI.getNumOperands() && isa<PointerType>(RI.getOperand(0)->getType()))
    RetNode.mergeWith(getValueDest(*RI.getOperand(0)));
}

void GraphBuilderLocal::visitVAArgInst(VAArgInst &I) {
  //FIXME: also updates the argument
  DSNodeHandle Ptr = getValueDest(*I.getOperand(0));
  if (Ptr.isNull()) return;

  // Make that the node is read and written
  Ptr.getNode()->NodeType.setReadNode();
  Ptr.getNode()->NodeType.setModifiedNode();

  // Ensure a type record exists.
  DSNode *PtrN = Ptr.getNode();
  PtrN->mergeTypeInfo(I.getType(), Ptr.getOffset(), false);

  if (isa<PointerType>(I.getType()))
    setDestTo(I, getLink(Ptr));
}

void GraphBuilderLocal::visitIntToPtrInst(IntToPtrInst &I) {
//  std::cerr << "cast in " << I.getParent()->getParent()->getName() << "\n";
//  I.dump();
  DSNode* N = createNode();
  N->NodeType.setUnknownNode();
  N->NodeType.setIntToPtrNode();
  setDestTo(I, N);
}

void GraphBuilderLocal::visitPtrToIntInst(PtrToIntInst& I) {
  if (DSNode* N = getValueDest(*I.getOperand(0)).getNode())
    N->NodeType.setPtrToIntNode();
}


void GraphBuilderLocal::visitBitCastInst(BitCastInst &I) {
  if (!isa<PointerType>(I.getType())) return; // Only pointers
  DSNodeHandle Ptr = getValueDest(*I.getOperand(0));
  if (Ptr.isNull()) return;
  setDestTo(I, Ptr);
}

void GraphBuilderLocal::visitCmpInst(CmpInst &I) {
  // TODO: must merge pointers
}

void GraphBuilderBase::visitGetElementPtrInst(User &GEP) {
  DSNodeHandle Value = getValueDest(*GEP.getOperand(0));
  if (Value.isNull())
    Value = createNode();

  // As a special case, if all of the index operands of GEP are constant zeros,
  // handle this just like we handle casts (ie, don't do much).
  bool AllZeros = true;
  for (unsigned i = 1, e = GEP.getNumOperands(); i != e; ++i) {
    if (ConstantInt * CI = dyn_cast<ConstantInt>(GEP.getOperand(i)))
      if (CI->isZero()) {
        continue;
      }
    AllZeros = false;
    break;
  }

  // If all of the indices are zero, the result points to the operand without
  // applying the type.
  if (AllZeros || (!Value.isNull() &&
                   Value.getNode()->isNodeCompletelyFolded())) {
    setDestTo(GEP, Value);
    return;
  }


  const PointerType *PTy = cast<PointerType>(GEP.getOperand(0)->getType());
  const Type *CurTy = PTy->getElementType();

  if (Value.getNode()->mergeTypeInfo(CurTy, Value.getOffset())) {
    // If the node had to be folded... exit quickly
    setDestTo(GEP, Value);  // GEP result points to folded node

    return;
  }

  const DataLayout &TD = Value.getNode()->getDataLayout();

#if 0
  // Handle the pointer index specially...
  if (GEP.getNumOperands() > 1 &&
      (!isa<Constant>(GEP.getOperand(1)) ||
       !cast<Constant>(GEP.getOperand(1))->isNullValue())) {

    // If we already know this is an array being accessed, don't do anything...
    if (!TopTypeRec.isArray) {
      TopTypeRec.isArray = true;

      // If we are treating some inner field pointer as an array, fold the node
      // up because we cannot handle it right.  This can come because of
      // something like this:  &((&Pt->X)[1]) == &Pt->Y
      //
      if (Value.getOffset()) {
        // Value is now the pointer we want to GEP to be...
        Value.getNode()->foldNodeCompletely();
        setDestTo(GEP, Value);  // GEP result points to folded node
        return;
      } else {
        // This is a pointer to the first byte of the node.  Make sure that we
        // are pointing to the outter most type in the node.
        // FIXME: We need to check one more case here...
      }
    }
  }
#endif

  // All of these subscripts are indexing INTO the elements we have...
  unsigned Offset = 0;
  for (gep_type_iterator I = gep_type_begin(GEP), E = gep_type_end(GEP);
       I != E; ++I)
    if (const StructType *STy = dyn_cast<StructType>(*I)) {
      const ConstantInt* CUI = cast<ConstantInt>(I.getOperand());
#if 0
      unsigned FieldNo = 
        CUI->getType()->isSigned() ? CUI->getSExtValue() : CUI->getZExtValue();
#else
      int FieldNo = CUI->getSExtValue();
#endif
      Offset += (unsigned)TD.getStructLayout(STy)->getElementOffset(FieldNo);
    } else if (isa<PointerType>(*I)) {
      if (!isa<Constant>(I.getOperand()) ||
          !cast<Constant>(I.getOperand())->isNullValue())
        Value.getNode()->NodeType.setArrayNode();
    }


#if 0
    if (const SequentialType *STy = cast<SequentialType>(*I)) {
      CurTy = STy->getElementType();
      if (ConstantInt *CS = dyn_cast<ConstantInt>(GEP.getOperand(i))) {
        Offset += 
          (CS->getType()->isSigned() ? CS->getSExtValue() : CS->getZExtValue())
          * TD.getTypeAllocSize(CurTy);
      } else {
        // Variable index into a node.  We must merge all of the elements of the
        // sequential type here.
        if (isa<PointerType>(STy)) {
          DEBUG(errs() << "Pointer indexing not handled yet!\n");
	} else {
          const ArrayType *ATy = cast<ArrayType>(STy);
          unsigned ElSize = TD.getTypeAllocSize(CurTy);
          DSNode *N = Value.getNode();
          assert(N && "Value must have a node!");
          unsigned RawOffset = Offset+Value.getOffset();

          // Loop over all of the elements of the array, merging them into the
          // zeroth element.
          for (unsigned i = 1, e = ATy->getNumElements(); i != e; ++i)
            // Merge all of the byte components of this array element
            for (unsigned j = 0; j != ElSize; ++j)
              N->mergeIndexes(RawOffset+j, RawOffset+i*ElSize+j);
        }
      }
    }
#endif

  // Add in the offset calculated...
  Value.setOffset(Value.getOffset()+Offset);

  // Check the offset
  DSNode *N = Value.getNode();
  if (N &&
      !N->isNodeCompletelyFolded() &&
      (N->getSize() != 0 || Offset != 0) &&
      !N->isForwarding()) {
    if ((Offset >= N->getSize()) || int(Offset) < 0) {
      // Accessing offsets out of node size range
      // This is seen in the "magic" struct in named (from bind), where the
      // fourth field is an array of length 0, presumably used to create struct
      // instances of different sizes

      // Collapse the node since its size is now variable
      N->foldNodeCompletely();
    }
  }

  // Value is now the pointer we want to GEP to be...  
  setDestTo(GEP, Value);
#if 0
  if (debug && (isa<Instruction>(GEP))) {
    Instruction * IGEP = (Instruction *)(&GEP);
    DSNode * N = Value.getNode();
    if (IGEP->getParent()->getParent()->getName() == "alloc_vfsmnt")
    {
      if (G.getPoolDescriptorsMap().count(N) != 0)
        if (G.getPoolDescriptorsMap()[N]) {
	  DEBUG(errs() << "LLVA: GEP[" << 0 << "]: Pool for " << GEP.getName() << " is " << G.getPoolDescriptorsMap()[N]->getName() << "\n");
	}
    }
  }
#endif

}


void GraphBuilderLocal::visitCallInst(CallInst &CI) {
  visitCallSite(&CI);
}

void GraphBuilderLocal::visitInvokeInst(InvokeInst &II) {
  visitCallSite(&II);
}

/// returns true if the intrinsic is handled
bool GraphBuilderLocal::visitIntrinsic(CallSite CS, Function *F) {
  switch (F->getIntrinsicID()) {
  case Intrinsic::vastart: {
    // Mark the memory written by the vastart intrinsic as incomplete
    DSNodeHandle RetNH = getValueDest(**CS.arg_begin());
    if (DSNode *N = RetNH.getNode()) {
      N->NodeType.setModifiedNode();
      N->NodeType.setAllocaNode();
      N->NodeType.setIncompleteNode();
      N->NodeType.setUnknownNode();
      N->foldNodeCompletely();
    }

    if (RetNH.hasLink(0)) {
      DSNodeHandle Link = RetNH.getLink(0);
      if (DSNode *N = Link.getNode()) {
        N->NodeType.setModifiedNode();
        N->NodeType.setAllocaNode();
        N->NodeType.setIncompleteNode();
        N->NodeType.setUnknownNode();
        N->foldNodeCompletely();
      }
    } else {
      //
      // Sometimes the argument to the vastart is casted and has no DSNode.
      // Peer past the cast.
      //
      Value * Operand = CS.getInstruction()->getOperand(1);
      if (CastInst * CI = dyn_cast<CastInst>(Operand))
        Operand = CI->getOperand (0);
      RetNH = getValueDest(*Operand);
      if (DSNode *N = RetNH.getNode()) {
        N->NodeType.setModifiedNode();
        N->NodeType.setAllocaNode();
        N->NodeType.setIncompleteNode();
        N->NodeType.setUnknownNode();
        N->foldNodeCompletely();
      }
    }

    return true;
  }
  case Intrinsic::vacopy:
    getValueDest(*CS.getInstruction()).
      mergeWith(getValueDest(**(CS.arg_begin())));
    return true;
  case Intrinsic::stacksave: {
    DSNode * Node = createNode();
    Node->NodeType.setAllocaNode();
    Node->NodeType.setIncompleteNode();
    Node->NodeType.setUnknownNode();
    Node->foldNodeCompletely();
    setDestTo (*(CS.getInstruction()), Node);
    return true;
  }
  case Intrinsic::stackrestore: {
    DSNode* Node = getValueDest(*CS.getInstruction()).getNode();
    Node->NodeType.setAllocaNode();
    Node->NodeType.setIncompleteNode();
    Node->NodeType.setUnknownNode();
    Node->foldNodeCompletely();
    return true;
  }
  case Intrinsic::vaend:
  case Intrinsic::dbg_func_start:
  case Intrinsic::dbg_region_end:
  case Intrinsic::dbg_stoppoint:
  case Intrinsic::dbg_region_start:
  case Intrinsic::dbg_declare:
    return true;  // noop
  case Intrinsic::memcpy: 
  case Intrinsic::memmove: {
    // Merge the first & second arguments, and mark the memory read and
    // modified.
    DSNodeHandle RetNH = getValueDest(**CS.arg_begin());
    RetNH.mergeWith(getValueDest(**(CS.arg_begin()+1)));
    if (DSNode *N = RetNH.getNode()) {
      N->NodeType.setModifiedNode();
      N->NodeType.setReadNode();
    }
    return true;
  }
  case Intrinsic::memset:
    // Mark the memory modified.
    if (DSNode *N = getValueDest(**CS.arg_begin()).getNode())
      N->NodeType.setModifiedNode();
    return true;

  case Intrinsic::eh_exception: {
    DSNode * Node = createNode();
    Node->NodeType.setIncompleteNode();
    Node->foldNodeCompletely();
    setDestTo (*(CS.getInstruction()), Node);
    return true;
  }

  case Intrinsic::atomic_cmp_swap: {
    DSNodeHandle Ptr = getValueDest(**CS.arg_begin());
    Ptr.getNode()->NodeType.setReadNode();
    Ptr.getNode()->NodeType.setModifiedNode();
    if (isa<PointerType>(F->getReturnType())) {
      setDestTo(*(CS.getInstruction()), getValueDest(**(CS.arg_begin() + 1)));
      getValueDest(**(CS.arg_begin() + 1))
        .mergeWith(getValueDest(**(CS.arg_begin() + 2)));
    }
  }
  case Intrinsic::atomic_swap:
  case Intrinsic::atomic_load_add:
  case Intrinsic::atomic_load_sub:
  case Intrinsic::atomic_load_and:
  case Intrinsic::atomic_load_nand:
  case Intrinsic::atomic_load_or:
  case Intrinsic::atomic_load_xor:
  case Intrinsic::atomic_load_max:
  case Intrinsic::atomic_load_min:
  case Intrinsic::atomic_load_umax:
  case Intrinsic::atomic_load_umin:
    {
      DSNodeHandle Ptr = getValueDest(**CS.arg_begin());
      Ptr.getNode()->NodeType.setReadNode();
      Ptr.getNode()->NodeType.setModifiedNode();
      if (isa<PointerType>(F->getReturnType()))
        setDestTo(*(CS.getInstruction()), getValueDest(**(CS.arg_begin() + 1)));
    }
   
              

  case Intrinsic::eh_selector_i32:
  case Intrinsic::eh_selector_i64:
  case Intrinsic::eh_typeid_for_i32:
  case Intrinsic::eh_typeid_for_i64:
  case Intrinsic::prefetch:
    return true;

  //
  // The return address aliases with the stack, is type-unknown, and should
  // have the unknown flag set since we don't know where it goes.
  //
  case Intrinsic::returnaddress: {
    DSNode * Node = createNode();
    Node->NodeType.setAllocaNode();
    Node->NodeType.setIncompleteNode();
    Node->NodeType.setUnknownNode();
    Node->foldNodeCompletely();
    setDestTo (*(CS.getInstruction()), Node);
    return true;
  }

  default: {
    //ignore pointer free intrinsics
    if (!isa<PointerType>(F->getReturnType())) {
      bool hasPtr = false;
      for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
           I != E && !hasPtr; ++I)
        if (isa<PointerType>(I->getType()))
          hasPtr = true;
      if (!hasPtr)
        return true;
    }

    DEBUG(errs() << "[dsa:local] Unhandled intrinsic: " << F->getName() << "\n");
    assert(0 && "Unhandled intrinsic");
    return false;
  }
  }
}

void GraphBuilderLocal::visitCallSite(CallSite CS) {
  Value *Callee = CS.getCalledValue();

  // Special case handling of certain libc allocation functions here.
  if (Function *F = dyn_cast<Function>(Callee))
    if (F->isDeclaration())
      if (F->isIntrinsic() && visitIntrinsic(CS, F))
        return;

  // Set up the return value...
  DSNodeHandle RetVal;
  Instruction *I = CS.getInstruction();
  if (isa<PointerType>(I->getType()))
    RetVal = getValueDest(*I);

  if (!isa<Function>(Callee))
    if (ConstantExpr* EX = dyn_cast<ConstantExpr>(Callee))
      if (EX->isCast() && isa<Function>(EX->getOperand(0)))
        Callee = cast<Function>(EX->getOperand(0));

  DSNode *CalleeNode = 0;
  if (!isa<Function>(Callee)) {
    CalleeNode = getValueDest(*Callee).getNode();
    if (CalleeNode == 0) {
      DEBUG(errs() << "WARNING: Program is calling through a null pointer?\n" << *I);
      return;  // Calling a null pointer?
    }
  }

  std::vector<DSNodeHandle> Args;
  Args.reserve(CS.arg_end()-CS.arg_begin());

  // Calculate the arguments vector...
  for (CallSite::arg_iterator I = CS.arg_begin(), E = CS.arg_end(); I != E; ++I)
    if (isa<PointerType>((*I)->getType()))
      Args.push_back(getValueDest(**I));
    else
      Args.push_back(0);

  // Add a new function call entry...
  if (CalleeNode) {
    addDSCallSite(DSCallSite(CS, RetVal, CalleeNode, Args));
  } else
    addDSCallSite(DSCallSite(CS, RetVal, cast<Function>(Callee), Args));
}

// visitInstruction - For all other instruction types, if we have any arguments
// that are of pointer type, make them have unknown composition bits, and merge
// the nodes together.
void GraphBuilderLocal::visitInstruction(Instruction &Inst) {
  DSNodeHandle CurNode;
  if (isa<PointerType>(Inst.getType()))
    CurNode = getValueDest(Inst);
  for (User::op_iterator I = Inst.op_begin(), E = Inst.op_end(); I != E; ++I)
    if (isa<PointerType>((*I)->getType()))
      CurNode.mergeWith(getValueDest(**I));

  if (DSNode *N = CurNode.getNode())
    N->NodeType.setUnknownNode();
}

void GraphBuilderLocal::visitExtractValueInst(ExtractValueInst &I) {
  assert(!isa<PointerType>(I.getType()) && "ExtractValue not supported");
}

void GraphBuilderLocal::visitInsertValueInst(InsertValueInst &I) {
  assert(!isa<PointerType>((*(I.op_begin()))->getType()) && "InsertValue not supported");
}


//===----------------------------------------------------------------------===//
// LocalDataStructures Implementation
//===----------------------------------------------------------------------===//

// MergeConstantInitIntoNode - Merge the specified constant into the node
// pointed to by NH.
void GraphBuilderGlobal::MergeConstantInitIntoNode(DSNodeHandle &NH, const Type* Ty, Constant *C) {
  // Ensure a type-record exists...
  DSNode *NHN = NH.getNode();
  NHN->mergeTypeInfo(Ty, NH.getOffset());

  if (isa<PointerType>(Ty)) {
    // Avoid adding edges from null, or processing non-"pointer" stores
    NH.addEdgeTo(getValueDest(*C));
    return;
  }

  if (Ty->isIntOrIntVector() || Ty->isFPOrFPVector()) return;

  const DataLayout &TD = NH.getNode()->getDataLayout();

  if (ConstantArray *CA = dyn_cast<ConstantArray>(C)) {
    for (unsigned i = 0, e = CA->getNumOperands(); i != e; ++i)
      // We don't currently do any indexing for arrays...
      MergeConstantInitIntoNode(NH, cast<ArrayType>(Ty)->getElementType(), cast<Constant>(CA->getOperand(i)));
  } else if (ConstantStruct *CS = dyn_cast<ConstantStruct>(C)) {
    const StructLayout *SL = TD.getStructLayout(cast<StructType>(Ty));
    for (unsigned i = 0, e = CS->getNumOperands(); i != e; ++i) {
      DSNode *NHN = NH.getNode();
      //Some programmers think ending a structure with a [0 x sbyte] is cute
      if (SL->getElementOffset(i) < SL->getSizeInBytes()) {
        DSNodeHandle NewNH(NHN, NH.getOffset()+(unsigned)SL->getElementOffset(i));
        MergeConstantInitIntoNode(NewNH, cast<StructType>(Ty)->getElementType(i), cast<Constant>(CS->getOperand(i)));
      } else if (SL->getElementOffset(i) == SL->getSizeInBytes()) {
        DOUT << "Zero size element at end of struct\n";
        NHN->foldNodeCompletely();
      } else {
        assert(0 && "type was smaller than offsets of of struct layout indicate");
      }
    }
  } else if (isa<ConstantAggregateZero>(C) || isa<UndefValue>(C)) {
    // Noop
  } else {
    assert(0 && "Unknown constant type!");
  }
}

void GraphBuilderGlobal::mergeGlobal(GlobalVariable *GV) {
  assert(!GV->isDeclaration() && "Cannot merge in external global!");
  // Get a node handle to the global node and merge the initializer into it.
  DSNodeHandle NH = getValueDest(*GV);
  MergeConstantInitIntoNode(NH, GV->getType()->getElementType(), GV->getInitializer());
}
void GraphBuilderGlobal::mergeAlias(GlobalAlias *GV) {
  assert(!GV->isDeclaration() && "Cannot merge in external alias!");
  // Get a node handle to the global node and merge the initializer into it.
  DSNodeHandle NH = getValueDest(*GV);
  DSNodeHandle NHT = getValueDest(*GV->getAliasee());
  NH.mergeWith(NHT);
}
void GraphBuilderGlobal::mergeFunction(Function* F) {
  assert(!F->isDeclaration() && "Cannot merge in external global!");
  DSNodeHandle NH = getValueDest(*F);
}


char LocalDataStructures::ID;

bool LocalDataStructures::runOnModule(Module &M) {
  init(&getAnalysis<DataLayout>());

  // First step, build the globals graph.
  {
    GraphBuilderGlobal GGB(GlobalsGraph);

    // Add initializers for all of the globals to the globals graph.
    for (Module::global_iterator I = M.global_begin(), E = M.global_end();
         I != E; ++I)
      if (!I->isDeclaration())
        GGB.mergeGlobal(I);
    for (Module::alias_iterator I = M.alias_begin(), E = M.alias_end();
         I != E; ++I)
      if (!I->isDeclaration())
        GGB.mergeAlias(I);
    // Add Functions to the globals graph.
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
      if (!I->isDeclaration())
        GGB.mergeFunction(I);
  }

  // Next step, iterate through the nodes in the globals graph, unioning
  // together the globals into equivalence classes.
  formGlobalECs();

  // Calculate all of the graphs...
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration()) {
      DSGraph* G = new DSGraph(GlobalECs, getDataLayout(), GlobalsGraph);
      GraphBuilderLocal GGB(*I, G->getOrCreateReturnNodeFor(*I), G, *this);
      G->getAuxFunctionCalls() = G->getFunctionCalls();
      setDSGraph(I, G);
    }

  GlobalsGraph->removeTriviallyDeadNodes();
  GlobalsGraph->markIncompleteNodes(DSGraph::MarkFormalArgs);

  // Now that we've computed all of the graphs, and merged all of the info into
  // the globals graph, see if we have further constrained the globals in the
  // program if so, update GlobalECs and remove the extraneous globals from the
  // program.
  formGlobalECs();

  return false;
}

