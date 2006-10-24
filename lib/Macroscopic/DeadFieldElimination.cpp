//===-- DeadFieldElimination.cpp - Dead Field Elimination Pass ------------===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements the -rds-deadfieldelim pass, which implements
// Macroscopic Dead Field Elimination.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "rds-deadfieldelim"
#include "StructureFieldVisitor.h"
#include "llvm/Analysis/DataStructure/DataStructure.h"
#include "llvm/Analysis/DataStructure/DSGraph.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Timer.h"
using namespace llvm;
using namespace llvm::Macroscopic;

namespace {
  /// DeadFieldElim - This class implements the dead field elimination
  /// optimization.  This transformation is a very simple macroscopic
  /// optimization whose high-level algorithm works like this:
  ///
  /// 1. Identify all type-homogenous data structures that only contain HGS
  ///    composition bits.
  /// 2. For each data structure, visit every load of fields in the node.
  ///    Keep track of whether a field is ever loaded.
  /// 3. If we found fields that are never loaded from, rewrite the affected 
  ///    data structures, deleting the dead fields!
  ///
  class DeadFieldElim : public ModulePass {

    bool runOnModule(Module &M);

    void getAnalysisUsage(AnalysisUsage &AU) const {
      // Need information from DSA.
      AU.addRequired<EquivClassGraphs>();
      AU.addPreserved<EquivClassGraphs>();
    }
  };
  RegisterPass<DeadFieldElim>
  X("rds-deadfieldelim", "Macroscopic Dead Field Elimination");
}  // end anonymous namespace


namespace {
  /// DeadFieldElimLattice - This is the most trivial lattice possible: it
  /// consists of two states, dead (the default) and alive (overdefined,
  /// bottom).  Because it has only two states, it does not need to maintain any
  /// actual state.
  struct DeadFieldElimLattice : public LatticeValue {
    DeadFieldElimLattice(DSNode *Node, const std::vector<unsigned> &Idxs,
                         const Type *FieldTy)
      : LatticeValue(Node, Idxs, FieldTy) {}

    // getInterestingEvents - We only care about reads from memory?
    static unsigned getInterestingEvents() { return Macroscopic::Visit::Loads; }

    virtual void dump() const {
      LatticeValue::dump();
      std::cerr << "  DEAD FIELD!\n";
    }

    // create - Return a dataflow fact for this field, initialized to top.
    static DeadFieldElimLattice *create(DSNode *Node,
                                        const std::vector<unsigned> &Idxs,
                                        const Type *FieldTy) {
      return new DeadFieldElimLattice(Node, Idxs, FieldTy);      
    }

    virtual bool mergeInValue(const LatticeValue *RHS) {
      return false;  // Neither LHS nor RHS are overdefined.
    }

    // VisitLoad - Load events always move us to bottom.
    virtual bool visitLoad(LoadInst &LI) {
      return true;  // Bottom!
    }
  };
}


#include "llvm/Constants.h"
#include "llvm/Instructions.h"
namespace {
  /// ConstPropLattice - This is the standard 3-level constant propagation
  /// lattice.  When CstVal is null it is undefined, when it is non-null it is
  /// defined to that constant, and it hits bottom of set to something
  /// unanalyzable or multiple values.
  struct ConstPropLattice : public LatticeValue {
    Constant *CstVal;

    ConstPropLattice(DSNode *Node, const std::vector<unsigned> &Idxs,
                     const Type *FieldTy)
      : LatticeValue(Node, Idxs, FieldTy), CstVal(0) {}

    // getInterestingEvents - We only care about reads from memory?
    static unsigned getInterestingEvents() { return Macroscopic::Visit::Stores;}

    // create - Return a dataflow fact for this field, initialized to top.
    static ConstPropLattice *create(DSNode *Node,
                                    const std::vector<unsigned> &Idxs,
                                    const Type *FieldTy) {
      return new ConstPropLattice(Node, Idxs, FieldTy);      
    }

    bool mergeInValue(Constant *V) {
      if (V == 0 || isa<UndefValue>(V)) return false;
      if (CstVal == 0)
        CstVal = V;
      else if (CstVal != V)
        return true;
      return false;
    }

    virtual void dump() const {
      LatticeValue::dump();
      if (CstVal)
        std::cerr << "  CONSTANT VALUE = " << *CstVal << "\n";
      else
        std::cerr << "  FIELD NEVER DEFINED!\n";
    }

    virtual bool mergeInValue(const LatticeValue *RHSLV) {
      const ConstPropLattice *RHS = static_cast<const ConstPropLattice*>(RHSLV);
      return mergeInValue(RHS->CstVal);
    }

    virtual bool visitStore(StoreInst &SI) {
      if (Constant *CV = dyn_cast<Constant>(SI.getOperand(0)))
        return mergeInValue(CV);
      return true;  // Bottom!
    }

    bool visitGlobalInit(Constant *InitVal) {
      return mergeInValue(InitVal);
    }

    virtual bool visitMemSet(CallInst &I) {
      if (Constant *C = dyn_cast<Constant>(I.getOperand(2)))
        if (C->isNullValue()) {
          std::cerr << "HANDLED MEMSET: " << I;
          return mergeInValue(Constant::getNullValue(getFieldType()));
        }
      return true;
    }
  };
}



bool DeadFieldElim::runOnModule(Module &M) {
  EquivClassGraphs &ECG = getAnalysis<EquivClassGraphs>();

  // Step #1: Identify all type-homogenous HGS (non-U) data structure nodes.
  std::set<DSNode*> Nodes;

  { NamedRegionTimer XX("Dead Fields");

  FindAllDataStructures(Nodes, 0 /*nothing required*/,
                        DSNode::UnknownNode, true /*typesafe*/, ECG);

  DEBUG(std::cerr << "DeadFieldElim: Found " << Nodes.size()
                  << " eligible data structure nodes.\n");

  // Step #2: For each data structure, visit every load of fields in the node.
  // Keep track of whether a field is ever loaded.  The visitor returns a set of
  // lattice values that have not reached bottom (in this case, that means it
  // returns a set of dead fields).
  std::set<DeadFieldElimLattice*> DeadFields =
    StructureFieldVisitor<DeadFieldElimLattice>(ECG).visit(Nodes);

  DEBUG(std::cerr << "DeadFieldElim: Found " << DeadFields.size()
        << " dead fields!\n");
  for (std::set<DeadFieldElimLattice*>::iterator I = DeadFields.begin(),
         E = DeadFields.end(); I != E; ++I) {
    DEBUG((*I)->dump());
    delete *I;
  }
  DEBUG(std::cerr << "\n");
  }
  //return true;

  { NamedRegionTimer XX("Constants");

  Nodes.clear();
  FindAllDataStructures(Nodes, 0 /*nothing required*/,
                        DSNode::UnknownNode, true /*typesafe*/, ECG);

  // Step #2: For each data structure, visit every stores to fields in the node.
  // Keep track of whether a field is always constant.  The visitor returns a
  // set of lattice values that have not reached bottom (in this case, that
  // means they are either undefined or overdefined.
  std::set<ConstPropLattice*> ConstantFields =
    StructureFieldVisitor<ConstPropLattice>(ECG).visit(Nodes);

  DEBUG(std::cerr << "ConstProp: Found " << ConstantFields.size()
        << " constant or uninitialized fields!\n");
  for (std::set<ConstPropLattice*>::iterator I = ConstantFields.begin(),
         E = ConstantFields.end(); I != E; ++I) {
    DEBUG((*I)->dump());
    delete *I;
  }
  DEBUG(std::cerr << "\n");

  }

  return true;

  // NOTE: We could do a simple thing that checks for fields whose loads are
  // immediately casted to something smaller, and stores that are casted from
  // something smaller to shrink them.


  { NamedRegionTimer XX("Combined");

  Nodes.clear();
  FindAllDataStructures(Nodes, 0 /*nothing required*/,
                        DSNode::UnknownNode, true /*typesafe*/, ECG);

  typedef CombinedLatticeValue<DeadFieldElimLattice,ConstPropLattice>
    CombinedLattice;

  // Step #2: For each data structure, visit every stores to fields in the node.
  // Keep track of whether a field is always constant.  The visitor returns a
  // set of lattice values that have not reached bottom (in this case, that
  // means they are either undefined or overdefined.
  std::set<CombinedLattice*> CombinedFields =
    StructureFieldVisitor<CombinedLattice>(ECG).visit(Nodes);

  DEBUG(std::cerr << "Combined: Found " << CombinedFields.size()
        << " interesting fields!\n");
  for (std::set<CombinedLattice*>::iterator I = CombinedFields.begin(),
         E = CombinedFields.end(); I != E; ++I) {
    DEBUG((*I)->dump());
    delete *I;
  }
  DEBUG(std::cerr << "\n");

  }

  return true;
}
