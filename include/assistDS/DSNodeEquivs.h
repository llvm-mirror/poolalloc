//===- DSNodeEquivs.h - Build DSNode equivalence classes ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass computes equivalence classes of DSNodes across DSGraphs.
//
//===----------------------------------------------------------------------===//

#ifndef DSNODEEQUIVS_H
#define DSNODEEQUIVS_H

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "dsa/DSNode.h"

#include "llvm/ADT/EquivalenceClasses.h"

#include <vector>

namespace llvm {

typedef std::vector<const Function *> FunctionList;
typedef FunctionList::iterator FunctionList_it;

class DSNodeEquivs : public ModulePass {
private:
  EquivalenceClasses<const DSNode*> Classes;

  void buildDSNodeEquivs(Module &M);

  void addNodesFromGraph(DSGraph *G);
  FunctionList getCallees(CallSite &CS);
  void equivNodesThroughCallsite(CallInst *CI);
  void equivNodesToGlobals(DSGraph *G);
  void equivNodeMapping(DSGraph::NodeMapTy & NM);
  
public:
  static char ID;

  DSNodeEquivs() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<TDDataStructures>();
    AU.setPreservesAll();
  }

  bool runOnModule(Module &M);

  // Returns the computed equivalence classes.
  const EquivalenceClasses<const DSNode*> &getEquivalenceClasses();

  // Returns the DSNode in the equivalence classes for the specified value.
  const DSNode *getMemberForValue(const Value *V);
};

}

#endif // DSNODEEQUIVS_H
