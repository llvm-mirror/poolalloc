//===-- RunTimeAssociate.cpp - MemHandle Association Pass -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This transform changes programs so that pointers have an associated handle
// corrosponding to DSA results.  This is a generalization of the Poolalloc
// pass
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pa_assoc"

#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "poolalloc/RunTimeAssociate.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Attributes.h"
#include "llvm/Support/CFG.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"

#include "dsa/EntryPointAnalysis.h"

using namespace llvm;
using namespace rPA;

char RTAssociate::ID = 0;

namespace {
RegisterPass<RTAssociate>
X("rtassoc", "Memory handle association");

STATISTIC(NumArgsAdded, "Number of function arguments added");
STATISTIC(MaxArgsAdded, "Maximum function arguments added to one function");
STATISTIC(NumCloned, "Number of functions cloned");
STATISTIC(NumPools, "Number of pools allocated");
}

////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////

static void GetNodesReachableFromGlobals(DSGraph* G,
                                         DenseSet<const DSNode*> &NodesFromGlobals) {
  for (DSScalarMap::global_iterator I = G->getScalarMap().global_begin(),
          E = G->getScalarMap().global_end(); I != E; ++I)
    G->getNodeForValue(*I).getNode()->markReachableNodes(NodesFromGlobals);
}

static void MarkNodesWhichMustBePassedIn(DenseSet<const DSNode*> &MarkedNodes,
                                         Function &F, DSGraph* G,
                                         EntryPointAnalysis* EPA) {
  // All DSNodes reachable from arguments must be passed in...
  // Unless this is an entry point to the program
  if (!EPA->isEntryPoint(&F)) {
    for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end();
            I != E; ++I) {
      DSGraph::ScalarMapTy::iterator AI = G->getScalarMap().find(I);
      if (AI != G->getScalarMap().end())
        if (DSNode * N = AI->second.getNode())
          N->markReachableNodes(MarkedNodes);
    }
  }

  // Marked the returned node as needing to be passed in.
  if (DSNode * RetNode = G->getReturnNodeFor(F).getNode())
    RetNode->markReachableNodes(MarkedNodes);

  // Calculate which DSNodes are reachable from globals.  If a node is reachable
  // from a global, we will create a global pool for it, so no argument passage
  // is required.
  DenseSet<const DSNode*> NodesFromGlobals;
  GetNodesReachableFromGlobals(G, NodesFromGlobals);

  // Remove any nodes reachable from a global.  These nodes will be put into
  // global pools, which do not require arguments to be passed in.

  for (DenseSet<const DSNode*>::iterator I = NodesFromGlobals.begin(),
          E = NodesFromGlobals.end(); I != E; ++I)
    MarkedNodes.erase(*I);
}


/// FindFunctionPoolArgs - In the first pass over the program, we decide which
/// arguments will have to be added for each function, build the FunctionInfo
/// map and recording this info in the ArgNodes set.
static void FindFunctionPoolArgs(Function &F, FuncInfo& FI,
                                 EntryPointAnalysis* EPA) {
  DenseSet<const DSNode*> MarkedNodes;

  if (FI.G->node_begin() == FI.G->node_end())
    return; // No memory activity, nothing is required

  // Find DataStructure nodes which are allocated in pools non-local to the
  // current function.  This set will contain all of the DSNodes which require
  // pools to be passed in from outside of the function.
  MarkNodesWhichMustBePassedIn(MarkedNodes, F, FI.G,EPA);

  //FI.ArgNodes.insert(FI.ArgNodes.end(), MarkedNodes.begin(), MarkedNodes.end());
  //Work around DenseSet not having iterator traits
  for (DenseSet<const DSNode*>::iterator ii = MarkedNodes.begin(),
       ee = MarkedNodes.end(); ii != ee; ++ii)
    FI.ArgNodes.insert(FI.ArgNodes.end(), *ii);
}



////////////////////////////////////////////////////////////////////////////////
// RTAssociate
////////////////////////////////////////////////////////////////////////////////

// MakeFunctionClone - If the specified function needs to be modified for pool
// allocation support, make a clone of it, adding additional arguments as
// necessary, and return it.  If not, just return null.
//
Function* RTAssociate::MakeFunctionClone(Function &F, FuncInfo& FI, DSGraph* G) {
  if (G->node_begin() == G->node_end()) return 0;

  if (FI.ArgNodes.empty())
    return 0; // No need to clone if no pools need to be passed in!

  // Update statistics..
  NumArgsAdded += FI.ArgNodes.size();
  if (MaxArgsAdded < FI.ArgNodes.size()) MaxArgsAdded = FI.ArgNodes.size();
  ++NumCloned;

  // Figure out what the arguments are to be for the new version of the
  // function
  FunctionType *OldFuncTy = F.getFunctionType();
  std::vector<Type*> ArgTys(FI.ArgNodes.size(), PoolDescPtrTy);
  ArgTys.reserve(OldFuncTy->getNumParams() + FI.ArgNodes.size());

  ArgTys.insert(ArgTys.end(), OldFuncTy->param_begin(), OldFuncTy->param_end());

  // Create the new function prototype
  FunctionType *FuncTy = FunctionType::get(OldFuncTy->getReturnType(), ArgTys,
                                           OldFuncTy->isVarArg());
  // Create the new function...
  Function *New = Function::Create(FuncTy, Function::InternalLinkage, F.getName());
  New->copyAttributesFrom(&F);
  F.getParent()->getFunctionList().insert(&F, New);

  // Set the rest of the new arguments names to be PDa<n> and add entries to the
  // pool descriptors map
  Function::arg_iterator NI = New->arg_begin();
  for (unsigned i = 0, e = FI.ArgNodes.size(); i != e; ++i, ++NI) {
    FI.PoolDescriptors[FI.ArgNodes[i]] = CreateArgPool(FI.ArgNodes[i], NI);
    NI->setName("PDa");
  }

  // Map the existing arguments of the old function to the corresponding
  // arguments of the new function, and copy over the names.
  ValueToValueMapTy ValueMap;
  for (Function::arg_iterator I = F.arg_begin();
          NI != New->arg_end(); ++I, ++NI) {
    ValueMap[I] = NI;
    NI->setName(I->getName());
  }

  // Perform the cloning.
  SmallVector<ReturnInst*,100> Returns;
  // TODO: review the boolean flag here
  CloneFunctionInto(New, &F, ValueMap, true, Returns);

  //
  // The CloneFunctionInto() function will copy the parameter attributes
  // verbatim.  This is incorrect; each attribute should be shifted one so
  // that the pool descriptor has no attributes.
  //
  const AttrListPtr OldAttrs = New->getAttributes();
  if (!OldAttrs.isEmpty()) {
    AttrListPtr NewAttrsVector;
    for (unsigned index = 0; index < OldAttrs.getNumSlots(); ++index) {
      const AttributeWithIndex & PAWI = OldAttrs.getSlot(index);
      unsigned argIndex = PAWI.Index;

      // If it's not the return value, move the attribute to the next
      // parameter.
      if (argIndex) ++argIndex;

      // Add the parameter to the new list.
      NewAttrsVector.addAttr(F.getContext(), argIndex, PAWI.Attrs);
    }

    // Assign the new attributes to the function clone
    New->setAttributes(NewAttrsVector);
  }

  for (ValueToValueMapTy::iterator I = ValueMap.begin(),
          E = ValueMap.end(); I != E; ++I)
    FI.NewToOldValueMap.insert(std::make_pair(I->second, const_cast<Value*>(I->first)));

  return FI.Clone = New;
}

RTAssociate::RTAssociate()
: ModulePass(ID) { }

void RTAssociate::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequiredTransitive<CompleteBUDataStructures > ();
  AU.addRequired<EntryPointAnalysis> ();
}

bool RTAssociate::runOnModule(Module &M) {
  if (M.begin() == M.end()) return false;

  //
  // Get references to the DSA information.  For SAFECode, we need Top-Down
  // DSA.  For Automatic Pool Allocation only, we need Bottom-Up DSA.  In all
  // cases, we need to use the Equivalence-Class version of DSA.
  //
  DataStructures* Graphs = &getAnalysis<CompleteBUDataStructures > ();
  EntryPointAnalysis* EPA = &getAnalysis<EntryPointAnalysis > ();

  //  PoolDescType = OpaqueType::get(M.getContext());
  PoolDescType = Type::getInt32Ty(M.getContext());
  PoolDescPtrTy = PointerType::getUnqual(PoolDescType);
  // TODO: Not sure how to do this anymore, commenting out.
  //M.addTypeName("PoolDescriptor", PoolDescType);

  // Create the pools for memory objects reachable by global variables.
  SetupGlobalPools(&M, Graphs-> getGlobalsGraph());

  // Loop over the functions in the original program finding the pool desc.
  // arguments necessary for each function that is indirectly callable.
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && Graphs->hasDSGraph(*I)) {
      FuncInfo & FI = makeFuncInfo(I, Graphs->getDSGraph(*I));
      FindFunctionPoolArgs(*I, FI, EPA);
    }

  // Map that maps an original function to its clone
  std::map<Function*, Function*> FuncToCloneMap;
  
  // Now clone a function using the pool arg list obtained in the previous
  // pass over the modules.  Loop over only the function initially in the
  // program, don't traverse newly added ones.  If the function needs new
  // arguments, make its clone.
  std::set<Function*> ClonedFunctions;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && !ClonedFunctions.count(I) &&
            Graphs->hasDSGraph(*I)) {
      FuncInfo & FI = FunctionInfo.find(I)->second;
      if (Function* Clone = MakeFunctionClone(*I, FI, Graphs->getDSGraph(*I))) {
        assert(!EPA->isEntryPoint(I) && "Entry Point Cloned");
        FuncToCloneMap[I] = Clone;
        ClonedFunctions.insert(Clone);
      }
    }

  // Now that all call targets are available, rewrite the function bodies of the
  // clones.
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isDeclaration() && !ClonedFunctions.count(I) &&
            Graphs->hasDSGraph(*I)) {
      if (FuncToCloneMap.find(I) == FuncToCloneMap.end()) {
        // Function was changed inplace
        ProcessFunctionBody(*I, *I, Graphs->getDSGraph(*I),Graphs);
      } else {
        // Function was cloned
        ProcessFunctionBody(*I, *FuncToCloneMap[I], Graphs->getDSGraph(*I),
                            Graphs);
      }
    }

  return true;
}

// SetupGlobalPools - Create global pools for all DSNodes in the globals graph.
// This is implemented by making the pool descriptor be a global variable of
// it's own.

void RTAssociate::SetupGlobalPools(Module* M, DSGraph* GG) {
  // Get the globals graph for the program.
  // DSGraph* GG = Graphs->getGlobalsGraph();

  // Get all of the nodes reachable from globals.
  DenseSet<const DSNode*> GlobalHeapNodes;
  GetNodesReachableFromGlobals(GG, GlobalHeapNodes);

  errs() << "Pool allocating " << GlobalHeapNodes.size()
          << " global nodes!\n";

  FuncInfo& FI = makeFuncInfo(0, GG);

  while (GlobalHeapNodes.size()) {
    const DSNode* D = *GlobalHeapNodes.begin();
    GlobalHeapNodes.erase(D);
    FI.PoolDescriptors[D] = CreateGlobalPool(D, M);
  }
}

/// CreateGlobalPool - Create a global pool descriptor object

GlobalVariable* RTAssociate::CreateGlobalPool(const DSNode* D, Module* M) {
  //Must use external linkage unless we have an inializer
  GlobalVariable *GV = new GlobalVariable(*M, PoolDescType, false,
                                          GlobalValue::ExternalLinkage, 0,
                                          "GlobalPool");
  ++NumPools;
  SpecialValues.insert(GV);
  return GV;
}

/// CreatePool - This creates the pool for local DSNodes
///

AllocaInst* RTAssociate::CreateLocalPool(const DSNode* D, Function &F) {
  AllocaInst* AI = new AllocaInst(PoolDescType, 0, "LocalPool",
                                  F.getEntryBlock().begin());
  ++NumPools;
  SpecialValues.insert(AI);
  return AI;
}

Argument* RTAssociate::CreateArgPool(const DSNode*D, Argument* Arg) {
  SpecialValues.insert(Arg);
  return Arg;
}

/// setupPoolForNode - Update or merge the pool with the DSNode's info and update
/// node mappings.

void RTAssociate::setupPoolForNode(const DSNode* D, Value* V) {
  SpecialValues.insert(V);
  PoolInfo*& PI = NodePoolMap[D];
  assert(!PI && "Pool already exists");
  PI = new PoolInfo();
  PI->addPrimaryDescriptor(V);
  PI->mergeNodeInfo(D);
  NodePoolMap[D] = PI;
}

/// ProcessFunctionBody - Pool allocate any data structures which are contained
/// in the specified function.
//

void RTAssociate::ProcessFunctionBody(Function &F, Function &NewF, DSGraph* G,
                                      DataStructures* DS) {
  if (G->node_begin() == G->node_end()) return; // Quick exit if nothing to do.

  FuncInfo &FI = *getFuncInfo(&F);

  // Calculate which DSNodes are reachable from globals.  If a node is reachable
  // from a global, we will create a global pool for it, so no argument passage
  // is required.

  G->getGlobalsGraph();

  // Map all node reachable from this global to the corresponding nodes in
  // the globals graph.
  DSGraph::NodeMapTy GlobalsGraphNodeMapping;
  G->computeGToGGMapping(GlobalsGraphNodeMapping);

  // Loop over all of the nodes which are non-escaping, adding pool-allocatable
  // ones to the NodesToPA vector.
  for (DSGraph::node_iterator I = G->node_begin(), E = G->node_end(); I != E; ++I) {
    DSNode *N = I;
    if (GlobalsGraphNodeMapping.count(N)) {
      // If it is a global pool, set up the pool descriptor appropriately.
      DSNode *GGN = GlobalsGraphNodeMapping[N].getNode();
      assert(getFuncInfo(0)->PoolDescriptors[GGN] && "Should be in global mapping!");
      FI.PoolDescriptors[N] = getFuncInfo(0)->PoolDescriptors[GGN];
    } else if (!FI.PoolDescriptors[N]) {
      // Otherwise, if it was not passed in from outside the function, it must
      // be a local pool!
      assert(!N->isGlobalNode() && "Should be in global mapping!");
      FI.PoolDescriptors[N] = CreateLocalPool(N, NewF);
    }
  }
  TransformBody(NewF, FI, DS);
}

FuncInfo* RTAssociate::getFuncInfo(const Function* F) {
  std::map<const Function*, FuncInfo>::iterator I = FunctionInfo.find(F);
  return I != FunctionInfo.end() ? &I->second : 0;
}

FuncInfo& RTAssociate::makeFuncInfo(const Function* F, DSGraph* G) {
  return FunctionInfo.insert(std::make_pair(F, FuncInfo(F, G))).first->second;
}

void RTAssociate::TransformBody(Function& F, FuncInfo& FI,
                                DataStructures* DS) {
  for (Function::iterator ii = F.begin(), ee = F.end(); ii != ee; ++ii)
    for (BasicBlock::iterator bi = ii->begin(); bi != ii->end();)
      if (CallInst* CI = dyn_cast<CallInst>(bi)) {
        ++bi;
        replaceCall(CallSite(CI), FI, DS);
      } else
        ++bi;
 }

void RTAssociate::replaceCall(CallSite CS, FuncInfo& FI, DataStructures* DS) {
  const Function *CF = CS.getCalledFunction();
  Instruction *TheCall = CS.getInstruction();

  // If the called function is casted from one function type to another, peer
  // into the cast instruction and pull out the actual function being called.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CS.getCalledValue()))
    if (CE->getOpcode() == Instruction::BitCast &&
        isa<Function>(CE->getOperand(0)))
      CF = cast<Function>(CE->getOperand(0));

  if (isa<InlineAsm>(TheCall->getOperand(0))) {
    errs() << "INLINE ASM: ignoring.  Hoping that's safe.\n";
    return;
  }

  // Ignore calls to NULL pointers.
  if (isa<ConstantPointerNull>(CS.getCalledValue())) {
    errs() << "WARNING: Ignoring call using NULL function pointer.\n";
    return;
  }
  // We need to figure out which local pool descriptors correspond to the pool
  // descriptor arguments passed into the function call.  Calculate a mapping
  // from callee DSNodes to caller DSNodes.  We construct a partial isomophism
  // between the graphs to figure out which pool descriptors need to be passed
  // in.  The roots of this mapping is found from arguments and return values.
  //
  DSGraph::NodeMapTy NodeMapping;
  Instruction *NewCall;
  Value *NewCallee;
  std::vector<const DSNode*> ArgNodes;
  DSGraph *CalleeGraph;  // The callee graph

  // For indirect callees, find any callee since all DS graphs have been
  // merged.
  if (CF) { // Direct calls are nice and simple.
    DEBUG(errs() << "  Handling direct call: " << *TheCall);
    FuncInfo *CFI = getFuncInfo(CF);
    if (CFI == 0 || CFI->Clone == 0) // Nothing to transform...
      return;

    NewCallee = CFI->Clone;
    ArgNodes = CFI->ArgNodes;

    assert ((DS->hasDSGraph (*CF)) && "Function has no ECGraph!\n");
    CalleeGraph = DS->getDSGraph(*CF);
  } else {
    DEBUG(errs() << "  Handling indirect call: " << *TheCall);

    // Here we fill in CF with one of the possible called functions.  Because we
    // merged together all of the arguments to all of the functions in the
    // equivalence set, it doesn't really matter which one we pick.
    // (If the function was cloned, we have to map the cloned call instruction
    // in CS back to the original call instruction.)
    Instruction *OrigInst =
      cast<Instruction>(FI.getOldValueIfAvailable(CS.getInstruction()));

    DSCallGraph::callee_iterator I = DS->getCallGraph().callee_begin(CS);
    if (I != DS->getCallGraph().callee_end(CS))
      CF = *I;

    // If we didn't find the callee in the constructed call graph, try
    // checking in the DSNode itself.
    // This isn't ideal as it means that this call site didn't have inlining
    // happen.
    if (!CF) {
      DSGraph* dg = DS->getDSGraph(*OrigInst->getParent()->getParent());
      DSNode* d = dg->getNodeForValue(OrigInst->getOperand(0)).getNode();
      assert (d && "No DSNode!\n");
      std::vector<const Function*> g;
      d->addFullFunctionList(g);
      if (g.size()) {
        EquivalenceClasses< const GlobalValue *> & EC = dg->getGlobalECs();
        for(std::vector<const Function*>::const_iterator ii = g.begin(), ee = g.end();
            !CF && ii != ee; ++ii) {
          for (EquivalenceClasses<const GlobalValue *>::member_iterator MI = EC.findLeader(*ii);
               MI != EC.member_end(); ++MI) // Loop over members in this set.
            if ((CF = dyn_cast<Function>(*MI))) {
              break;
            }
        }
      }
    }

    //
    // Do an assert unless we're bugpointing something.
    //
//    if ((UsingBugpoint) && (!CF)) return;
    if (!CF)
      errs() << "No Graph for CallSite in "
      << TheCall->getParent()->getParent()->getName().str()
      << " originally "
      << OrigInst->getParent()->getParent()->getName().str()
      << "\n";

    assert (CF && "No call graph info");

    // Get the common graph for the set of functions this call may invoke.
//    if (UsingBugpoint && (!(Graphs.hasDSGraph(*CF)))) return;
    assert ((DS->hasDSGraph(*CF)) && "Function has no DSGraph!\n");
    CalleeGraph = DS->getDSGraph(*CF);

#ifndef NDEBUG
    // Verify that all potential callees at call site have the same DS graph.
    DSCallGraph::callee_iterator E = DS->getCallGraph().callee_end(CS);
    for (; I != E; ++I)
      if (!(*I)->isDeclaration())
        assert(CalleeGraph == DS->getDSGraph(**I) &&
               "Callees at call site do not have a common graph!");
#endif

    // Find the DS nodes for the arguments that need to be added, if any.
    FuncInfo *CFI = getFuncInfo(CF);
    assert(CFI && "No function info for callee at indirect call?");
    ArgNodes = CFI->ArgNodes;

    if (ArgNodes.empty())
      return;           // No arguments to add?  Transformation is a noop!

    // Cast the function pointer to an appropriate type!
    std::vector<Type*> ArgTys(ArgNodes.size(), PoolDescPtrTy);
    for (CallSite::arg_iterator I = CS.arg_begin(), E = CS.arg_end();
         I != E; ++I)
      ArgTys.push_back((*I)->getType());

    FunctionType *FTy = FunctionType::get(TheCall->getType(), ArgTys, false);
    PointerType *PFTy = PointerType::getUnqual(FTy);

    // If there are any pool arguments cast the func ptr to the right type.
    NewCallee = CastInst::CreatePointerCast(CS.getCalledValue(), PFTy, "tmp", TheCall);
  }

  Function::const_arg_iterator FAI = CF->arg_begin(), E = CF->arg_end();
  CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
  for ( ; FAI != E && AI != AE; ++FAI, ++AI)
    if (!isa<Constant>(*AI))
      DSGraph::computeNodeMapping(CalleeGraph->getNodeForValue(FAI),
                                  FI.getDSNodeHFor(*AI), NodeMapping, false);

  assert(AI == AE && "Varargs calls not handled yet!");

  // Map the return value as well...
  if (isa<PointerType>(TheCall->getType()))
    DSGraph::computeNodeMapping(CalleeGraph->getReturnNodeFor(*CF),
                                FI.getDSNodeHFor(TheCall), NodeMapping, false);

  // Okay, now that we have established our mapping, we can figure out which
  // pool descriptors to pass in...
  std::vector<Value*> Args;
  for (unsigned i = 0, e = ArgNodes.size(); i != e; ++i) {
    Value *ArgVal = Constant::getNullValue(PoolDescPtrTy);
    if (NodeMapping.count(ArgNodes[i]))
      if (DSNode *LocalNode = NodeMapping[ArgNodes[i]].getNode())
        if (FI.PoolDescriptors.count(LocalNode))
          ArgVal = FI.PoolDescriptors.find(LocalNode)->second;
    if (isa<Constant > (ArgVal) && cast<Constant > (ArgVal)->isNullValue())
      errs() << "WARNING: NULL POOL ARGUMENTS ARE PASSED IN!\n";
    Args.push_back(ArgVal);
  }

  // Add the rest of the arguments...
  Args.insert(Args.end(), CS.arg_begin(), CS.arg_end());

  //
  // There are circumstances where a function is casted to another type and
  // then called (que horible).  We need to perform a similar cast if the
  // type doesn't match the number of arguments.
  //
  if (Function * NewFunction = dyn_cast<Function>(NewCallee)) {
    FunctionType * NewCalleeType = NewFunction->getFunctionType();
    if (NewCalleeType->getNumParams() != Args.size()) {
      std::vector<Type *> Types;
      Type * FuncTy = FunctionType::get (NewCalleeType->getReturnType(),
                                         Types,
                                         true);
      FuncTy = PointerType::getUnqual (FuncTy);
      NewCallee = new BitCastInst (NewCallee, FuncTy, "", TheCall);
    }
  }

  std::string Name = TheCall->getName();    TheCall->setName("");

  if (InvokeInst *II = dyn_cast<InvokeInst>(TheCall)) {
    NewCall = InvokeInst::Create (NewCallee, II->getNormalDest(),
                                  II->getUnwindDest(),
                                  Args, Name, TheCall);
  } else {
    NewCall = CallInst::Create (NewCallee, Args, Name,
                                TheCall);
  }

  TheCall->replaceAllUsesWith(NewCall);
  DEBUG(errs() << "  Result Call: " << *NewCall);

  if (TheCall->getType()->getTypeID() != Type::VoidTyID) {
    // If we are modifying the original function, update the DSGraph...
    DSGraph::ScalarMapTy &SM = FI.G->getScalarMap();
    DSGraph::ScalarMapTy::iterator CII = SM.find(TheCall);
    if (CII != SM.end()) {
      SM[NewCall] = CII->second;
      SM.erase(CII);                     // Destroy the CallInst
    } else if (!FI.NewToOldValueMap.empty()) {
      // Otherwise, if this is a clone, update the NewToOldValueMap with the new
      // CI return value.
      FI.UpdateNewToOldValueMap(TheCall, NewCall);
    }
  } else if (!FI.NewToOldValueMap.empty()) {
    FI.UpdateNewToOldValueMap(TheCall, NewCall);
  }

  //FIXME: attributes on call?
  CallSite(NewCall).setCallingConv(CallSite(TheCall).getCallingConv());

  TheCall->eraseFromParent();
}

