//===- DSNodeEquivs.cpp -  Build DSNode equivalence classes ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass to compute DSNode equivalence classes across
// DSGraphs.
//
//===----------------------------------------------------------------------===//

#include "assistDS/DSNodeEquivs.h"

#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/Support/InstIterator.h"

namespace llvm {

char DSNodeEquivs::ID = 0;

static RegisterPass<DSNodeEquivs>
X("dsnodeequivs", "Compute DSNode equivalence classes");

// Build equivalence classes of DSNodes that are mapped between graphs.
void DSNodeEquivs::buildDSNodeEquivs(Module &M) {
  TDDataStructures &TDDS = getAnalysis<TDDataStructures>();

  Module::iterator FuncIt = M.begin(), FuncItEnd = M.end();
  for (; FuncIt != FuncItEnd; ++FuncIt) {
    Function &F = *FuncIt;

    if (!TDDS.hasDSGraph(F))
      continue;

    inst_iterator InstIt = inst_begin(F), InstItEnd = inst_end(F);
    for (; InstIt != InstItEnd; ++InstIt) {
      if (CallInst *Call = dyn_cast<CallInst>(&*InstIt)) {
        equivNodesThroughCallsite(Call);
      }
    }
    
    equivNodesToGlobals(TDDS.getDSGraph(F));
  }
}

FunctionList DSNodeEquivs::getCallees(CallSite &CS) {
  const Function *CalledFunc = CS.getCalledFunction();

  // If the called function is casted from one function type to another, peer
  // into the cast instruction and pull out the actual function being called.
  if (ConstantExpr *CExpr = dyn_cast<ConstantExpr>(CS.getCalledValue())) {
    if (CExpr->getOpcode() == Instruction::BitCast &&
        isa<Function>(CExpr->getOperand(0)))
      CalledFunc = cast<Function>(CExpr->getOperand(0));
  }

  FunctionList Callees;

  // Direct calls are simple.
  if (CalledFunc) {
    Callees.push_back(CalledFunc);
    return Callees;
  }

  // Okay, indirect call.
  // Ask the DSCallGraph what this calls...

  TDDataStructures &TDDS = getAnalysis<TDDataStructures>();
  const DSCallGraph &DSCG = TDDS.getCallGraph();

  DSCallGraph::callee_iterator CalleeIt = DSCG.callee_begin(CS);
  DSCallGraph::callee_iterator CalleeItEnd = DSCG.callee_end(CS);
  for (; CalleeIt != CalleeItEnd; ++CalleeIt)
    Callees.push_back(*CalleeIt);

  // If the callgraph doesn't give us what we want, query the DSGraph
  // ourselves.
  if (Callees.empty()) {
    Instruction *Inst = CS.getInstruction();
    Function *Parent = Inst->getParent()->getParent();
    DSNodeHandle &NH = TDDS.getDSGraph(*Parent)->getNodeForValue(CS.getCalledValue());

    if (!NH.isNull()) {
      DSNode *Node = NH.getNode();
      Node->addFullFunctionList(Callees);
    }
  }

  // For debugging, dump out the callsites we are unable to get callees for.
  if (Callees.empty()) {
    errs() << "Failed to get callees for callsite:\n";
    CS.getInstruction()->dump();
  }

  return Callees;
}

// Compute mappings through the given call site.
void DSNodeEquivs::equivNodesThroughCallsite(CallInst *CI) {
  TDDataStructures &TDDS = getAnalysis<TDDataStructures>();
  DSGraph &Graph = *TDDS.getDSGraph(*CI->getParent()->getParent());
  CallSite CS(CI);
  FunctionList Callees = getCallees(CS);

  FunctionList_it CalleeIt = Callees.begin(), CalleeItEnd = Callees.end();
  for (; CalleeIt != CalleeItEnd; ++CalleeIt) {
    const Function &Callee = **CalleeIt;

    // We can't merge through graphs that don't exist.
    if (!TDDS.hasDSGraph(Callee))
      continue;
    
    DSGraph &CalleeGraph = *TDDS.getDSGraph(Callee);
    DSGraph::NodeMapTy NodeMap;

    // Heavily lifted/inspired by PA code

    // Map arguments
    Function::const_arg_iterator FArgIt = Callee.arg_begin();
    Function::const_arg_iterator FArgItEnd = Callee.arg_end();
    CallSite::arg_iterator ArgIt = CS.arg_begin(), ArgItEnd = CS.arg_end();
    for (; FArgIt != FArgItEnd && ArgIt != ArgItEnd; ++FArgIt, ++ArgIt) {
      if (isa<Constant>(*ArgIt))
        continue;

      DSNodeHandle &CalleeArgNH = CalleeGraph.getNodeForValue(FArgIt);
      DSNodeHandle &CSArgNH = Graph.getNodeForValue(*ArgIt);
      DSGraph::computeNodeMapping(CalleeArgNH, CSArgNH, NodeMap, false);
    }

    // Map return value
    if (isa<PointerType>(CI->getType())) {
      DSNodeHandle &CalleeRetNH = CalleeGraph.getReturnNodeFor(Callee);
      DSNodeHandle &CINH = Graph.getNodeForValue(CI);
      DSGraph::computeNodeMapping(CalleeRetNH, CINH, NodeMap, false);
    }

    // Merge information from the computed node mapping into the equivalence
    // classes.
    equivNodeMapping(NodeMap);
  }
}

// Compute mappings with the globals graph.
void DSNodeEquivs::equivNodesToGlobals(DSGraph *G) {
  DSGraph *GlobalsGr = G->getGlobalsGraph();
  DSGraph::NodeMapTy NodeMap;
  DSScalarMap &ScalarMap = GlobalsGr->getScalarMap();

  DSScalarMap::global_iterator GlobalIt = ScalarMap.global_begin();
  DSScalarMap::global_iterator GlobalItEnd = ScalarMap.global_end();
  for (; GlobalIt != GlobalItEnd; ++GlobalIt) {
    const GlobalValue *Global = *GlobalIt;

    DSNode *LocalNode = G->getNodeForValue(Global).getNode();
    DSNode *GlobalNode = GlobalsGr->getNodeForValue(Global).getNode();
    assert(LocalNode && "No node for global in local scalar map?");
    assert(GlobalNode && "No node for global in global scalar map?");

    // Map the two together and all reachable from each...
    DSGraph::computeNodeMapping(LocalNode, GlobalNode, NodeMap, false);

    // Build EC's with this mapping.
    equivNodeMapping(NodeMap);
  }
}

// Utility function to put nodes that map together into equivalence classes.
void DSNodeEquivs::equivNodeMapping(DSGraph::NodeMapTy &NodeMap) {
  DSGraph::NodeMapTy::iterator NodeMapIt = NodeMap.begin();
  DSGraph::NodeMapTy::iterator NodeMapItEnd = NodeMap.end();
  for (; NodeMapIt != NodeMapItEnd; ++NodeMapIt) {
    DSNodeHandle &NH = NodeMapIt->second;
    if (NH.isNull())
      continue;

    const DSNode *N1 = NodeMapIt->first;
    const DSNode *N2 = NH.getNode();

    Classes.unionSets(N1, N2);
  }
}

bool DSNodeEquivs::runOnModule(Module &M) {
  buildDSNodeEquivs(M);
  // Does not modify module.
  return false;
}

// Returns the computed equivalence classes.
const EquivalenceClasses<const DSNode *> &
DSNodeEquivs::getEquivalenceClasses() {
  return Classes;
}

}
