//===- DataStructureStats.cpp - Various statistics for DS Graphs ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a little pass that prints out statistics for DS Graphs.
//
//===----------------------------------------------------------------------===//

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"

#include <ostream>
using namespace llvm;

namespace {
  STATISTIC (TotalNumCallees, "Total number of callee functions at all indirect call sites");
  STATISTIC (NumIndirectCalls, "Total number of indirect call sites in the program");
  //  STATISTIC (NumPoolNodes, "Number of allocation nodes that could be pool allocated");

  // Typed/Untyped memory accesses: If DSA can infer that the types the loads
  // and stores are accessing are correct (ie, the node has not been collapsed),
  // increment the appropriate counter.
  STATISTIC (NumTypedMemAccesses,
                                "Number of loads/stores which are fully typed");
  STATISTIC (NumUntypedMemAccesses,
                                "Number of loads/stores which are untyped");
  STATISTIC (NumTypeCount1Accesses,
                                "Number of loads/stores which are access a DSNode with 1 type");
  STATISTIC (NumTypeCount2Accesses,
                                "Number of loads/stores which are access a DSNode with 2 type");
  STATISTIC (NumTypeCount3Accesses,
                                "Number of loads/stores which are access a DSNode with 3 type");
  STATISTIC (NumTypeCount4Accesses,
                                "Number of loads/stores which are access a DSNode with >3 type");
  STATISTIC (NumIncompleteAccesses,
                                "Number of loads/stores which are on incomplete nodes");

  class DSGraphStats : public FunctionPass, public InstVisitor<DSGraphStats> {
    void countCallees(const Function &F);
    const DSGraph *TDGraph;

    DSNodeHandle getNodeHandleForValue(Value *V);
    bool isNodeForValueUntyped(Value *V);
  public:
    static char ID;
    DSGraphStats() : FunctionPass((intptr_t)&ID) {}

    /// Driver functions to compute the Load/Store Dep. Graph per function.
    bool runOnFunction(Function& F);

    /// getAnalysisUsage - This modify nothing, and uses the Top-Down Graph.
    void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<TDDataStructures>();
    }

    void visitLoad(LoadInst &LI);
    void visitStore(StoreInst &SI);

    /// Debugging support methods
    void print(llvm::raw_ostream &O, const Module* = 0) const { }
  };

  static RegisterPass<DSGraphStats> Z("dsstats", "DS Graph Statistics");
}

char DSGraphStats::ID;

FunctionPass *llvm::createDataStructureStatsPass() { 
  return new DSGraphStats();
}


static bool isIndirectCallee(Value *V) {
  if (isa<Function>(V)) return false;

  if (CastInst *CI = dyn_cast<CastInst>(V))
    return isIndirectCallee(CI->getOperand(0));

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V))
    if (CE->isCast()) 
      return isIndirectCallee(CE->getOperand(0));

  return true;
}


void DSGraphStats::countCallees(const Function& F) {
  unsigned numIndirectCalls = 0, totalNumCallees = 0;

  for (DSGraph::fc_iterator I = TDGraph->fc_begin(), E = TDGraph->fc_end();
       I != E; ++I)
    if (isIndirectCallee(I->getCallSite().getCalledValue())) {
      // This is an indirect function call
      std::vector<const Function*> Callees;
      I->getCalleeNode()->addFullFunctionList(Callees);

      if (Callees.size() > 0) {
        totalNumCallees  += Callees.size();
        ++numIndirectCalls;
      } else {
        DEBUG(errs() << "WARNING: No callee in Function '" 
	      << F.getNameStr() << "' at call: \n"
	      << *I->getCallSite().getInstruction());
      }
    }

  TotalNumCallees  += totalNumCallees;
  NumIndirectCalls += numIndirectCalls;

  if (numIndirectCalls) {
    DEBUG(errs() << "  In function " << F.getName() << ":  "
	  << (totalNumCallees / (double) numIndirectCalls)
	  << " average callees per indirect call\n");
  }
}

DSNodeHandle DSGraphStats::getNodeHandleForValue(Value *V) {
  const DSGraph *G = TDGraph;
  const DSGraph::ScalarMapTy &ScalarMap = G->getScalarMap();
  DSGraph::ScalarMapTy::const_iterator I = ScalarMap.find(V);
  if (I != ScalarMap.end())
    return I->second;
  
  G = TDGraph->getGlobalsGraph();
  const DSGraph::ScalarMapTy &GlobalScalarMap = G->getScalarMap();
  I = GlobalScalarMap.find(V);
  if (I != GlobalScalarMap.end())
    return I->second;
  
  return 0;
}

bool DSGraphStats::isNodeForValueUntyped(Value *V) {
  DSNodeHandle NH = getNodeHandleForValue(V);
  if(!NH.getNode())
    return true;
  else {
    DSNode* N = NH.getNode();
    if (N->isNodeCompletelyFolded())
      return true;
    if ( N->isIncompleteNode()){
      ++NumIncompleteAccesses;
      return true;
    }
    // it is a complete node, now check how many types are present
   int count = 0;
   unsigned offset = NH.getOffset();
   if (N->type_begin() != N->type_end())
    for (DSNode::TyMapTy::const_iterator ii = N->type_begin(),
        ee = N->type_end(); ii != ee; ++ii) {
      if(ii->first != offset)
        continue;
      count += ii->second->size();
    }
 
   if(count == 1)
      ++NumTypeCount1Accesses;
    else if(count == 2)
      ++NumTypeCount2Accesses;
    else if(count == 3)
      ++NumTypeCount3Accesses;
    else 
      ++NumTypeCount4Accesses;
  }
  return false;
}

void DSGraphStats::visitLoad(LoadInst &LI) {
  if (isNodeForValueUntyped(LI.getOperand(0))) {
    NumUntypedMemAccesses++;
  } else {
    NumTypedMemAccesses++;
  }
}

void DSGraphStats::visitStore(StoreInst &SI) {
  if (isNodeForValueUntyped(SI.getOperand(1))) {
    NumUntypedMemAccesses++;
  } else {
    NumTypedMemAccesses++;
  }
}



bool DSGraphStats::runOnFunction(Function& F) {
  TDGraph = getAnalysis<TDDataStructures>().getDSGraph(F);
  countCallees(F);
  visit(F);
  return true;
}
