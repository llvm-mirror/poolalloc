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
#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Intrinsics.h"
#include "llvm/InlineAsm.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Timer.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Triple.h"

#include <fstream>

// FIXME: This should eventually be a FunctionPass that is automatically
// aggregated into a Pass.
//
#include "llvm/Module.h"

using namespace llvm;

namespace {
STATISTIC(NumDirectCall,    "Number of direct calls added");
STATISTIC(NumIndirectCall,  "Number of indirect calls added");
STATISTIC(NumAsmCall,       "Number of asm calls collapsed/seen");
STATISTIC(NumBoringCall,    "Number of pointer-free direct calls ignored");
STATISTIC(NumIntrinsicCall, "Number of intrinsics called");

RegisterPass<LocalDataStructures>
X("dsa-local", "Local Data Structure Analysis");

cl::opt<std::string> hasMagicSections("dsa-magic-sections",
        cl::desc("File with section to global mapping")); //, cl::ReallyHidden);
}

namespace {
  //===--------------------------------------------------------------------===//
  //  GraphBuilder Class
  //===--------------------------------------------------------------------===//
  //
  /// This class is the builder class that constructs the local data structure
  /// graph by performing a single pass over the function in question.
  ///
  class GraphBuilder : InstVisitor<GraphBuilder> {
    DSGraph &G;
    Function* FB;
    DataStructures* DS;
    const TargetData& TD;

    ////////////////////////////////////////////////////////////////////////////
    // Helper functions used to implement the visitation functions...

    void MergeConstantInitIntoNode(DSNodeHandle &NH, const Type* Ty, Constant *C);

    /// createNode - Create a new DSNode, ensuring that it is properly added to
    /// the graph.
    ///
    DSNode *createNode() 
    {   
      DSNode* ret = new DSNode(&G);
      assert(ret->getParentGraph() && "No parent?");
      return ret;
    }

    /// setDestTo - Set the ScalarMap entry for the specified value to point to
    /// the specified destination.  If the Value already points to a node, make
    /// sure to merge the two destinations together.
    ///
    void setDestTo(Value &V, const DSNodeHandle &NH);

    /// getValueDest - Return the DSNode that the actual value points to.
    ///
    DSNodeHandle getValueDest(Value* V);

    /// getLink - This method is used to return the specified link in the
    /// specified node if one exists.  If a link does not already exist (it's
    /// null), then we create a new node, link it, then return it.
    ///
    DSNodeHandle &getLink(const DSNodeHandle &Node, unsigned Link = 0);

    ////////////////////////////////////////////////////////////////////////////
    // Visitor functions, used to handle each instruction type we encounter...
    friend class InstVisitor<GraphBuilder>;

    //TODO: generalize
    bool visitAllocation(CallSite CS) {
      if (Function* F = CS.getCalledFunction()) {
        if (F->hasName() && F->getName() == "malloc") {
          setDestTo(*CS.getInstruction(), createNode()->setHeapMarker());
          return true;
        }
      }
      return false;
    }

    //FIXME: implement in stdlib pass
//    void visitMallocInst(MallocInst &MI)
//    { setDestTo(MI, createNode()->setHeapMarker()); }

    void visitAllocaInst(AllocaInst &AI)
    { setDestTo(AI, createNode()->setAllocaMarker()); }

    //FIXME: implement in stdlib pass
    //void visitFreeInst(FreeInst &FI)
    //{ if (DSNode *N = getValueDest(FI.getOperand(0)).getNode())
//        N->setHeapMarker();
//    }

    //the simple ones
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
    void visitInsertValueInst(InsertValueInst& I);
    void visitExtractValueInst(ExtractValueInst& I);

    //the nasty ones
    void visitGetElementPtrInst(User &GEP);
    void visitCallInst(CallInst &CI);
    void visitInvokeInst(InvokeInst &II);
    void visitInstruction(Instruction &I);

    bool visitIntrinsic(CallSite CS, Function* F);
    void visitCallSite(CallSite CS);
    void visitVAStartInst(CallSite CS);
    void visitVAStartNode(DSNode* N);

  public:
    GraphBuilder(Function &f, DSGraph &g, DataStructures& DSi)
      : G(g), FB(&f), DS(&DSi), TD(g.getTargetData()) {
      // Create scalar nodes for all pointer arguments...
      for (Function::arg_iterator I = f.arg_begin(), E = f.arg_end();
           I != E; ++I) {
        if (isa<PointerType>(I->getType())) {
          // WD: Why do we set the external marker so early in the analysis?
          // Functions we have definitions for, but are externally reachable have no external contexts
          // that we'd want to BU external information into (since those contexts are by definition
          // ones we don't have code for).  Shouldn't this just be set in TD?
#if 0
          DSNode * Node = getValueDest(I).getNode();

          if (!f.hasInternalLinkage())
            Node->setExternalMarker();
#else
          getValueDest(I).getNode();
#endif

        }
      }

      // Create an entry for the return, which tracks which functions are in
      // the graph
      g.getOrCreateReturnNodeFor(f);

      // Create a node to handle information about variable arguments
      g.getOrCreateVANodeFor(f);

      visit(f);  // Single pass over the function

      // If there are any constant globals referenced in this function, merge
      // their initializers into the local graph from the globals graph.
      // This resolves indirect calls in some common cases
      // Only merge info for nodes that already exist in the local pass
      // otherwise leaf functions could contain less collapsing than the globals
      // graph
       if (g.getScalarMap().global_begin() != g.getScalarMap().global_end()) {
        ReachabilityCloner RC(&g, g.getGlobalsGraph(), 0);
        std::vector<const GlobalValue*> GVV(g.getScalarMap().global_begin(),
                                            g.getScalarMap().global_end());
        for (std::vector<const GlobalValue*>::iterator I = GVV.begin();
             I != GVV.end(); ++I)
          if (const GlobalVariable * GV = dyn_cast<GlobalVariable > (*I))
            if (GV->isConstant())
              RC.merge(g.getNodeForValue(GV), g.getGlobalsGraph()->getNodeForValue(GV));
      }

      g.markIncompleteNodes(DSGraph::MarkFormalArgs);

      // Compute sources of external
      unsigned EFlags = 0
        | DSGraph::DontMarkFormalsExternal
        | DSGraph::ProcessCallSites;
      // Mark globals reachable from formals as external if we don't have
      // internal linkage.
      if (!f.hasInternalLinkage()) {
        EFlags |= DSGraph::MarkGlobalsReachableFromFormals;
      }

      g.computeExternalFlags(EFlags);

      // Remove any nodes made dead due to merging...
      g.removeDeadNodes(DSGraph::KeepUnreachableGlobals);
    }

    // GraphBuilder ctor for working on the globals graph
    explicit GraphBuilder(DSGraph& g)
      :G(g), FB(0), TD(g.getTargetData())
    {}

    void mergeInGlobalInitializer(GlobalVariable *GV);
    void mergeExternalGlobal(GlobalVariable* GV);
    void mergeFunction(Function* F) { getValueDest(F); }
  };

  /// Traverse the whole DSGraph, and propagate the unknown flags through all 
  /// out edges.
  static void propagateUnknownFlag(DSGraph * G) {
    std::vector<DSNode *> workList;
    DenseSet<DSNode *> visited;
    for (DSGraph::node_iterator I = G->node_begin(), E = G->node_end(); I != E; ++I)
      if (I->isUnknownNode()) 
        workList.push_back(&*I);
  
    while (!workList.empty()) {
      DSNode * N = workList.back();
      workList.pop_back();
      if (visited.count(N) != 0) continue;
      visited.insert(N);
      N->setUnknownMarker();
      for (DSNode::edge_iterator I = N->edge_begin(), E = N->edge_end(); I != E; ++I)
        if (!I->second.isNull())
          workList.push_back(I->second.getNode());
    }
  }
}

//===----------------------------------------------------------------------===//
// Helper method implementations...
//


///
/// getValueDest - Return the DSNode that the actual value points to.
///
DSNodeHandle GraphBuilder::getValueDest(Value* V) {
  if (isa<Constant>(V) && cast<Constant>(V)->isNullValue()) 
    return 0;  // Null doesn't point to anything, don't add to ScalarMap!

  DSNodeHandle &NH = G.getNodeForValue(V);
  if (!NH.isNull())
    return NH;     // Already have a node?  Just return it...

  // Otherwise we need to create a new node to point to.
  // Check first for constant expressions that must be traversed to
  // extract the actual value.
  DSNode* N;
  if (Function * F = dyn_cast<Function > (V)) {
    // Create a new global node for this function.
    N = createNode();
    N->addFunction(F);
    if (F->isDeclaration())
      N->setExternFuncMarker();
  } else if (GlobalValue * GV = dyn_cast<GlobalValue > (V)) {
    // Create a new global node for this global variable.
    N = createNode();
    N->addGlobal(GV);
    if (GV->isDeclaration())
      N->setExternGlobalMarker();
  } else if (Constant *C = dyn_cast<Constant>(V)) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
      if (CE->isCast()) {
        if (isa<PointerType>(CE->getOperand(0)->getType()))
          NH = getValueDest(CE->getOperand(0));
        else
          NH = createNode()->setUnknownMarker();
      } else if (CE->getOpcode() == Instruction::GetElementPtr) {
        visitGetElementPtrInst(*CE);
        assert(G.hasNodeForValue(CE) && "GEP didn't get processed right?");
        NH = G.getNodeForValue(CE);
      } else {
        // This returns a conservative unknown node for any unhandled ConstExpr
        NH = createNode()->setUnknownMarker();
      }
      if (NH.isNull()) {  // (getelementptr null, X) returns null
        G.eraseNodeForValue(V);
        return 0;
      }
      return NH;
    } else if (isa<UndefValue>(C)) {
      G.eraseNodeForValue(V);
      return 0;
    } else if (isa<GlobalAlias>(C)) {
      // XXX: Need more investigation
      // According to Andrew, DSA is broken on global aliasing, since it does
      // not handle the aliases of parameters correctly. Here is only a quick
      // fix for some special cases.
      NH = getValueDest(cast<GlobalAlias>(C)->getAliasee());
      return NH;
    } else {
      errs() << "Unknown constant: " << *C << "\n";
      assert(0 && "Unknown constant type!");
    }
    N = createNode(); // just create a shadow node
  } else {
    // Otherwise just create a shadow node
    N = createNode();
  }

  NH.setTo(N, 0);      // Remember that we are pointing to it...
  return NH;
}


/// getLink - This method is used to return the specified link in the
/// specified node if one exists.  If a link does not already exist (it's
/// null), then we create a new node, link it, then return it.  We must
/// specify the type of the Node field we are accessing so that we know what
/// type should be linked to if we need to create a new node.
///
DSNodeHandle &GraphBuilder::getLink(const DSNodeHandle &node, unsigned LinkNo) {
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
void GraphBuilder::setDestTo(Value &V, const DSNodeHandle &NH) {
  G.getNodeForValue(&V).mergeWith(NH);
}


//===----------------------------------------------------------------------===//
// Specific instruction type handler implementations...
//

// PHINode - Make the scalar for the PHI node point to all of the things the
// incoming values point to... which effectively causes them to be merged.
//
void GraphBuilder::visitPHINode(PHINode &PN) {
  if (!isa<PointerType>(PN.getType())) return; // Only pointer PHIs

  DSNodeHandle &PNDest = G.getNodeForValue(&PN);
  for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i)
    PNDest.mergeWith(getValueDest(PN.getIncomingValue(i)));
}

void GraphBuilder::visitSelectInst(SelectInst &SI) {
  if (!isa<PointerType>(SI.getType()))
    return; // Only pointer Selects

  DSNodeHandle &Dest = G.getNodeForValue(&SI);
  DSNodeHandle S1 = getValueDest(SI.getOperand(1));
  DSNodeHandle S2 = getValueDest(SI.getOperand(2));
  Dest.mergeWith(S1);
  Dest.mergeWith(S2);
}

void GraphBuilder::visitLoadInst(LoadInst &LI) {
  DSNodeHandle Ptr = getValueDest(LI.getOperand(0));

  if (Ptr.isNull()) return; // Load from null

  // Make that the node is read from...
  Ptr.getNode()->setReadMarker();

  // Ensure a typerecord exists...
  Ptr.getNode()->mergeTypeInfo(LI.getType(), Ptr.getOffset());

  if (isa<PointerType>(LI.getType()))
    setDestTo(LI, getLink(Ptr));
}

void GraphBuilder::visitStoreInst(StoreInst &SI) {
  const Type *StoredTy = SI.getOperand(0)->getType();
  DSNodeHandle Dest = getValueDest(SI.getOperand(1));
  if (Dest.isNull()) return;

  // Mark that the node is written to...
  Dest.getNode()->setModifiedMarker();

  // Ensure a type-record exists...
  Dest.getNode()->mergeTypeInfo(StoredTy, Dest.getOffset());

  // Avoid adding edges from null, or processing non-"pointer" stores
  if (isa<PointerType>(StoredTy))
    Dest.addEdgeTo(getValueDest(SI.getOperand(0)));
}

void GraphBuilder::visitReturnInst(ReturnInst &RI) {
  if (RI.getNumOperands() && isa<PointerType>(RI.getOperand(0)->getType()))
    G.getOrCreateReturnNodeFor(*FB).mergeWith(getValueDest(RI.getOperand(0)));
}

void GraphBuilder::visitVAArgInst(VAArgInst &I) {
  assert(0 && "What frontend generates this?");
  //FIXME: also updates the argument
  DSNodeHandle Ptr = getValueDest(I.getOperand(0));
  if (Ptr.isNull()) return;

  // Make that the node is read and written
  Ptr.getNode()->setReadMarker()->setModifiedMarker();

  // Ensure a type record exists.
  DSNode *PtrN = Ptr.getNode();
  PtrN->mergeTypeInfo(I.getType(), Ptr.getOffset());

  if (isa<PointerType>(I.getType()))
    setDestTo(I, getLink(Ptr));
}

void GraphBuilder::visitIntToPtrInst(IntToPtrInst &I) {
//  std::cerr << "cast in " << I.getParent()->getParent()->getName() << "\n";
//  I.dump();
  setDestTo(I, createNode()->setUnknownMarker()->setIntToPtrMarker()); 
}

void GraphBuilder::visitPtrToIntInst(PtrToIntInst& I) {
  if (DSNode* N = getValueDest(I.getOperand(0)).getNode())
    N->setPtrToIntMarker();
}


void GraphBuilder::visitBitCastInst(BitCastInst &I) {
  
  if (!isa<PointerType>(I.getType())) return; // Only pointers
  DSNodeHandle Ptr = getValueDest(I.getOperand(0));
  if (Ptr.isNull()) return;
  setDestTo(I, Ptr);
}

void GraphBuilder::visitCmpInst(CmpInst &I) {
  //Address can escape through cmps
}

void GraphBuilder::visitInsertValueInst(InsertValueInst& I) {
  setDestTo(I, createNode()->setAllocaMarker());

  const Type *StoredTy = I.getInsertedValueOperand()->getType();
  DSNodeHandle Dest = getValueDest(&I);
  Dest.mergeWith(getValueDest(I.getAggregateOperand()));

  // Mark that the node is written to...
  Dest.getNode()->setModifiedMarker();

  // Ensure a type-record exists...
  Dest.getNode()->mergeTypeInfo(StoredTy, I.getInsertedValueOperandIndex()); 

  // Avoid adding edges from null, or processing non-"pointer" stores
  if (isa<PointerType>(StoredTy))
    Dest.addEdgeTo(getValueDest(I.getInsertedValueOperand()));
}

void GraphBuilder::visitExtractValueInst(ExtractValueInst& I) {
  DSNodeHandle Ptr = getValueDest(I.getOperand(0));

  // Make that the node is read from...
  Ptr.getNode()->setReadMarker();

  // Ensure a typerecord exists...
  Ptr.getNode()->mergeTypeInfo(I.getType(), I.getAggregateOperandIndex());

  if (isa<PointerType>(I.getType()))
    setDestTo(I, getLink(Ptr));
}

void GraphBuilder::visitGetElementPtrInst(User &GEP) {
  //
  // Ensure that the indexed pointer has a DSNode.
  //
  DSNodeHandle Value = getValueDest(GEP.getOperand(0));
  if (Value.isNull())
    Value = createNode();

  //
  // There are a few quick and easy cases to handle.  If the index operands of
  // the GEP are all zero, or if the DSNode of the indexed pointer is already
  // folded, then we know that the result of the GEP will have the same offset
  // into the same DSNode as the indexed pointer.
  //
  bool AllZeros = true;
  for (unsigned i = 1, e = GEP.getNumOperands(); i != e; ++i) {
    if (ConstantInt * CI = dyn_cast<ConstantInt>(GEP.getOperand(i)))
      if (CI->isZero()) {
        continue;
      }
    AllZeros = false;
    break;
  }

  if (AllZeros || (!Value.isNull() &&
                   Value.getNode()->isNodeCompletelyFolded())) {
    setDestTo(GEP, Value);
    return;
  }

  //
  // Okay, no easy way out.  Calculate the offset into the object being
  // indexed.
  //

  //
  // Ensure the uncollapsed node has a large enough size for the struct type
  //
  // FIXME: I am not sure if the code below is completely correct (especially
  //        if we start doing fancy analysis on non-constant array indices).
  //        What if the array is indexed using a larger index than its declared
  //        size?  Does the LLVM verifier catch such issues?
  //
  /*const PointerType *PTy = cast<PointerType > (GEP.getOperand(0)->getType());
  const Type *CurTy = PTy->getElementType();
  if (TD.getTypeAllocSize(CurTy) + Value.getOffset() > Value.getNode()->getSize())
    Value.getNode()->growSize(TD.getTypeAllocSize(CurTy) + Value.getOffset());*/

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

  //
  // Determine the offset (in bytes) between the result of the GEP and the
  // GEP's pointer operand.
  //
  // Note: All of these subscripts are indexing INTO the elements we have...
  //
  // FIXME: We can do better for array indexing.  First, if the array index is
  //        constant, we can determine how much farther we're moving the
  //        pointer.  Second, we can try to use the results of other analysis
  //        passes (e.g., ScalarEvolution) to find min/max values to do less
  //        conservative type-folding.
  //
  unsigned Offset = 0;
  for (gep_type_iterator I = gep_type_begin(GEP), E = gep_type_end(GEP);
       I != E; ++I)
    if (const StructType *STy = dyn_cast<StructType>(*I)) {
      // indexing into a structure
      // next index must be a constant
      const ConstantInt* CUI = cast<ConstantInt>(I.getOperand());
      int FieldNo = CUI->getSExtValue();
      // increment the offset by the actual byte offset being accessed
      Offset += (unsigned)TD.getStructLayout(STy)->getElementOffset(FieldNo);
     
      if(!Value.getNode()->isArrayNode() || Value.getNode()->getSize() <= 1){
        if (TD.getTypeAllocSize(STy) + Value.getOffset() > Value.getNode()->getSize())
          Value.getNode()->growSize(TD.getTypeAllocSize(STy) + Value.getOffset());
      }

    } else if(const ArrayType *ATy = dyn_cast<ArrayType>(*I)) {
       // indexing into an array.
     Value.getNode()->setArrayMarker();
     const Type *CurTy = ATy->getElementType();

      if(!isa<ArrayType>(CurTy) &&
         Value.getNode()->getSize() <= 1) {
          Value.getNode()->growSize(TD.getTypeAllocSize(CurTy));
      }
      if(CurTy->isVoidTy()) {
        Value.getNode()->growSize(1);
      }
      if(isa<ArrayType>(CurTy) && Value.getNode()->getSize() <= 1){
        const Type *ETy = (cast<ArrayType>(CurTy))->getElementType();
        while(isa<ArrayType>(ETy)) {
          ETy = (cast<ArrayType>(ETy))->getElementType();
         }
        Value.getNode()->growSize(TD.getTypeAllocSize(ETy));
        if(ETy->isVoidTy()) {
          Value.getNode()->growSize(1);
        }
     }
// indexing into an array.
      
      // Find if the DSNode belongs to the array
      // If not fold.
      if((Value.getOffset() || Offset != 0) 
	 || (!isa<ArrayType>(CurTy)
         && (Value.getNode()->getSize() != TD.getTypeAllocSize(CurTy)))) {

        Value.getNode()->foldNodeCompletely();
        Value.getNode();
        Offset = 0;
        break;
      }
    } else if (const PointerType *PtrTy = dyn_cast<PointerType>(*I)) { 
      const Type *CurTy = PtrTy->getElementType();

      //
      // Unless we're advancing the pointer by zero bytes via array indexing,
      // fold the node (i.e., mark it type-unknown) and indicate that we're
      // indexing zero bytes into the object.
      //
      // Note that we break out of the loop if we fold the node.  Once
      // something is folded, all values within it are considered to alias.
      //
      
      if (!isa<Constant>(I.getOperand()) ||
          !cast<Constant>(I.getOperand())->isNullValue()) {
        Value.getNode()->setArrayMarker();


        if(!isa<ArrayType>(CurTy) && Value.getNode()->getSize() <= 1){
          Value.getNode()->growSize(TD.getTypeAllocSize(CurTy));
        }
        if(CurTy->isVoidTy()) {
          Value.getNode()->growSize(1);
        }
        if(isa<ArrayType>(CurTy) && Value.getNode()->getSize() <= 1){
          const Type *ETy = (cast<ArrayType>(CurTy))->getElementType();
          while(isa<ArrayType>(ETy)) {
            ETy = (cast<ArrayType>(ETy))->getElementType();
          }
          Value.getNode()->growSize(TD.getTypeAllocSize(ETy));
          if(ETy->isVoidTy()) {
            Value.getNode()->growSize(1);
          }
        }
        if(Value.getOffset() || Offset != 0
         || (!isa<ArrayType>(CurTy)
          && (Value.getNode()->getSize() != TD.getTypeAllocSize(CurTy)))) {
          Value.getNode()->foldNodeCompletely();
          Value.getNode();
          Offset = 0;
          break;
        }
      }
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
}


void GraphBuilder::visitCallInst(CallInst &CI) {
  visitCallSite(&CI);
}

void GraphBuilder::visitInvokeInst(InvokeInst &II) {
  visitCallSite(&II);
}

void GraphBuilder::visitVAStartInst(CallSite CS) {
  // Build out DSNodes for the va_list depending on the target arch
  // And assosiate the right node with the VANode for this function
  // so it can be merged with the right arguments from callsites

  DSNodeHandle RetNH = getValueDest(*CS.arg_begin());

  if (DSNode *N = RetNH.getNode())
    visitVAStartNode(N);
}

void GraphBuilder::visitVAStartNode(DSNode* N) {
  assert(N && "Null node as argument");
  assert(FB && "No function for this graph?");
  Module *M = FB->getParent();
  assert(M && "No module for function");
  Triple TargetTriple(M->getTargetTriple());
  Triple::ArchType Arch = TargetTriple.getArch();

  // Fetch the VANode associated with the func containing the call to va_start
  DSNodeHandle & VANH = G.getVANodeFor(*FB);
  // Make sure this NodeHandle has a node to go with it
  if (VANH.isNull()) VANH.mergeWith(createNode());

  // Create a dsnode for an array of pointers to the VAInfo for this func
  DSNode * VAArray = createNode();
  VAArray->setArrayMarker();
  VAArray->foldNodeCompletely();
  VAArray->setLink(0,VANH);

  //VAStart modifies its argument
  N->setModifiedMarker();

  // For the architectures we support, build dsnodes that match
  // how we know va_list is used.
  switch (Arch) {
    case Triple::x86:
      // On x86, we have:
      // va_list as a pointer to an array of pointers to the variable arguments
      if (N->getSize() < 1)
        N->growSize(1);
      N->setLink(0, VAArray);
      break;
    case Triple::x86_64:
      // On x86_64, we have va_list as a struct {i32, i32, i8*, i8* }
      // The first i8* is where arguments generally go, but the second i8* can
      // be used also to pass arguments by register.
      // We model this by having both the i8*'s point to an array of pointers
      // to the arguments.
      if (N->getSize() < 24)
        N->growSize(24); //sizeof the va_list struct mentioned above
      N->setLink(8,VAArray); //first i8*
      N->setLink(16,VAArray); //second i8*
      break;
    default:
      // FIXME: For now we abort if we don't know how to handle this arch
      // Either add support for other architectures, or at least mark the
      // nodes unknown/incomplete or whichever results in the correct
      // conservative behavior in the general case
      assert(0 && "VAstart not supported on this architecture!");
      //XXX: This might be good enough in those cases that we don't know
      //what the arch does
      N->setIncompleteMarker()->setUnknownMarker()->foldNodeCompletely();
  }

  // XXX: We used to set the alloca marker for the DSNode passed to va_start.
  // Seems to me that you could allocate the va_list on the heap, so ignoring
  // for now.
  N->setModifiedMarker()->setVAStartMarker();
}

/// returns true if the intrinsic is handled
bool GraphBuilder::visitIntrinsic(CallSite CS, Function *F) {
  ++NumIntrinsicCall;
  switch (F->getIntrinsicID()) {
  case Intrinsic::vastart: {
    visitVAStartInst(CS);
    return true;
  }
  case Intrinsic::vacopy: {
    // Simply merge the two arguments to va_copy.
    // This results in loss of precision on the temporaries used to manipulate
    // the va_list, and so isn't a big deal.  In theory we would build a
    // separate graph for this (like the one created in visitVAStartNode)
    // and only merge the node containing the variable arguments themselves.
    DSNodeHandle destNH = getValueDest(CS.getArgument(0));
    DSNodeHandle srcNH = getValueDest(CS.getArgument(1));
    destNH.mergeWith(srcNH);
    return true;
  }
  case Intrinsic::stacksave: {
    DSNode * Node = createNode();
    Node->setAllocaMarker()->setIncompleteMarker()->setUnknownMarker();
    Node->foldNodeCompletely();
    setDestTo (*(CS.getInstruction()), Node);
    return true;
  }
  case Intrinsic::stackrestore:
    getValueDest(CS.getInstruction()).getNode()->setAllocaMarker()
                                               ->setIncompleteMarker()
                                               ->setUnknownMarker()
                                               ->foldNodeCompletely();
    return true;
  case Intrinsic::vaend:
  case Intrinsic::memory_barrier:
    return true;  // noop
  case Intrinsic::memcpy: 
  case Intrinsic::memmove: {
    // Merge the first & second arguments, and mark the memory read and
    // modified.
    DSNodeHandle RetNH = getValueDest(*CS.arg_begin());
    RetNH.mergeWith(getValueDest(*(CS.arg_begin()+1)));
    if (DSNode *N = RetNH.getNode())
      N->setModifiedMarker()->setReadMarker();
    return true;
  }
  case Intrinsic::memset:
    // Mark the memory modified.
    if (DSNode *N = getValueDest(*CS.arg_begin()).getNode())
      N->setModifiedMarker();
    return true;

  case Intrinsic::eh_exception: {
    DSNode * Node = createNode();
    Node->setIncompleteMarker();
    Node->foldNodeCompletely();
    setDestTo (*(CS.getInstruction()), Node);
    return true;
  }

  case Intrinsic::eh_selector: {
    DSNode * Node = createNode();
    Node->setIncompleteMarker();
    Node->foldNodeCompletely();
    setDestTo (*(CS.getInstruction()), Node);
    return true;
  }

  case Intrinsic::atomic_cmp_swap: {
    DSNodeHandle Ptr = getValueDest(*CS.arg_begin());
    Ptr.getNode()->setReadMarker();
    Ptr.getNode()->setModifiedMarker();
    if (isa<PointerType>(F->getReturnType())) {
      setDestTo(*(CS.getInstruction()), getValueDest(*(CS.arg_begin() + 1)));
      getValueDest(*(CS.arg_begin() + 1))
        .mergeWith(getValueDest(*(CS.arg_begin() + 2)));
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
      DSNodeHandle Ptr = getValueDest(*CS.arg_begin());
      Ptr.getNode()->setReadMarker();
      Ptr.getNode()->setModifiedMarker();
      if (isa<PointerType>(F->getReturnType()))
        setDestTo(*CS.getInstruction(), getValueDest(*(CS.arg_begin() + 1)));
    }
   
              

  case Intrinsic::prefetch:
    return true;

  case Intrinsic::objectsize:
    return true;

  //
  // The return address aliases with the stack, is type-unknown, and should
  // have the unknown flag set since we don't know where it goes.
  //
  case Intrinsic::returnaddress: {
    DSNode * Node = createNode();
    Node->setAllocaMarker()->setIncompleteMarker()->setUnknownMarker();
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

void GraphBuilder::visitCallSite(CallSite CS) {
  //
  // Get the called value.  Strip off any casts which are lossless.
  //
  Value *Callee = CS.getCalledValue()->stripPointerCasts();

  // Special case handling of certain libc allocation functions here.
  if (Function *F = dyn_cast<Function>(Callee))
    if (F->isIntrinsic() && visitIntrinsic(CS, F))
      return;

  if (visitAllocation(CS))
    return;

  //Can't do much about inline asm (yet!)
  if (isa<InlineAsm> (Callee)) {
    ++NumAsmCall;
    DSNodeHandle RetVal;
    Instruction *I = CS.getInstruction();
    if (isa<PointerType > (I->getType()))
      RetVal = getValueDest(I);

    // Calculate the arguments vector...
    for (CallSite::arg_iterator I = CS.arg_begin(), E = CS.arg_end(); I != E; ++I)
      if (isa<PointerType > ((*I)->getType()))
        RetVal.mergeWith(getValueDest(*I));
    if (!RetVal.isNull())
      RetVal.getNode()->foldNodeCompletely();
    return;
  }

  //uninteresting direct call
  if (isa<Function>(Callee) && !DSCallGraph::hasPointers(CS)) {
    ++NumBoringCall;
    return;
  }

  // Set up the return value...
  DSNodeHandle RetVal;
  Instruction *I = CS.getInstruction();
  if (isa<PointerType>(I->getType()))
    RetVal = getValueDest(I);

  DSNode *CalleeNode = 0;
  if (!isa<Function>(Callee)) {
    CalleeNode = getValueDest(Callee).getNode();
    if (CalleeNode == 0) {
      DEBUG(errs() << "WARNING: Program is calling through a null pointer?\n" << *I);
      return;  // Calling a null pointer?
    }
  }

  // NOTE: This code is identical to 'DSGraph::getDSCallSiteForCallSite',
  // the reason it's duplicated is because this calls getValueDest instead
  // of getNodeForValue to get the DSNodes for the arguments.  Since we're in
  // local it's possible that we need to create a DSNode for the argument, as
  // opposed to getNodeForValue which simply retrieves the existing node.
  
  // FIXME: refactor so we don't have this duplication. 

  //Get the FunctionType for the called function
  const FunctionType *CalleeFuncType = DSCallSite::FunctionTypeOfCallSite(CS);
  int NumFixedArgs = CalleeFuncType->getNumParams();

  // Sanity check--this really, really shouldn't happen
  if (!CalleeFuncType->isVarArg())
    assert(CS.arg_size() == static_cast<unsigned>(NumFixedArgs) &&
        "Too many arguments/incorrect function signature!");

  std::vector<DSNodeHandle> Args;
  Args.reserve(CS.arg_end()-CS.arg_begin());
  DSNodeHandle VarArgNH;

  // Calculate the arguments vector...
    // Add all fixed pointer arguments, then merge the rest together
  for (CallSite::arg_iterator I = CS.arg_begin(), E = CS.arg_end();
      I != E; ++I)
    if (isa<PointerType>((*I)->getType())) {
      DSNodeHandle ArgNode = getValueDest(*I);
      if (I - CS.arg_begin() < NumFixedArgs) {
        Args.push_back(ArgNode);
      } else {
        VarArgNH.mergeWith(ArgNode);
      }
    }

  // Add a new function call entry...
  if (CalleeNode) {
    ++NumIndirectCall;
    G.getFunctionCalls().push_back(DSCallSite(CS, RetVal, VarArgNH, CalleeNode,
                                              Args));
  } else {
    ++NumDirectCall;
    G.getFunctionCalls().push_back(DSCallSite(CS, RetVal, VarArgNH,
                                              cast<Function>(Callee),
                                              Args));
  }


}

// visitInstruction - For all other instruction types, if we have any arguments
// that are of pointer type, make them have unknown composition bits, and merge
// the nodes together.
void GraphBuilder::visitInstruction(Instruction &Inst) {
  DSNodeHandle CurNode;
  if (isa<PointerType>(Inst.getType()))
    CurNode = getValueDest(&Inst);
  for (User::op_iterator I = Inst.op_begin(), E = Inst.op_end(); I != E; ++I)
    if (isa<PointerType>((*I)->getType()))
      CurNode.mergeWith(getValueDest(*I));

  if (DSNode *N = CurNode.getNode())
    N->setUnknownMarker();
}



//===----------------------------------------------------------------------===//
// LocalDataStructures Implementation
//===----------------------------------------------------------------------===//

//
// Function: MergeConstantInitIntoNode()
//
// Description:
//  Merge the specified constant into the specified DSNode.
//
void
GraphBuilder::MergeConstantInitIntoNode(DSNodeHandle &NH,
                                        const Type* Ty,
                                        Constant *C) {
  //
  // Ensure a type-record exists...
  //
  DSNode *NHN = NH.getNode();
  //NHN->mergeTypeInfo(Ty, NH.getOffset());

  //
  // If we've found something of pointer type, create or find its DSNode and
  // make a link from the specified DSNode to the new DSNode describing the
  // pointer we've just found.
  //
  if (isa<PointerType>(Ty)) {
    NHN->setArrayMarker();
  //  NHN->mergeTypeInfo(Ty, NH.getOffset());
    NH.addEdgeTo(getValueDest(C));
    return;
  }

  //
  // If the type of the object (array element, structure field, etc.) is an
  // integer or floating point type, then just ignore it.  It has no DSNode.
  //
  if (Ty->isIntOrIntVectorTy() || Ty->isFPOrFPVectorTy()) return;

  //
  // Handle aggregate constants.
  //
  if (ConstantArray *CA = dyn_cast<ConstantArray>(C)) {
    //
    // For an array, we don't worry about different elements pointing to
    // different objects; we essentially pretend that all array elements alias.
    //
    const Type * ElementType = cast<ArrayType>(Ty)->getElementType();
    for (unsigned i = 0, e = CA->getNumOperands(); i != e; ++i) {
      Constant * ConstElement = cast<Constant>(CA->getOperand(i));
      MergeConstantInitIntoNode(NH, ElementType, ConstElement);
    }
  } else if (ConstantStruct *CS = dyn_cast<ConstantStruct>(C)) {
    //
    // For a structure, we need to merge each element of the constant structure
    // into the specified DSNode.  However, we must also handle structures that
    // end with a zero-length array ([0 x sbyte]); this is a common C idiom
    // that continues to plague the world.
    //
    //NHN->mergeTypeInfo(Ty, NH.getOffset());

    const StructLayout *SL = TD.getStructLayout(cast<StructType>(Ty));
   
     for (unsigned i = 0, e = CS->getNumOperands(); i != e; ++i) {
      DSNode *NHN = NH.getNode();
      if (SL->getElementOffset(i) < SL->getSizeInBytes()) {
        //
        // Get the type and constant value of this particular element of the
        // constant structure.
        //
        const Type * ElementType = cast<StructType>(Ty)->getElementType(i);
        Constant * ConstElement = cast<Constant>(CS->getOperand(i));

        //
        // Get the offset (in bytes) into the memory object that we're
        // analyzing.
        //
        unsigned offset = NH.getOffset()+(unsigned)SL->getElementOffset(i);
        NHN->mergeTypeInfo(ElementType, offset);
        //
        // Create a new DSNodeHandle.  This DSNodeHandle will point to the same
        // DSNode as the one we're constructing for our caller; however, it
        // will point into a different offset into that DSNode.
        //
        DSNodeHandle NewNH (NHN, offset);
        assert ((NewNH.getOffset() == offset) && "Need to resize DSNode!\n");

        //
        // Recursively merge in this element of the constant struture into the
        // DSNode.
        //
        MergeConstantInitIntoNode(NewNH, ElementType, ConstElement);
      } else if (SL->getElementOffset(i) == SL->getSizeInBytes()) {
        //
        // If this is one of those cute structures that ends with a zero-length
        // array, just fold the DSNode now and get it over with.
        //
        DEBUG(errs() << "Zero size element at end of struct\n" );
        NHN->foldNodeCompletely();
      } else {
        assert(0 && "type was smaller than offsets of struct layout indicate");
      }
    }
  } else if (isa<ConstantAggregateZero>(C) || isa<UndefValue>(C)) {
    //
    // Undefined values and NULL pointers have no DSNodes, so they do nothing.
    //
  } else {
    assert(0 && "Unknown constant type!");
  }
}

void GraphBuilder::mergeInGlobalInitializer(GlobalVariable *GV) {
  // Ensure that the global variable is not external
  assert(!GV->isDeclaration() && "Cannot merge in external global!");

  //
  // Get a node handle to the global node and merge the initializer into it.
  //
  DSNodeHandle NH = getValueDest(GV);

  //
  // Ensure that the DSNode is large enough to hold the new constant that we'll
  // be adding to it.
  //
 const Type * ElementType = GV->getType()->getElementType();
 while(const ArrayType *ATy = dyn_cast<ArrayType>(ElementType)) {
    ElementType = ATy->getElementType();
  }
  if(!NH.getNode()->isNodeCompletelyFolded()) {
    unsigned requiredSize = TD.getTypeAllocSize(ElementType) + NH.getOffset();
    if (NH.getNode()->getSize() < requiredSize){
      NH.getNode()->growSize (requiredSize);
    }
  }

  //
  // Do the actual merging in of the constant initializer.
  //
//  MergeConstantInitIntoNode(NH, ElementType, GV->getInitializer());
  MergeConstantInitIntoNode(NH, GV->getType()->getElementType(), GV->getInitializer());

}

void GraphBuilder::mergeExternalGlobal(GlobalVariable *GV) {
  // Get a node handle to the global node and merge the initializer into it.
  DSNodeHandle NH = getValueDest(GV);
}

// some evil programs use sections as linker generated arrays
// read a description of this behavior in and apply it
// format: numglobals section globals...
// terminates when numglobals == 0
void handleMagicSections(DSGraph* GlobalsGraph, Module& M) {
  std::ifstream msf(hasMagicSections.c_str(), std::ifstream::in);
  if (msf.good()) {
    //no checking happens here
    unsigned count = 0;
    msf >> count;
    while (count) {
      std::string section;
      msf >> section;
      svset<Value*> inSection;
      for (Module::iterator MI = M.begin(), ME = M.end();
              MI != ME; ++MI)
        if (MI->hasSection() && MI->getSection() == section)
          inSection.insert(MI);
      for (Module::global_iterator MI = M.global_begin(), ME = M.global_end();
              MI != ME; ++MI)
        if (MI->hasSection() && MI->getSection() == section)
          inSection.insert(MI);

      for (unsigned x = 0; x < count; ++x) {
        std::string global;
        msf >> global;
        Value* V = M.getNamedValue(global);
        if (V) {
          DSNodeHandle& DHV = GlobalsGraph->getNodeForValue(V);
          for (svset<Value*>::iterator SI = inSection.begin(),
                  SE = inSection.end(); SI != SE; ++SI) {
            DEBUG(errs() << "Merging " << V->getNameStr() << " with "
                    << (*SI)->getNameStr() << "\n");
            GlobalsGraph->getNodeForValue(*SI).mergeWith(DHV);
          }
          //DHV.getNode()->dump();
        }
      }
      msf >> count;
    }
  } else {
    errs() << "Failed to open magic sections file:" << hasMagicSections <<
            "\n";
  }
}

char LocalDataStructures::ID;

bool LocalDataStructures::runOnModule(Module &M) {
  init(&getAnalysis<TargetData>());

  // First step, build the globals graph.
  {
    GraphBuilder GGB(*GlobalsGraph);

    // Add initializers for all of the globals to the globals graph.
    for (Module::global_iterator I = M.global_begin(), E = M.global_end();
         I != E; ++I)
      if (!(I->hasSection() && I->getSection() == "llvm.metadata")) {
        if (I->isDeclaration())
          GGB.mergeExternalGlobal(I);
        else
          GGB.mergeInGlobalInitializer(I);
      }
    // Add Functions to the globals graph.
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
      if (I->hasAddressTaken())
        GGB.mergeFunction(I);
  }
  
  if (hasMagicSections.size())
    handleMagicSections(GlobalsGraph, M);

  // Next step, iterate through the nodes in the globals graph, unioning
  // together the globals into equivalence classes.
  formGlobalECs();

  // Calculate all of the graphs...
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration()) {
      DSGraph* G = new DSGraph(GlobalECs, getTargetData(), *TypeSS, GlobalsGraph);
      GraphBuilder GGB(*I, *G, *this);
      G->getAuxFunctionCalls() = G->getFunctionCalls();
      setDSGraph(*I, G);
      propagateUnknownFlag(G);
      callgraph.insureEntry(I);
      G->buildCallGraph(callgraph, true);
    }

  GlobalsGraph->removeTriviallyDeadNodes();
  GlobalsGraph->markIncompleteNodes(DSGraph::MarkFormalArgs);
  GlobalsGraph->computeExternalFlags(DSGraph::ProcessCallSites);

  // Now that we've computed all of the graphs, and merged all of the info into
  // the globals graph, see if we have further constrained the globals in the
  // program if so, update GlobalECs and remove the extraneous globals from the
  // program.
  formGlobalECs();

  propagateUnknownFlag(GlobalsGraph);
  return false;
}

