//===-- StructureFieldVisitor.h - Macroscopic DS inspector ------*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines the public interface to the LatticeValue and
// StructureFieldVisitor classes.
//
//===----------------------------------------------------------------------===//

#ifndef MACROSCOPIC_STRUCTUREFIELDVISITOR_H
#define MACROSCOPIC_STRUCTUREFIELDVISITOR_H

#include <set>
#include <map>
#include <vector>

namespace llvm {
  class Type;
  class Constant;
  class Instruction;
  class LoadInst;
  class CallInst;
  class StoreInst;
  class EquivClassGraphs;
  class DSNode;
  class DSGraph;

namespace Macroscopic {

/// FindAllDataStructures - Inspect the program specified by ECG, adding to
/// 'Nodes' all of the data structures node in the program that contain the
/// "IncludeFlags" and do not contain "ExcludeFlags" node flags.  If
/// OnlyHomogenous is true, only type-homogenous nodes are considered.
void FindAllDataStructures(std::set<DSNode*> &Nodes, unsigned IncludeFlags,
                           unsigned ExcludeFlags, bool OnlyHomogenous,
                           EquivClassGraphs &ECG);

//===----------------------------------------------------------------------===//
/// Visit types - This describes an enum specifying which methods are
/// overloaded in the concrete implementation of the LatticeValue class.
namespace Visit {
  enum {
    Loads = 1,
    Stores = 2,
    Allocs = 4,
    Deallocs = 8,
    // ...
  };
}

//===----------------------------------------------------------------------===//
/// LatticeValue - Describe.
///
class LatticeValue {
  DSNode *Node;
  std::vector<unsigned> Idxs;  // The index path to get to this field.
  const Type *FieldTy;         // The type of this field.
public:
  LatticeValue(DSNode *N, const std::vector<unsigned> &I,
               const Type *Ty) : Node(N), Idxs(I), FieldTy(Ty) {}
  virtual ~LatticeValue() {}

  DSNode *getNode() const { return Node; }
  DSGraph &getParentGraph() const;

  const std::vector<unsigned> &getIndices() const { return Idxs; }
  const Type *getFieldType() const { return FieldTy; }

  /// getFieldOffset - Get the byte offset of this field from the start of the
  /// node.
  ///
  unsigned getFieldOffset() const;

  /// dump - Print debugging information about this class.
  ///
  virtual void dump() const;

  /// mergeInValue - Merge the information from the RHS lattice value (which is
  /// guaranteed to be the same dynamic type as 'this') into this lattice value.
  /// If the resultant value hits bottom, return true.  This is used for
  /// interprocedural analysis.
  ///
  virtual bool mergeInValue(const LatticeValue *RHS) {
    return true;
  }
  
  //===--------------------------------------------------------------------===//
  // Visitation methods - These methods update the current lattice state
  // based on the information in the operation.  If the lattice value reaches
  // bottom, the method implementation should return true so that analysis
  // can be stopped as early as possible.  Visitation methods are grouped by
  // their Visitation class.

  // All lattice values must implement these.

  /// visitRecognizedCall - The node for this lattice value is passed into some
  /// external function that is "known" by the Local analysis pass (e.g. atoi).
  /// By default, this stops any analysis of the node.
  ///
  virtual bool visitRecognizedCall(Instruction &I) {
    return true;
  }
  
  // Load Vistation methods.
  virtual bool visitLoad(LoadInst &);

  // Store Visitation methods.
  virtual bool visitStore(StoreInst &);
  virtual bool visitGlobalInit(Constant *InitVal);
  virtual bool visitMemSet(CallInst &I) {
    return visitRecognizedCall((Instruction&)I);
  }
};

//===----------------------------------------------------------------------===//
/// CombinedLatticeValue - Describe.
///
template<typename L1, typename L2>
class CombinedLatticeValue : public LatticeValue {
  L1 LV1; L2 LV2;
  bool LV1Bottom, LV2Bottom;
public:
  CombinedLatticeValue(DSNode *Node, const std::vector<unsigned> &Idxs,
                       const Type *FieldTy)
    : LatticeValue(Node, Idxs, FieldTy), LV1(Node, Idxs, FieldTy),
      LV2(Node, Idxs, FieldTy), LV1Bottom(false), LV2Bottom(false) {}
      
  static unsigned getInterestingEvents() {
    return L1::getInterestingEvents() | L2::getInterestingEvents();
  }

  static CombinedLatticeValue *create(DSNode *Node,
                                      const std::vector<unsigned> &Idxs,
                                      const Type *FieldTy) {
    return new CombinedLatticeValue(Node, Idxs, FieldTy);      
  }

  void dump() const {
    if (!LV1Bottom) LV1.dump();
    if (!LV2Bottom) LV2.dump();
  }

  virtual bool mergeInValue(const LatticeValue *RHSLV) {
    const CombinedLatticeValue *RHS =
      static_cast<const CombinedLatticeValue *>(RHSLV);
    LV1Bottom |= RHS->LV1Bottom;
    LV2Bottom |= RHS->LV2Bottom;
    if (!LV1Bottom) LV1Bottom = LV1.mergeInValue(&RHS->LV1);
    if (!LV2Bottom) LV2Bottom = LV2.mergeInValue(&RHS->LV2);
    return LV1Bottom & LV2Bottom;
  }
  
  virtual bool visitRecognizedCall(Instruction &I) {
    if (!LV1Bottom) LV1Bottom = LV1.visitRecognizedCall(I);
    if (!LV2Bottom) LV2Bottom = LV2.visitRecognizedCall(I);
    return LV1Bottom & LV2Bottom;
  }
  
  // Load Vistation methods.
  virtual bool visitLoad(LoadInst &LI) {
    if (!LV1Bottom && (L1::getInterestingEvents() & Visit::Loads))
      LV1Bottom = LV1.visitLoad(LI);
    if (!LV2Bottom && (L2::getInterestingEvents() & Visit::Loads))
      LV2Bottom = LV2.visitLoad(LI);
    return LV1Bottom & LV2Bottom;
  }

  // Store Visitation methods.
  virtual bool visitStore(StoreInst &SI) {
    if (!LV1Bottom && (L1::getInterestingEvents() & Visit::Stores))
      LV1Bottom = LV1.visitStore(SI);
    if (!LV2Bottom && (L2::getInterestingEvents() & Visit::Stores))
      LV2Bottom = LV2.visitStore(SI);
    return LV1Bottom & LV2Bottom;
  }

  virtual bool visitGlobalInit(Constant *InitVal) {
    if (!LV1Bottom && (L1::getInterestingEvents() & Visit::Stores))
      LV1Bottom = LV1.visitGlobalInit(InitVal);
    if (!LV2Bottom && (L2::getInterestingEvents() & Visit::Stores))
      LV2Bottom = LV2.visitGlobalInit(InitVal);
    return LV1Bottom & LV2Bottom;
  }

  virtual bool visitMemSet(CallInst &I) {
    if (!LV1Bottom && (L1::getInterestingEvents() & Visit::Stores))
      LV1Bottom = LV1.visitMemSet(I);
    if (!LV2Bottom && (L2::getInterestingEvents() & Visit::Stores))
      LV2Bottom = LV2.visitMemSet(I);
    return LV1Bottom & LV2Bottom;
  }
};


//===----------------------------------------------------------------------===//
/// StructureFieldVisitorBase - This implements all of the heavy lifting
/// for the StructureFieldVisitor class.  This class should not be used
/// directly, see it (below) for usage.
///
class StructureFieldVisitorBase {
  unsigned Callbacks;     // Bitfield containing bits from Macroscopic::Visit
  EquivClassGraphs &ECG;
  
  std::map<DSGraph*, std::multimap<DSNode*,LatticeValue*> > CalleeFnFacts;

  // createLatticeValue - Provide a virtual ctor for the concrete lattice value.
  virtual LatticeValue* createLatticeValue(DSNode *Node,
                                           const std::vector<unsigned> &Idxs,
                                           const Type *FieldTy) = 0;
protected:
  // Only allow this to be subclassed.
  StructureFieldVisitorBase(unsigned CB, EquivClassGraphs &ecg)
    : Callbacks(CB), ECG(ecg) {}
  virtual ~StructureFieldVisitorBase() {}
  
  
  std::set<LatticeValue*> visitNodes(const std::set<DSNode*> &Nodes);
  void visitFields(std::set<LatticeValue*> &Fields);

private:
  void visitGraph(DSGraph &DSG, std::multimap<DSNode*, LatticeValue*> &NodeLVs);
  std::multimap<DSNode*, LatticeValue*> &getCalleeFacts(DSGraph &DSG);

  void AddLatticeValuesForFields(DSNode *N, const Type *Ty,
                                 const std::vector<unsigned> &Idxs,
                                 std::set<LatticeValue*> &Values);
  void AddLatticeValuesForNode(DSNode *N, std::set<LatticeValue*> &Values);
  void ProcessNodesReachableFromGlobals(DSGraph &DSG,
                             std::multimap<DSNode*,LatticeValue*> &NodeLVs);

};



//===----------------------------------------------------------------------===//
/// FIXME: Describe
template<typename ConcLatticeVal>
class StructureFieldVisitor : public StructureFieldVisitorBase {
public:
  // FIXME: We really want to be able to infer which methods LatticeVal
  // overloads from the base class using template metaprogramming techniques.
  StructureFieldVisitor(EquivClassGraphs &ECG)
    : StructureFieldVisitorBase(ConcLatticeVal::getInterestingEvents(), ECG) {}

  /// visit - Visit the specified set of data structure nodes, recursively
  /// visiting all accesses to structure fields defined in those nodes.  This
  /// method returns a set of lattice values that have not reached bottom.
  std::set<ConcLatticeVal*> visit(const std::set<DSNode*> &Nodes) {
    std::set<LatticeValue*> ResultVals = visitNodes(Nodes);

    // Convert the result set to the appropriate lattice type.
    std::set<ConcLatticeVal*> Result;
    while (!ResultVals.empty()) {
      Result.insert(static_cast<ConcLatticeVal*>(*ResultVals.begin()));
      ResultVals.erase(ResultVals.begin());
    }
    return Result;
  }


private:  // Virtual method implementations for the base class.
  virtual LatticeValue* createLatticeValue(DSNode *N,
                                           const std::vector<unsigned> &Idxs,
                                           const Type *FieldTy) {
    return ConcLatticeVal::create(N, Idxs, FieldTy);
  }
};

}  // End Macroscopic namespace
}  // End llvm namespace

#endif
