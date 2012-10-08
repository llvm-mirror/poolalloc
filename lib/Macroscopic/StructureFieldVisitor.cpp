//===-- StructureFieldVisitor.cpp - Implement StructureFieldVisitor -------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements the StructureFieldVisitor and related classes.
//
//===----------------------------------------------------------------------===//

#include "StructureFieldVisitor.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Analysis/DataStructure/DataStructure.h"
#include "llvm/Analysis/DataStructure/DSGraph.h"
#include "llvm/Intrinsics.h"
#include "llvm/Support/InstVisitor.h"
#include <algorithm>
#include <iostream>

using namespace llvm::Macroscopic;
using namespace llvm;

/// FindAllDataStructures - Inspect the program specified by ECG, adding to
/// 'Nodes' all of the data structures node in the program that contain the
/// "IncludeFlags" and do not contain "ExcludeFlags" node flags.  If
/// OnlyHomogenous is true, only type-homogenous nodes are considered.
void FindAllDataStructures(std::set<DSNode*> &Nodes, unsigned IncludeFlags,
                           unsigned ExcludeFlags, bool OnlyHomogenous,
                           EquivClassGraphs &ECG) {
  // Loop over all of the graphs in ECG, finding nodes that are not incomplete
  // and do not have any of the flags specified by Flags.
  ExcludeFlags |= DSNode::Incomplete;

  /// FIXME: nodes in the global graph should not be marked incomplete in main!!
  for (hash_map<const Function*, DSGraph*>::iterator GI = ECG.DSInfo.begin(),
         E = ECG.DSInfo.end(); GI != E; ++GI) {
    assert(GI->second && "Null graph pointer?");
    DSGraph &G = *GI->second;
    for (DSGraph::node_iterator I = G.node_begin(), E = G.node_end();
         I != E; ++I)
      // If this node matches our constraints, include it.
      if ((I->getNodeFlags() & IncludeFlags) == IncludeFlags &&
          (I->getNodeFlags() & ExcludeFlags) == 0)
        if (!OnlyHomogenous || !I->isNodeCompletelyFolded())
          Nodes.insert(I);
  }
}

//===----------------------------------------------------------------------===//
// LatticeValue class implementation
//

DSGraph &LatticeValue::getParentGraph() const {
  DSGraph *PG = getNode()->getParentGraph();
  assert(PG && "Node doesn't have a parent, is it a GlobalGraph node?");
  return *PG;
}

/// getFieldOffset - Return the offset of this field from the start of the node.
///
unsigned LatticeValue::getFieldOffset() const {
  const DataLayout &TD = getNode()->getParentGraph()->getDataLayout();

  unsigned Offset = 0;
  const Type *Ty = Node->getType();
  for (unsigned i = 0, e = Idxs.size(); i != e; ++i)
    if (const StructType *STy = dyn_cast<StructType>(Ty)) {
      const StructLayout *SL = TD.getStructLayout(STy);
      Offset += SL->MemberOffsets[Idxs[i]];
      Ty = STy->getElementType(Idxs[i]);
    } else {
      const SequentialType *STy = cast<SequentialType>(Ty);
      Ty = STy->getElementType();
      --i;   // This doesn't index a struct.
    }
  return Offset;
}



void LatticeValue::dump() const {
  std::cerr << "\nFunction: " << Node->getParentGraph()->getFunctionNames()
            << "\n";
  Node->dump();
  if (!Idxs.empty()) {
    std::cerr << "Field: ";
    for (unsigned i = 0, e = Idxs.size(); i != e; ++i)
      std::cerr << Idxs[i] << ".";
  }
  std::cerr << "\n";
}

bool LatticeValue::visitLoad(LoadInst &) {
  std::cerr << "ERROR: Client requested load visitation, but did not "
            << "overload visitLoad!\n";
  dump();
  return true;
}
bool LatticeValue::visitStore(StoreInst &) {
  std::cerr << "ERROR: Client requested store visitation, but did not "
            << "overload visitStore!\n";
  dump();
  return true;
}

bool LatticeValue::visitGlobalInit(Constant *) {
  std::cerr << "ERROR: Client requested store visitation, but did not "
            << "overload visitGlobalInit!\n";
  dump();
  return true;
}

//===----------------------------------------------------------------------===//
// SFVInstVisitor class implementation
//

namespace {
  /// SFVInstVisitor - This visitor is used to do the actual visitation of
  /// memory instructions in the program.
  ///
  struct SFVInstVisitor : public InstVisitor<SFVInstVisitor, bool> {
    DSGraph &DSG;
    const unsigned Callbacks;
    std::multimap<DSNode*, LatticeValue*> &NodeLVs;

    // DirectCallSites - When we see call sites, we don't process them, but we
    // do remember them if they are direct calls.
    std::set<Instruction*> DirectCallSites;

    SFVInstVisitor(DSGraph &dsg, unsigned CBs,
                   std::multimap<DSNode*, LatticeValue*> &LVs)
      : DSG(dsg), Callbacks(CBs), NodeLVs(LVs) {}

    // Methods used by visitation methods.
    LatticeValue *getLatticeValueForField(Value *Ptr);
    bool RemoveLatticeValueAtBottom(LatticeValue *LV);

    /// Visitation methods - These methods should return true when a lattice
    /// value is driven to 'bottom' and removed from NodeLVs.
    bool visitLoadInst(LoadInst &LI);
    bool visitStoreInst(StoreInst &SI);

    /// These are not implemented yet.
    bool visitGetElementPtrInst(GetElementPtrInst &GEPI) { return false; }
    bool visitMallocInst(MallocInst &MI) { return false; }
    bool visitAllocaInst(AllocaInst &AI) { return false; }
    bool visitFreeInst(FreeInst &FI) { return false; }
    bool visitPHINode(PHINode &PN) { return false; }
    bool visitSelectInst(SelectInst &SI) { return false; }
    bool visitSetCondInst(SetCondInst &SCI) { return false; }
    bool visitCastInst(CastInst &CI) { return false; }
    bool visitReturnInst(ReturnInst &RI) { return false; }
    
    // visitCallInst/visitInvokeInst - Call and invoke are handled specially, so
    // they are just noops here.
    bool visitCallInst(CallInst &CI) { return visitCallSite(CI); }
    bool visitInvokeInst(InvokeInst &II) { return visitCallSite(II); }
    bool visitCallSite(Instruction &I) {
      // Remember direct function calls.
      if (isa<Function>(I.getOperand(0))) DirectCallSites.insert(&I);
      return false;
    }

    bool visitInstruction(Instruction &I) {
#ifndef NDEBUG
      // Check to make sure this instruction really isn't using anything we the
      // client needs to know about.
      assert(getLatticeValueForField(&I) == 0 && "Inst should be handled!");
      for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i)
        assert(getLatticeValueForField(I.getOperand(i)) == 0 &&
               "Inst should be handled by visitor!");
#endif
      return false;
    }
  };
}

static void ComputeStructureFieldIndices(const Type *Ty, unsigned Offset,
                                         std::vector<unsigned> &Idxs,
                                         const DataLayout &TD) {
  if (Ty->isFirstClassType()) {
    assert(Offset == 0 && "Illegal structure index!");
    return;
  }

  if (const SequentialType *STy = dyn_cast<SequentialType>(Ty)) {
    ComputeStructureFieldIndices(STy->getElementType(), Offset, Idxs, TD);
  } else if (const StructType *STy = dyn_cast<StructType>(Ty)) {
    const StructLayout *SL = TD.getStructLayout(STy);
    
    std::vector<uint64_t>::const_iterator SI =
      std::upper_bound(SL->MemberOffsets.begin(), SL->MemberOffsets.end(),
                       Offset);
    assert(SI != SL->MemberOffsets.begin() && "Offset not in structure type!");
    --SI;
    assert(*SI <= Offset && "upper_bound didn't work");
    assert((SI == SL->MemberOffsets.begin() || *(SI-1) < Offset) &&
           (SI+1 == SL->MemberOffsets.end() || *(SI+1) > Offset) &&
           "Upper bound didn't work!");
    Offset -= *SI;   // Skip over the offset to this structure field.
    unsigned Idx = SI - SL->MemberOffsets.begin();
    assert(Idx < STy->getNumElements() && "Illegal structure index");
    Idxs.push_back(Idx);
    ComputeStructureFieldIndices(STy->getElementType(Idx), Offset, Idxs, TD);
  } else {
    assert(0 && "Unknown type to index into!");
  }  
}

LatticeValue *SFVInstVisitor::getLatticeValueForField(Value *Ptr) {
  if (!isa<PointerType>(Ptr->getType()) ||
      isa<ConstantPointerNull>(Ptr)) return 0;

  const DSNodeHandle *NH = &DSG.getNodeForValue(Ptr);
  DSNode *Node = NH->getNode();
  assert(Node && "Pointer doesn't have node??");

  std::multimap<DSNode*, LatticeValue*>::iterator I = NodeLVs.find(Node);
  if (I == NodeLVs.end()) return 0;  // Not a node we are still tracking.

  // Okay, next convert the node offset to a field index expression.
  std::vector<unsigned> Idxs;
  ComputeStructureFieldIndices(Node->getType(), NH->getOffset(), Idxs,
                               Node->getParentGraph()->getDataLayout());

  for (; I != NodeLVs.end() && I->first == Node; ++I)
    if (I->second->getIndices() == Idxs)
      return I->second;
  return 0;
}

/// RemoveLatticeValueAtBottom - When analysis determines that LV hit bottom,
/// this method is used to remove it from the NodeLVs map.  This method always
/// returns true to simplify caller code.
///
bool SFVInstVisitor::RemoveLatticeValueAtBottom(LatticeValue *LV) {
  for (std::multimap<DSNode*, LatticeValue*>::iterator I
         = NodeLVs.find(LV->getNode());; ++I) {
    assert(I != NodeLVs.end() && "Lattice value not in map??");
    if (I->second == LV) {
      NodeLVs.erase(I);
      return true;
    }
  }
}


bool SFVInstVisitor::visitLoadInst(LoadInst &LI) {
  if ((Callbacks & Visit::Loads) == 0) return false;

  if (LatticeValue *LV = getLatticeValueForField(LI.getOperand(0)))
    if (LV->visitLoad(LI))
      return RemoveLatticeValueAtBottom(LV);

  return false;
}

bool SFVInstVisitor::visitStoreInst(StoreInst &SI) {
  if ((Callbacks & Visit::Stores) == 0) return false;

  if (LatticeValue *LV = getLatticeValueForField(SI.getOperand(1)))
    if (LV->visitStore(SI))
      return RemoveLatticeValueAtBottom(LV);

  return false;
}


//===----------------------------------------------------------------------===//
// StructureFieldVisitorBase class implementation
//

void StructureFieldVisitorBase::
AddLatticeValuesForFields(DSNode *Node, const Type *Ty,
                          const std::vector<unsigned> &Idxs,
                          std::set<LatticeValue*> &Values) {
  if (Ty->isFirstClassType()) {
    if (LatticeValue *LV = createLatticeValue(Node, Idxs, Ty))
      Values.insert(LV);
    return;
  }

  const StructType *STy = dyn_cast<StructType>(Ty);
  if (STy == 0) return;  // Not handling arrays yet!

  std::vector<unsigned> NextIdxs(Idxs);
  NextIdxs.push_back(0);
  for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
    NextIdxs.back() = i;
    AddLatticeValuesForFields(Node, STy->getElementType(i), NextIdxs, Values);
  }
}

void StructureFieldVisitorBase::
AddLatticeValuesForNode(DSNode *N, std::set<LatticeValue*> &Values) {
  if (N->isNodeCompletelyFolded() ||
      N->getType() == Type::VoidTy) return;  // Can't analyze node.
  std::vector<unsigned> Idxs;
  AddLatticeValuesForFields(N, N->getType(), Idxs, Values);
}


/// visitNodes - This is a simple wrapper around visitFields that creates a
/// lattice value for every field in the specified collection of nodes.
///
std::set<LatticeValue*> StructureFieldVisitorBase::
visitNodes(const std::set<DSNode*> &Nodes) {
  std::set<LatticeValue*> Result;

  // Create lattice values for all of the fields in these nodes.
  for (std::set<DSNode*>::const_iterator I = Nodes.begin(), E = Nodes.end();
       I != E; ++I)
    AddLatticeValuesForNode(*I, Result);

  // Now that we have the lattice values, just use visitFields to do the grunt
  // work.
  visitFields(Result);
  return Result;
}


/// VisitGlobalInit - The specified lattice value corresponds to a field (or
/// several) in the specified global.  Merge all of the overlapping initializer
/// values into LV (up-to and until it becomes overdefined).
///
static bool VisitGlobalInit(LatticeValue *LV, Constant *Init,
                            unsigned FieldOffset) {
  const DataLayout &TD = LV->getParentGraph().getDataLayout();

  if (Init->isNullValue())
    return LV->visitGlobalInit(Constant::getNullValue(LV->getFieldType()));
  if (isa<UndefValue>(Init))
    return LV->visitGlobalInit(UndefValue::get(LV->getFieldType()));

  if (LV->getNode()->isArray() &&
      TD.getTypeSize(Init->getType()) > LV->getNode()->getSize()) {
    ConstantArray *CA = cast<ConstantArray>(Init);
    for (unsigned i = 0, e = CA->getNumOperands(); i != e; ++i)
      if (VisitGlobalInit(LV, CA->getOperand(i), FieldOffset))
        return true;
    return false;
  }

NextStep:  // Manual tail recursion

  if (Init->isNullValue())
    return LV->visitGlobalInit(Constant::getNullValue(LV->getFieldType()));
  if (isa<UndefValue>(Init))
    return LV->visitGlobalInit(UndefValue::get(LV->getFieldType()));
  if (Init->getType()->isFirstClassType()) {
    assert(FieldOffset == 0 && "GV Init mismatch!");
    return LV->visitGlobalInit(Init);
  }
  
  if (const StructType *STy = dyn_cast<StructType>(Init->getType())) {
    const StructLayout *SL = TD.getStructLayout(STy);
    unsigned Field = SL->getElementContainingOffset(FieldOffset);
    FieldOffset -= SL->MemberOffsets[Field];
    Init = cast<ConstantStruct>(Init)->getOperand(Field);
    goto NextStep;
  } else if (const ArrayType *ATy = dyn_cast<ArrayType>(Init->getType())) {
    unsigned ElSz = TD.getTypeSize(ATy->getElementType());
    unsigned Idx = FieldOffset / ElSz;
    FieldOffset -= Idx*ElSz;
    Init = cast<ConstantArray>(Init)->getOperand(Idx);
    goto NextStep;
  } else {
    assert(0 && "Unexpected initializer type!");
    return true;
  }
}


/// visitFields - Visit all uses of the specified fields, updating the lattice
/// values as appropriate.
///
void StructureFieldVisitorBase::visitFields(std::set<LatticeValue*> &Fields) {
  // Now that we added all of the initial nodes, find out what graphs these
  // nodes are rooted in.  For efficiency, handle batches of nodes in the same
  // function together at the same time.  To do this, pull everything out of
  // Fields and put it into FieldsByGraph.
  std::multimap<DSGraph*, LatticeValue*> FieldsByGraph;
  while (!Fields.empty()) {
    LatticeValue *LV = *Fields.begin();
    Fields.erase(Fields.begin());
    FieldsByGraph.insert(std::make_pair(&LV->getParentGraph(), LV));
  }

  // Now that everything is grouped by graph, process a graph worth of nodes at
  // a time, moving the results back to Fields if they are not overdefined.
  while (!FieldsByGraph.empty()) {
    // NodeLVs - Inspect all of the lattice values that are active in this
    // graph.
    std::multimap<DSNode*, LatticeValue*> NodeLVs;
    DSGraph &DSG = *FieldsByGraph.begin()->first;

    // Pull out all nodes in this graph, putting them into NodeLVs.
    while (FieldsByGraph.begin()->first == &DSG) {
      LatticeValue *LV = FieldsByGraph.begin()->second;
      FieldsByGraph.erase(FieldsByGraph.begin());
      NodeLVs.insert(std::make_pair(LV->getNode(), LV));
    }

    // Visit all operations in this graph, looking at how these lattice values
    // are used!
    visitGraph(DSG, NodeLVs);

    if (NodeLVs.empty()) continue;

    // If any of the nodes have globals, and if we are inspecting stores, make
    // sure to notice the global variable initializers.
    if (Callbacks & Visit::Stores) {
      for (std::multimap<DSNode*, LatticeValue*>::iterator I = NodeLVs.begin();
           I != NodeLVs.end(); ) {
        // If this node has globals, incorporate them.
        bool LVisOverdefined = false;
        std::vector<GlobalValue*> GVs;
        I->first->addFullGlobalsList(GVs);
        for (unsigned i = 0, e = GVs.size(); i != e; ++i)
          if (GlobalVariable *GV = dyn_cast<GlobalVariable>(GVs[i])) {
            assert(GV->hasInitializer() && "Should not be analyzing this GV!");
            if (VisitGlobalInit(I->second, GV->getInitializer(),
                                I->second->getFieldOffset())) {
              LVisOverdefined = true;
              break;
            }
          }

        if (LVisOverdefined)
          NodeLVs.erase(I++);
        else
          ++I;
      }
      if (NodeLVs.empty()) continue;
    }

    // Okay, final check.  If we have any dataflow facts about nodes that are
    // reachable from global variable nodes, we must make sure to check every
    // function in the program that uses the global for uses of the node.
    // Because of the globals graph, we might not have all of the information we
    // need with our (potentially) truncated call graph traversal.
    ProcessNodesReachableFromGlobals(DSG, NodeLVs);

    // Everything that survived goes into Fields now that we processed it.
    while (!NodeLVs.empty()) {
      Fields.insert(NodeLVs.begin()->second);
      NodeLVs.erase(NodeLVs.begin());
    }
  }
}

/// ComputeInverseGraphFrom - Perform a simple depth-first search of the
/// DSGraph, recording the inverse of all of the edges in the graph.
///
static void ComputeInverseGraphFrom(DSNode *N,
                         std::set<std::pair<DSNode*,DSNode*> > &InverseGraph) {
  for (DSNode::edge_iterator I = N->edge_begin(), E = N->edge_end(); I != E;++I)
    if (DSNode *Target = I->getNode())
      if (InverseGraph.insert(std::make_pair(Target, N)).second)
        ComputeInverseGraphFrom(Target, InverseGraph);
}


/// ComputeNodesReacahbleFrom - Compute the set of nodes in the specified
/// inverse graph that are reachable from N.  This is a simple depth first
/// search.
///
static void ComputeNodesReachableFrom(DSNode *N,
                            std::set<std::pair<DSNode*,DSNode*> > &InverseGraph,
                                      hash_set<const DSNode*> &Reachable) {
  if (!Reachable.insert(N).second) return;  // Already visited!
  
  std::set<std::pair<DSNode*,DSNode*> >::iterator I = 
    InverseGraph.lower_bound(std::make_pair(N, (DSNode*)0));
  for (; I != InverseGraph.end() && I->first == N; ++I)
    ComputeNodesReachableFrom(I->second, InverseGraph, Reachable);
}

/// ProcessNodesReachableFromGlobals - If we inferred anything about nodes
/// reachable from globals, we have to make sure that we incorporate data for
/// all graphs that include those globals due to the nature of the globals
/// graph.
///
void StructureFieldVisitorBase::
ProcessNodesReachableFromGlobals(DSGraph &DSG,
                                 std::multimap<DSNode*,LatticeValue*> &NodeLVs){
  // Start by marking all nodes reachable from globals.
  DSScalarMap &SM = DSG.getScalarMap();
  if (SM.global_begin() == SM.global_end()) return;

  hash_set<const DSNode*> Reachable;
  for (DSScalarMap::global_iterator GI = SM.global_begin(),
         E = SM.global_end(); GI != E; ++GI)
    SM[*GI].getNode()->markReachableNodes(Reachable);
  if (Reachable.empty()) return;
  
  // If any of the nodes with dataflow facts are reachable from the globals
  // graph, we have to do the GG processing step.
  bool MustProcessThroughGlobalsGraph = false;
  for (std::multimap<DSNode*, LatticeValue*>::iterator I = NodeLVs.begin(),
         E = NodeLVs.end(); I != E; ++I)
    if (Reachable.count(I->first)) {
      MustProcessThroughGlobalsGraph = true;
      break;
    }
  
  if (!MustProcessThroughGlobalsGraph) return;
  Reachable.clear();

  // Compute the mapping from DSG to the globals graph.
  DSGraph::NodeMapTy DSGToGGMap;
  DSG.computeGToGGMapping(DSGToGGMap);

  // Most of the times when we find facts about things reachable from globals we
  // we are in the main graph.  This means that we have *all* of the globals
  // graph in this DSG.  To be efficient, we compute the minimum set of globals
  // that can reach any of the NodeLVs facts.
  //
  // I'm not aware of any wonderful way of computing the set of globals that
  // points to the set of nodes in NodeLVs that is not N^2 in either NodeLVs or
  // the number of globals, except to compute the inverse of DSG.  As such, we
  // compute the inverse graph of DSG, which basically has the edges going from
  // pointed to nodes to pointing nodes.  Because we only care about one
  // connectedness properties, we ignore field info.  In addition, we only
  // compute inverse of the portion of the graph reachable from the globals.
  std::set<std::pair<DSNode*,DSNode*> > InverseGraph;

  for (DSScalarMap::global_iterator GI = SM.global_begin(),
         E = SM.global_end(); GI != E; ++GI)
    ComputeInverseGraphFrom(SM[*GI].getNode(), InverseGraph);

  // Okay, now that we have our bastardized inverse graph, compute the set of
  // globals nodes reachable from our lattice nodes.
  for (std::multimap<DSNode*, LatticeValue*>::iterator I = NodeLVs.begin(),
         E = NodeLVs.end(); I != E; ++I)
    ComputeNodesReachableFrom(I->first, InverseGraph, Reachable);
 
  // Now that we know which nodes point to the data flow facts, figure out which
  // globals point to the data flow facts.
  std::set<GlobalValue*> Globals;
  for (hash_set<const DSNode*>::iterator I = Reachable.begin(),
         E = Reachable.end(); I != E; ++I)
    Globals.insert((*I)->globals_begin(), (*I)->globals_end());

  // Finally, loop over all of the DSGraphs for the program, computing
  // information for the graph if not done already, mapping the result into our
  // context.
  for (hash_map<const Function*, DSGraph*>::iterator GI = ECG.DSInfo.begin(),
         E = ECG.DSInfo.end(); GI != E; ++GI) {
    DSGraph &FG = *GI->second;
    // Graphs can contain multiple functions, only process the graph once.
    if (GI->first != FG.retnodes_begin()->first ||
        // Also, do not bother reprocessing DSG.
        &FG == &DSG)
      continue;

    bool GraphUsesGlobal = false;
    for (std::set<GlobalValue*>::iterator I = Globals.begin(),
           E = Globals.end(); I != E; ++I)
      if (FG.getScalarMap().count(*I)) {
        GraphUsesGlobal = true;
        break;
      }

    // If this graph does not contain the global at all, there is no reason to
    // even think about it.
    if (!GraphUsesGlobal) continue;

    // Otherwise, compute the full set of dataflow effects of the function.
    std::multimap<DSNode*, LatticeValue*> &FGF = getCalleeFacts(FG);
    //std::cerr << "Computed: " << FG.getFunctionNames() << "\n";

#if 0
    for (std::multimap<DSNode*, LatticeValue*>::iterator I = FGF.begin(),
           E = FGF.end(); I != E; ++I)
      I->second->dump();
#endif
    // Compute the mapping of nodes in the globals graph to the function's
    // graph.  Note that this function graph may not have nodes (or may have
    // fragments of full nodes) in the globals graph, and we don't want this to
    // pessimize the analysis.
    std::multimap<const DSNode*, std::pair<DSNode*,int> > GraphMap;
    DSGraph::NodeMapTy GraphToGGMap;
    FG.computeGToGGMapping(GraphToGGMap);

    // "Invert" the mapping.  We compute the mapping from the start of a global
    // graph node to a place in the graph's node.  Note that not all of the GG
    // node may be present in the graphs node, so there may be a negative offset
    // involved.
    while (!GraphToGGMap.empty()) {
      DSNode *GN = const_cast<DSNode*>(GraphToGGMap.begin()->first);
      DSNodeHandle &GGNH = GraphToGGMap.begin()->second;
      GraphMap.insert(std::make_pair(GGNH.getNode(),
                                     std::make_pair(GN, -GGNH.getOffset())));
      GraphToGGMap.erase(GraphToGGMap.begin());
    }

    // Loop over all of the dataflow facts that we have computed, mapping them
    // to the globals graph.
    for (std::multimap<DSNode*, LatticeValue*>::iterator I = NodeLVs.begin(),
           E = NodeLVs.end(); I != E; ) {
      bool FactHitBottom = false;

      //I->second->dump();

      assert(I->first->getParentGraph() == &DSG);
      assert(I->second->getNode()->getParentGraph() == &DSG);

      // Node is in the GG?
      DSGraph::NodeMapTy::iterator DSGToGGMapI = DSGToGGMap.find(I->first);
      if (DSGToGGMapI != DSGToGGMap.end()) {
        DSNodeHandle &GGNH = DSGToGGMapI->second;
        const DSNode *GGNode = GGNH.getNode();
        unsigned DSGToGGOffset = GGNH.getOffset();

        // See if there is a node in FG that corresponds to this one.  If not,
        // no information will be computed in this scope, as the memory is not
        // accessed.
        std::multimap<const DSNode*, std::pair<DSNode*,int> >::iterator GMI =
          GraphMap.find(GGNode);

        // LatticeValOffset - The offset from the start of the GG Node to the
        // start of the field we are interested in.
        unsigned LatticeValOffset = I->second->getFieldOffset()+DSGToGGOffset;

        // Loop over all of the nodes in FG that correspond to this single node
        // in the GG.
        for (; GMI != GraphMap.end() && GMI->first == GGNode; ++GMI) {
          // Compute the offset to the field in the user graph.
          unsigned FieldOffset = LatticeValOffset - GMI->second.second;

          // If the field is within the amount of memory accessed by this scope,
          // then there must be a corresponding lattice value.
          DSNode *FGNode = GMI->second.first;
          if (FieldOffset < FGNode->getSize()) {
            LatticeValue *CorrespondingLV = 0;

            std::multimap<DSNode*, LatticeValue*>::iterator FGFI =
              FGF.find(FGNode);
            for (; FGFI != FGF.end() && FGFI->first == FGNode; ++FGFI)
              if (FGFI->second->getFieldOffset() == FieldOffset) {
                CorrespondingLV = FGFI->second;
                break;
              }

            // Finally, if either there was no corresponding fact (because it
            // hit bottom in this scope), or if merging the two pieces of
            // information makes it hit bottom, remember this.
            if (CorrespondingLV == 0 ||
                I->second->mergeInValue(CorrespondingLV))
              FactHitBottom = true;
          }
        }
      }

      if (FactHitBottom) {
        delete I->second;
        NodeLVs.erase(I++);
        if (NodeLVs.empty()) return;
      } else {
        ++I;
      }
    }
  }
}


/// getCalleeFacts - Compute the data flow effect that calling one of the
/// functions in this graph has on the caller.  This information is cached after
/// it is computed for a function the first time.
///
std::multimap<DSNode*, LatticeValue*> &
StructureFieldVisitorBase::getCalleeFacts(DSGraph &DSG) {
  // Already computed?
  std::map<DSGraph*, std::multimap<DSNode*,LatticeValue*> >::iterator
    I = CalleeFnFacts.find(&DSG);
  if (I != CalleeFnFacts.end())
    return I->second;   // We already computed stuff for this fn!
  
  std::multimap<DSNode*, LatticeValue*> &Result = CalleeFnFacts[&DSG];

  std::set<LatticeValue*> LVs;
  for (DSGraph::node_iterator NI = DSG.node_begin(), E = DSG.node_end();
       NI != E; ++NI)
    AddLatticeValuesForNode(NI, LVs);

  while (!LVs.empty()) {
    Result.insert(std::make_pair((*LVs.begin())->getNode(), *LVs.begin()));
    LVs.erase(LVs.begin());
  }

  if (!Result.empty())
    visitGraph(DSG, Result);
  return Result;
}


/// NodeCanPossiblyBeInteresting - Return true if the specified node can
/// potentially be interesting to a client that is only interested in
/// VisitFlags events.  This is used to reduce the cost of interprocedural
/// analysis by not traversing the call graph through portions that the DSGraph
/// can answer immediately.
///
static bool NodeCanPossiblyBeInteresting(const DSNode *N, unsigned VisitFlags) {
  // No fields are accessed in this context.
  if (N->getType() == Type::VoidTy) return false;
  
  // This node is possibly interesting if we are looking for reads and it is
  // read, we're looking for writes and it is modified, etc.
  if ((VisitFlags & Visit::Loads) && N->isRead()) return true;
  if ((VisitFlags & Visit::Stores) && N->isModified()) return true;

  assert((VisitFlags & ~(Visit::Loads|Visit::Stores)) == 0 &&
         "Unknown visitation type!");

  // Otherwise, this node is not interesting to the current client.
  return false;
}

/// visitGraph - Visit the functions in the specified graph, updating the
/// specified lattice values for all of their uses.
///
void StructureFieldVisitorBase::
visitGraph(DSGraph &DSG, std::multimap<DSNode*, LatticeValue*> &NodeLVs) {
  assert(!NodeLVs.empty() && "No lattice values to compute!");

  // To visit a graph, first step, we visit the instruction making up each
  // function in the graph, but ignore calls when processing them.  We handle
  // call nodes explicitly by looking at call nodes in the graph if needed.  We
  // handle instructions before calls to avoid interprocedural analysis if we
  // can drive lattice values to bottom early.
  //
  SFVInstVisitor IV(DSG, Callbacks, NodeLVs);

  for (DSGraph::retnodes_iterator FI = DSG.retnodes_begin(),
         E = DSG.retnodes_end(); FI != E; ++FI)
    for (Function::iterator BB = FI->first->begin(), E = FI->first->end();
         BB != E; ++BB)
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
        if (IV.visit(*I) && NodeLVs.empty())
          return;  // Nothing left to analyze.

  // Keep track of which actual direct callees are handled.
  std::set<Function*> CalleesHandled;

  // Once we have visited all of the instructions in the function bodies, if
  // there are lattice values that have not been driven to bottom, see if any of
  // the nodes involved are passed into function calls.  If so, we potentially
  // have to recursively traverse the call graph.
  for (DSGraph::fc_iterator CS = DSG.fc_begin(), E = DSG.fc_end();
       CS != E; ++CS) {
    // Figure out the mapping from a node in the caller (potentially several)
    // nodes in the callee.
    DSGraph::NodeMapTy CallNodeMap;

    Instruction *TheCall = CS->getCallSite().getInstruction();

    // If this is an indirect function call, assume nothing gets passed through
    // it. FIXME: THIS IS BROKEN!  Just get the ECG for the fn ptr if it's not
    // direct.
    if (CS->isIndirectCall())
      continue;

    // If this is an external function call, it cannot be involved with this
    // node, because otherwise the node would be marked incomplete!
    if (CS->getCalleeFunc()->isExternal())
      continue;

    // If we can handle this function call, remove it from the set of direct
    // calls found by the visitor.
    CalleesHandled.insert(CS->getCalleeFunc());

    std::vector<DSNodeHandle> Args;

    DSGraph *CG = &ECG.getDSGraph(*CS->getCalleeFunc());
    CG->getFunctionArgumentsForCall(CS->getCalleeFunc(), Args);

    if (!CS->getRetVal().isNull())
      DSGraph::computeNodeMapping(Args[0], CS->getRetVal(), CallNodeMap);
    for (unsigned i = 0, e = CS->getNumPtrArgs(); i != e; ++i) {
      if (i == Args.size()-1) break;
      DSGraph::computeNodeMapping(Args[i+1], CS->getPtrArg(i), CallNodeMap);
    }
    Args.clear();

    // The mapping we just computed maps from nodes in the callee to nodes in
    // the caller, so we can't query it efficiently.  Instead of going through
    // the trouble of inverting the map to do this (linear time with the size of
    // the mapping), we just do a linear search to see if any affected nodes are
    // passed into this call.
    bool CallCanModifyDataFlow = false;
    for (DSGraph::NodeMapTy::iterator MI = CallNodeMap.begin(),
           E = CallNodeMap.end(); MI != E; ++MI)
      if (NodeLVs.count(MI->second.getNode()))
        // Okay, the node is passed in, check to see if the call might do
        // something interesting to it (i.e. if analyzing the call can produce
        // anything other than "top").
        if ((CallCanModifyDataFlow = NodeCanPossiblyBeInteresting(MI->first,
                                                                  Callbacks)))
          break;

    // If this function call cannot impact the analysis (either because the
    // nodes we are tracking are not passed into the call, or the DSGraph for
    // the callee tells us that analysis of the callee can't provide interesting
    // information), ignore it.
    if (!CallCanModifyDataFlow)
      continue;

    // Okay, either compute analysis results for the callee function, or reuse
    // results previously computed.
    std::multimap<DSNode*, LatticeValue*> &CalleeFacts = getCalleeFacts(*CG);

    // Merge all of the facts for the callee into the facts for the caller.  If
    // this reduces anything in the caller to 'bottom', remove them.
    for (DSGraph::NodeMapTy::iterator MI = CallNodeMap.begin(),
           E = CallNodeMap.end(); MI != E; ++MI) {
      // If we have Lattice facts in the caller for this node in the callee,
      // merge any information from the callee into the caller.

      // If the node is not accessed in the callee at all, don't update.
      if (MI->first->getType() == Type::VoidTy)
        continue;

      // If there are no data-flow facts live in the caller for this node, don't
      // both processing it.
      std::multimap<DSNode*, LatticeValue*>::iterator NLVI =
        NodeLVs.find(MI->second.getNode());
      if (NLVI == NodeLVs.end()) continue;
          
          
      // Iterate over all of the lattice values that have corresponding fields
      // in the callee, merging in information as we go.  Be careful about the
      // fact that the callee may get passed the address of a substructure and
      // other funny games.
      //if (CalleeFacts.count(const_cast<DSNode*>(MI->first)) == 0) {

      DSNode *CalleeNode = const_cast<DSNode*>(MI->first);

      unsigned CalleeNodeOffset = MI->second.getOffset();
      while (NLVI->first == MI->second.getNode()) {
        // Figure out what offset in the callee this field would land.
        unsigned FieldOff = NLVI->second->getFieldOffset()+CalleeNodeOffset;

        // If the field is not within the callee node, ignore it.
        if (FieldOff >= CalleeNode->getSize()) {
          ++NLVI;
          continue;
        }

        // Okay, check to see if we have a lattice value for the field at offset
        // FieldOff in the callee node.
        const LatticeValue *CalleeLV = 0;

        std::multimap<DSNode*, LatticeValue*>::iterator CFI = 
          CalleeFacts.lower_bound(CalleeNode);
        for (; CFI != CalleeFacts.end() && CFI->first == CalleeNode; ++CFI)
          if (CFI->second->getFieldOffset() == FieldOff) {
            CalleeLV = CFI->second;   // Found it!
            break;
          }
        
        // If we don't, the lattice value hit bottom and we should remove the
        // lattice value in the caller.
        if (!CalleeLV) {
          delete NLVI->second;   // The lattice value hit bottom.
          NodeLVs.erase(NLVI++);
          continue;
        }

        // Finally, if we did find a corresponding entry, merge the information
        // into the caller's lattice value and keep going.
        if (NLVI->second->mergeInValue(CalleeLV)) {
          // Okay, merging these two caused the caller value to hit bottom.
          // Remove it.
          delete NLVI->second;   // The lattice value hit bottom.
          NodeLVs.erase(NLVI++);
        }

        ++NLVI;  // We successfully merged in some information!
      }

      // If we ran out of facts to prove, just exit.
      if (NodeLVs.empty()) return;
    }
  }

  // The local analysis pass inconveniently discards many local function calls
  // from the graph if they are to known functions.  Loop over direct function
  // calls not handled above and visit them as appropriate.
  while (!IV.DirectCallSites.empty()) {
    Instruction *Call = *IV.DirectCallSites.begin();
    IV.DirectCallSites.erase(IV.DirectCallSites.begin());

    // Is this one actually handled by DSA?
    if (CalleesHandled.count(cast<Function>(Call->getOperand(0))))
      continue;

    // Collect the pointers involved in this call.    
    std::vector<Value*> Pointers;
    if (isa<PointerType>(Call->getType()))
      Pointers.push_back(Call);
    for (unsigned i = 1, e = Call->getNumOperands(); i != e; ++i)
      if (isa<PointerType>(Call->getOperand(i)->getType()))
        Pointers.push_back(Call->getOperand(i));

    // If this is an intrinsic function call, figure out which one.
    unsigned IID = cast<Function>(Call->getOperand(0))->getIntrinsicID();

    for (unsigned i = 0, e = Pointers.size(); i != e; ++i) {
      // If any of our lattice values are passed into this call, which is
      // specially handled by the local analyzer, inform the lattice function.
      DSNode *N = DSG.getNodeForValue(Pointers[i]).getNode();
      for (std::multimap<DSNode*, LatticeValue*>::iterator LVI =
             NodeLVs.lower_bound(N); LVI != NodeLVs.end() && LVI->first == N;) {
        bool AtBottom = false;
        switch (IID) {
        default:
          AtBottom = LVI->second->visitRecognizedCall(*Call);
          break;
        case Intrinsic::memset:
          if (Callbacks & Visit::Stores)
            AtBottom = LVI->second->visitMemSet(*cast<CallInst>(Call));
          break;
        }

        if (AtBottom) {
          delete LVI->second;
          NodeLVs.erase(LVI++);
        } else {
          ++LVI;
        }
      }
    }
  }
}
