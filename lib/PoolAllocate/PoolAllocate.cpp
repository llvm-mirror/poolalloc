//===-- PoolAllocate.cpp - Pool Allocation Pass ---------------------------===//
//
// This transform changes programs so that disjoint data structures are
// allocated out of different pools of memory, increasing locality.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "PoolAllocation"
#include "poolalloc/PoolAllocate.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Analysis/DataStructure.h"
#include "llvm/Analysis/DSGraph.h"
#include "llvm/Support/CFG.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "Support/CommandLine.h"
#include "Support/Debug.h"
#include "Support/DepthFirstIterator.h"
#include "Support/Statistic.h"
using namespace llvm;
using namespace PA;

// PASS_ALL_ARGUMENTS - If this is set to true, pass in pool descriptors for all
// DSNodes in a function, even if there are no allocations or frees in it.  This
// is useful for SafeCode.
#define PASS_ALL_ARGUMENTS 0

const Type *PoolAllocate::PoolDescPtrTy = 0;

namespace {
  Statistic<> NumArgsAdded("poolalloc", "Number of function arguments added");
  Statistic<> NumCloned   ("poolalloc", "Number of functions cloned");
  Statistic<> NumPools    ("poolalloc", "Number of pools allocated");
  Statistic<> NumPoolFree ("poolalloc", "Number of poolfree's elided");

  const Type *VoidPtrTy;

  // The type to allocate for a pool descriptor: { sbyte*, uint, uint }
  // void *Data (the data)
  // unsigned NodeSize  (size of an allocated node)
  // unsigned FreeablePool (are slabs in the pool freeable upon calls to 
  //                        poolfree?)
  const Type *PoolDescType;
  
  RegisterOpt<PoolAllocate>
  X("poolalloc", "Pool allocate disjoint data structures");

  cl::opt<bool> DisableInitDestroyOpt("poolalloc-force-simple-pool-init",
                                      cl::desc("Always insert poolinit/pooldestroy calls at start and exit of functions"));
}

void PoolAllocate::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<BUDataStructures>();
  AU.addRequired<TDDataStructures>();
  AU.addRequired<TargetData>();
}

bool PoolAllocate::run(Module &M) {
  if (M.begin() == M.end()) return false;
  CurModule = &M;
  BU = &getAnalysis<BUDataStructures>();
  TDDS = &getAnalysis<TDDataStructures>();

  // Add the pool* prototypes to the module
  AddPoolPrototypes();

  // Figure out what the equivalence classes are for indirectly called functions
  BuildIndirectFunctionSets(M);

  // Create the pools for memory objects reachable by global variables.
  if (SetupGlobalPools(M))
    return true;

  // Loop over the functions in the original program finding the pool desc.
  // arguments necessary for each function that is indirectly callable.
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    if (!I->isExternal())
      FindFunctionPoolArgs(*I);

  std::map<Function*, Function*> FuncMap;

  // Now clone a function using the pool arg list obtained in the previous pass
  // over the modules.  Loop over only the function initially in the program,
  // don't traverse newly added ones.  If the function needs new arguments, make
  // its clone.
  Module::iterator LastOrigFunction = --M.end();
  for (Module::iterator I = M.begin(); ; ++I) {
    if (!I->isExternal())
      if (Function *R = MakeFunctionClone(*I))
        FuncMap[I] = R;
    if (I == LastOrigFunction) break;
  }
  
  ++LastOrigFunction;

  // Now that all call targets are available, rewrite the function bodies of the
  // clones.
  for (Module::iterator I = M.begin(); I != LastOrigFunction; ++I)
    if (!I->isExternal()) {
      std::map<Function*, Function*>::iterator FI = FuncMap.find(I);
      ProcessFunctionBody(*I, FI != FuncMap.end() ? *FI->second : *I);
    }

  return true;
}

static void GetNodesReachableFromGlobals(DSGraph &G,
                                         hash_set<DSNode*> &NodesFromGlobals) {
  for (DSGraph::ScalarMapTy::iterator I = G.getScalarMap().begin(), 
         E = G.getScalarMap().end(); I != E; ++I)
    if (isa<GlobalValue>(I->first))              // Found a global
      I->second.getNode()->markReachableNodes(NodesFromGlobals);
}

// AddPoolPrototypes - Add prototypes for the pool functions to the specified
// module and update the Pool* instance variables to point to them.
//
void PoolAllocate::AddPoolPrototypes() {
  if (VoidPtrTy == 0) {
    VoidPtrTy = PointerType::get(Type::SByteTy);
    PoolDescType =
      StructType::get(make_vector<const Type*>(VoidPtrTy, VoidPtrTy,
                                               Type::UIntTy, Type::UIntTy, 0));
    PoolDescPtrTy = PointerType::get(PoolDescType);
  }

  CurModule->addTypeName("PoolDescriptor", PoolDescType);
  
  // Get poolinit function...
  PoolInit = CurModule->getOrInsertFunction("poolinit", Type::VoidTy,
                                            PoolDescPtrTy, Type::UIntTy, 0);

  // Get pooldestroy function...
  PoolDestroy = CurModule->getOrInsertFunction("pooldestroy", Type::VoidTy,
                                               PoolDescPtrTy, 0);
  
  // The poolalloc function
  PoolAlloc = CurModule->getOrInsertFunction("poolalloc", 
                                             VoidPtrTy, PoolDescPtrTy,
                                             Type::UIntTy, 0);
  
  // Get the poolfree function...
  PoolFree = CurModule->getOrInsertFunction("poolfree", Type::VoidTy,
                                            PoolDescPtrTy, VoidPtrTy, 0);  
}


static void printNTOMap(std::map<Value*, const Value*> &NTOM) {
  std::cerr << "NTOM MAP\n";
  for (std::map<Value*, const Value *>::iterator I = NTOM.begin(), 
	 E = NTOM.end(); I != E; ++I) {
    if (!isa<Function>(I->first) && !isa<BasicBlock>(I->first))
      std::cerr << *I->first << " to " << *I->second << "\n";
  }
}

static void MarkNodesWhichMustBePassedIn(hash_set<DSNode*> &MarkedNodes,
                                         Function &F, DSGraph &G) {
  // Mark globals and incomplete nodes as live... (this handles arguments)
  if (F.getName() != "main") {
    // All DSNodes reachable from arguments must be passed in.
    for (Function::aiterator I = F.abegin(), E = F.aend(); I != E; ++I) {
      DSGraph::ScalarMapTy::iterator AI = G.getScalarMap().find(I);
      if (AI != G.getScalarMap().end())
        if (DSNode *N = AI->second.getNode())
          N->markReachableNodes(MarkedNodes);
    }
  }

  // Marked the returned node as alive...
  if (DSNode *RetNode = G.getReturnNodeFor(F).getNode())
    RetNode->markReachableNodes(MarkedNodes);

  // Calculate which DSNodes are reachable from globals.  If a node is reachable
  // from a global, we will create a global pool for it, so no argument passage
  // is required.
  hash_set<DSNode*> NodesFromGlobals;
  GetNodesReachableFromGlobals(G, NodesFromGlobals);

  // Remove any nodes reachable from a global.  These nodes will be put into
  // global pools, which do not require arguments to be passed in.  Also, erase
  // any marked node that is not a heap node.  Since no allocations or frees
  // will be done with it, it needs no argument.
  for (hash_set<DSNode*>::iterator I = MarkedNodes.begin(),
         E = MarkedNodes.end(); I != E; ) {
    DSNode *N = *I++;
    if ((!N->isHeapNode() && !PASS_ALL_ARGUMENTS) || NodesFromGlobals.count(N))
      MarkedNodes.erase(N);
  }
}

const PA::EquivClassInfo &PoolAllocate::getECIForIndirectCallSite(CallSite CS) {
  Instruction *I = CS.getInstruction();
  assert(I && "Not a call site?");
  assert(OneCalledFunction.count(I) && "No targets for indcall?");
  Function *Called = OneCalledFunction[I];
  Function *Leader = FuncECs.findClass(Called);
  assert(Leader && "Leader not found for indirect call target!");
  assert(ECInfoForLeadersMap.count(Leader) && "No ECI for indirect call site!");
  return ECInfoForLeadersMap[Leader];
}

// BuildIndirectFunctionSets - Iterate over the module looking for indirect
// calls to functions.  If a call site can invoke any functions [F1, F2... FN],
// unify the N functions together in the FuncECs set.
//
void PoolAllocate::BuildIndirectFunctionSets(Module &M) {
  const BUDataStructures::ActualCalleesTy AC = BU->getActualCallees();

#if 0  // THIS SHOULD WORK, but doesn't because DSA is buggy.  FIXME!

  // Loop over all of the indirect calls in the program.  If a call site can
  // call multiple different functions, we need to unify all of the callees into
  // the same equivalence class.
  Instruction *LastInst = 0;
  Function *FirstFunc = 0;
  for (BUDataStructures::ActualCalleesTy::const_iterator I = AC.begin(),
         E = AC.end(); I != E; ++I) {
    CallSite CS = CallSite::get(I->first);
    if (!CS.getCalledFunction() &&    // Ignore direct calls
        !I->second->isExternal()) {   // Ignore functions we cannot modify
      //std::cerr << "CALLEE: " << I->second->getName() << " from : " <<
      //I->first;
      if (I->first != LastInst) {
        // This is the first callee from this call site.
        LastInst = I->first;
        FirstFunc = I->second;
        OneCalledFunction[LastInst] = FirstFunc;
        FuncECs.addElement(I->second);
      } else {
        // This is not the first possible callee from a particular call site.
        // Union the callee in with the other functions.
        FuncECs.unionSetsWith(FirstFunc, I->second);
      }
    }
  }
#else

  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    if (MI->isExternal()) continue;
    DEBUG(std::cerr << "Processing indirect calls function:" <<  MI->getName()
          << "\n");

    DSGraph &TDG = TDDS->getDSGraph(*MI);
    const std::vector<DSCallSite> &callSites = TDG.getFunctionCalls();

    // For each call site in the function
    // All the functions that can be called at the call site are put in the
    // same equivalence class.
    for (std::vector<DSCallSite>::const_iterator CSI = callSites.begin(), 
           CSE = callSites.end(); CSI != CSE ; ++CSI)
      if (CSI->isIndirectCall()) {
        Instruction *TheCall = CSI->getCallSite().getInstruction();
        DSNode *DSN = CSI->getCalleeNode();
        std::cerr << "INDCALL: " << *TheCall;
        if (DSN->isIncomplete())
          std::cerr << "Incomplete node: " << *TheCall;
        // assert(DSN->isGlobalNode());
        const std::vector<GlobalValue*> &Callees = DSN->getGlobals();
        if (Callees.empty())
          std::cerr << "No targets: " << *TheCall;
        Function *RunningClass = 0;
        for (unsigned i = 0, e = Callees.size(); i != e; ++i)
          if (Function *F = dyn_cast<Function>(Callees[i])) {
            OneCalledFunction[TheCall] = F;
            if (RunningClass == 0)
              FuncECs.addElement(RunningClass = F);
            else
              FuncECs.unionSetsWith(RunningClass, F);
          }
      }
  }
#endif

  // Now that all of the equivalences have been built, turn the union-find data
  // structure into a simple map from each function in the equiv class to the
  // DSGraph used to represent the union of graphs.
  //
  std::map<Function*, Function*> &leaderMap = FuncECs.getLeaderMap();
  DEBUG(std::cerr << "Indirect Function Equivalence Map:\n");
  for (std::map<Function*, Function*>::iterator LI = leaderMap.begin(),
	 LE = leaderMap.end(); LI != LE; ++LI) {
    DEBUG(std::cerr << "  " << LI->first->getName() << ": leader is "
                    << LI->second->getName() << "\n");

    // Now the we have the equiv class info for this object, merge the DSGraph
    // for this function into the composite DSGraph.
    EquivClassInfo &ECI = ECInfoForLeadersMap[LI->second];

    // If this is the first function in this equiv class, create the graph now.
    if (ECI.G == 0)
      ECI.G = new DSGraph(BU->getGlobalsGraph().getTargetData());

    // Clone this member of the equivalence class into ECI.
    DSGraph::NodeMapTy NodeMap;    
    ECI.G->cloneInto(BU->getDSGraph(*LI->first), ECI.G->getScalarMap(),
                     ECI.G->getReturnNodes(), NodeMap, 0);

    // This is N^2 with the number of functions in this equiv class, but I don't
    // really care right now.  FIXME!
    for (unsigned i = 0, e = ECI.FuncsInClass.size(); i != e; ++i) {
      Function *F = ECI.FuncsInClass[i];
      // Merge the return nodes together.
      ECI.G->getReturnNodes()[F].mergeWith(ECI.G->getReturnNodes()[LI->first]);

      // Merge the arguments of the functions together.
      Function::aiterator AI1 = F->abegin();
      Function::aiterator AI2 = LI->first->abegin();
      for (; AI1 != F->aend() && AI2 != LI->first->aend(); ++AI1,++AI2)
        ECI.G->getNodeForValue(AI1).mergeWith(ECI.G->getNodeForValue(AI2));
    }
    ECI.FuncsInClass.push_back(LI->first);
  }
  DEBUG(std::cerr << "\n");

  // Now that we have created the graphs for each of the equivalence sets, we
  // have to figure out which pool descriptors to pass into the functions.  We
  // must pass arguments for any pool descriptor that is needed by any function
  // in the equiv. class.
  for (ECInfoForLeadersMapTy::iterator ECII = ECInfoForLeadersMap.begin(),
         E = ECInfoForLeadersMap.end(); ECII != E; ++ECII) {
    EquivClassInfo &ECI = ECII->second;

    // Traverse part of the graph to provoke most of the node forwardings to
    // occur.
    DSGraph::ScalarMapTy &SM = ECI.G->getScalarMap();
    for (DSGraph::ScalarMapTy::iterator I = SM.begin(), E = SM.end(); I!=E; ++I)
      I->second.getNode();   // Collapse forward references...

    // Remove breadcrumbs from merging nodes.
    ECI.G->removeTriviallyDeadNodes();

    // Loop over all of the functions in this equiv class, figuring out which
    // pools must be passed in for each function.
    for (unsigned i = 0, e = ECI.FuncsInClass.size(); i != e; ++i) {
      Function *F = ECI.FuncsInClass[i];
      DSGraph &FG = BU->getDSGraph(*F);

      // Figure out which nodes need to be passed in for this function (if any)
      hash_set<DSNode*> &MarkedNodes = FunctionInfo[F].MarkedNodes;
      MarkNodesWhichMustBePassedIn(MarkedNodes, *F, FG);

      if (!MarkedNodes.empty()) {
        // If any nodes need to be passed in, figure out which nodes these are
        // in the unified graph for this equivalence class.
        DSGraph::NodeMapTy NodeMapping;
        for (Function::aiterator I = F->abegin(), E = F->aend(); I != E; ++I)
          DSGraph::computeNodeMapping(FG.getNodeForValue(I),
                                      ECI.G->getNodeForValue(I), NodeMapping);
        DSGraph::computeNodeMapping(FG.getReturnNodeFor(*F),
                                    ECI.G->getReturnNodeFor(*F), NodeMapping);
        
        // Loop through all of the nodes which must be passed through for this
        // callee, and add them to the arguments list.
        for (hash_set<DSNode*>::iterator I = MarkedNodes.begin(),
               E = MarkedNodes.end(); I != E; ++I) {
          DSNode *LocGraphNode = *I, *ECIGraphNode = NodeMapping[*I].getNode();

          // Remember the mapping of this node
          ECI.ECGraphToPrivateMap[std::make_pair(F,ECIGraphNode)] =LocGraphNode;

          // Add this argument to be passed in.  Don't worry about duplicates,
          // they will be eliminated soon.
          ECI.ArgNodes.push_back(ECIGraphNode);
        }
      }
    }

    // Okay, all of the functions have had their required nodes added to the
    // ECI.ArgNodes list, but there might be duplicates.  Eliminate the dups
    // now.
    std::sort(ECI.ArgNodes.begin(), ECI.ArgNodes.end());
    ECI.ArgNodes.erase(std::unique(ECI.ArgNodes.begin(), ECI.ArgNodes.end()),
                       ECI.ArgNodes.end());
    
    // Uncomment this if you want to see the aggregate graph result
    //ECI.G->viewGraph();
  }
}



// SetupGlobalPools - Create global pools for all DSNodes in the globals graph
// which contain heap objects.  If a global variable points to a piece of memory
// allocated from the heap, this pool gets a global lifetime.  This is
// implemented by making the pool descriptor be a global variable of it's own,
// and initializing the pool on entrance to main.  Note that we never destroy
// the pool, because it has global lifetime.
//
// This method returns true if correct pool allocation of the module cannot be
// performed because there is no main function for the module and there are
// global pools.
//
bool PoolAllocate::SetupGlobalPools(Module &M) {
  // Get the globals graph for the program.
  DSGraph &GG = BU->getGlobalsGraph();

  // Get all of the nodes reachable from globals.
  hash_set<DSNode*> GlobalHeapNodes;
  GetNodesReachableFromGlobals(GG, GlobalHeapNodes);

  // Filter out all nodes which have no heap allocations merged into them.
  for (hash_set<DSNode*>::iterator I = GlobalHeapNodes.begin(),
         E = GlobalHeapNodes.end(); I != E; ) {
    hash_set<DSNode*>::iterator Last = I++;
    if (!(*Last)->isHeapNode())
      GlobalHeapNodes.erase(Last);
  }
  
  // If we don't need to create any global pools, exit now.
  if (GlobalHeapNodes.empty()) return false;

  // Otherwise get the main function to insert the poolinit calls.
  Function *MainFunc = M.getMainFunction();
  if (MainFunc == 0 || MainFunc->isExternal()) {
    std::cerr << "Cannot pool allocate this program: it has global "
              << "pools but no 'main' function yet!\n";
    return true;
  }

  BasicBlock::iterator InsertPt = MainFunc->getEntryBlock().begin();
  while (isa<AllocaInst>(InsertPt)) ++InsertPt;
  
  TargetData &TD = getAnalysis<TargetData>();

  // Loop over all of the pools, creating a new global pool descriptor,
  // inserting a new entry in GlobalNodes, and inserting a call to poolinit in
  // main.
  for (hash_set<DSNode*>::iterator I = GlobalHeapNodes.begin(),
         E = GlobalHeapNodes.end(); I != E; ++I) {
    GlobalVariable *GV =
      new GlobalVariable(PoolDescType, false, GlobalValue::InternalLinkage, 
                         Constant::getNullValue(PoolDescType), "GlobalPool",&M);
    GlobalNodes[*I] = GV;

    Value *ElSize =
      ConstantUInt::get(Type::UIntTy, (*I)->getType()->isSized() ? 
                        TD.getTypeSize((*I)->getType()) : 4);
    new CallInst(PoolInit, make_vector((Value*)GV, ElSize, 0), "", InsertPt);
  }

  return false;
}

void PoolAllocate::FindFunctionPoolArgs(Function &F) {
  DSGraph &G = BU->getDSGraph(F);

  FuncInfo &FI = FunctionInfo[&F];   // Create a new entry for F
  hash_set<DSNode*> &MarkedNodes = FI.MarkedNodes;

  // If this function is part of an indirectly called function equivalence
  // class, we have to handle it specially.
  if (Function *Leader = FuncECs.findClass(&F)) {
    EquivClassInfo &ECI = ECInfoForLeadersMap[Leader];

    // The arguments passed in will be the ones specified by the ArgNodes list.
    for (unsigned i = 0, e = ECI.ArgNodes.size(); i != e; ++i) {
      DSNode *ArgNode =
        ECI.ECGraphToPrivateMap[std::make_pair(&F, ECI.ArgNodes[i])];
      FI.ArgNodes.push_back(ArgNode);
      MarkedNodes.insert(ArgNode);
    }

    return;
  }

  if (G.getNodes().empty())
    return;  // No memory activity, nothing is required

  // Find DataStructure nodes which are allocated in pools non-local to the
  // current function.  This set will contain all of the DSNodes which require
  // pools to be passed in from outside of the function.
  MarkNodesWhichMustBePassedIn(MarkedNodes, F, G);
  
  FI.ArgNodes.insert(FI.ArgNodes.end(), MarkedNodes.begin(), MarkedNodes.end());
}

// MakeFunctionClone - If the specified function needs to be modified for pool
// allocation support, make a clone of it, adding additional arguments as
// necessary, and return it.  If not, just return null.
//
Function *PoolAllocate::MakeFunctionClone(Function &F) {
  DSGraph &G = BU->getDSGraph(F);
  std::vector<DSNode*> &Nodes = G.getNodes();
  if (Nodes.empty()) return 0;
    
  FuncInfo &FI = FunctionInfo[&F];
  if (FI.ArgNodes.empty())
    return 0;           // No need to clone if no pools need to be passed in!

  // Update statistics..
  NumArgsAdded += FI.ArgNodes.size();
  ++NumCloned;
 
      
  // Figure out what the arguments are to be for the new version of the function
  const FunctionType *OldFuncTy = F.getFunctionType();
  std::vector<const Type*> ArgTys(FI.ArgNodes.size(), PoolDescPtrTy);
  ArgTys.reserve(OldFuncTy->getParamTypes().size() + FI.ArgNodes.size());

  ArgTys.insert(ArgTys.end(), OldFuncTy->getParamTypes().begin(),
                OldFuncTy->getParamTypes().end());


  // Create the new function prototype
  FunctionType *FuncTy = FunctionType::get(OldFuncTy->getReturnType(), ArgTys,
                                           OldFuncTy->isVarArg());
  // Create the new function...
  Function *New = new Function(FuncTy, GlobalValue::InternalLinkage,
                               F.getName(), F.getParent());

  // Set the rest of the new arguments names to be PDa<n> and add entries to the
  // pool descriptors map
  std::map<DSNode*, Value*> &PoolDescriptors = FI.PoolDescriptors;
  Function::aiterator NI = New->abegin();
  
  for (unsigned i = 0, e = FI.ArgNodes.size(); i != e; ++i, ++NI) {
    NI->setName("PDa");
    PoolDescriptors[FI.ArgNodes[i]] = NI;
  }

  // Map the existing arguments of the old function to the corresponding
  // arguments of the new function, and copy over the names.
  std::map<const Value*, Value*> ValueMap;
  for (Function::aiterator I = F.abegin(); NI != New->aend(); ++I, ++NI) {
    ValueMap[I] = NI;
    NI->setName(I->getName());
  }

  // Populate the value map with all of the globals in the program.
  // FIXME: This should be unnecessary!
  Module &M = *F.getParent();
  for (Module::iterator I = M.begin(), E=M.end(); I!=E; ++I)    ValueMap[I] = I;
  for (Module::giterator I = M.gbegin(), E=M.gend(); I!=E; ++I) ValueMap[I] = I;

  // Perform the cloning.
  std::vector<ReturnInst*> Returns;
  CloneFunctionInto(New, &F, ValueMap, Returns);

  // Invert the ValueMap into the NewToOldValueMap
  std::map<Value*, const Value*> &NewToOldValueMap = FI.NewToOldValueMap;
  for (std::map<const Value*, Value*>::iterator I = ValueMap.begin(),
         E = ValueMap.end(); I != E; ++I)
    NewToOldValueMap.insert(std::make_pair(I->second, I->first));
  
  return FI.Clone = New;
}


// CreatePools - This creates the pool initialization and destruction code for
// the DSNodes specified by the NodesToPA list.  This adds an entry to the
// PoolDescriptors map for each DSNode.
//
void PoolAllocate::CreatePools(Function &F,
                               const std::vector<DSNode*> &NodesToPA,
                               std::map<DSNode*, Value*> &PoolDescriptors) {

  // Loop over all of the pools, inserting code into the entry block of the
  // function for the initialization and code in the exit blocks for
  // destruction.
  //
  Instruction *InsertPoint = F.front().begin();
  for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i) {
    DSNode *Node = NodesToPA[i];
    
    // Create a new alloca instruction for the pool...
    Value *AI = new AllocaInst(PoolDescType, 0, "PD", InsertPoint);

    // Void types in DS graph are never used
    if (Node->getType() == Type::VoidTy)
      std::cerr << "Node collapsing in '" << F.getName() << "'\n";
    ++NumPools;
      
    // Update the PoolDescriptors map
    PoolDescriptors.insert(std::make_pair(Node, AI));
  }
}



// processFunction - Pool allocate any data structures which are contained in
// the specified function...
//
void PoolAllocate::ProcessFunctionBody(Function &F, Function &NewF) {
  DSGraph &G = BU->getDSGraph(F);

  std::vector<DSNode*> &Nodes = G.getNodes();
  if (Nodes.empty()) return;     // Quick exit if nothing to do...
  
  FuncInfo &FI = FunctionInfo[&F];   // Get FuncInfo for F
  hash_set<DSNode*> &MarkedNodes = FI.MarkedNodes;
  
  DEBUG(std::cerr << "[" << F.getName() << "] Pool Allocate: ");

  // Calculate which DSNodes are reachable from globals.  If a node is reachable
  // from a global, we will create a global pool for it, so no argument passage
  // is required.
  DSGraph &GG = BU->getGlobalsGraph();
  DSGraph::NodeMapTy GlobalsGraphNodeMapping;
  for (DSGraph::ScalarMapTy::iterator I = G.getScalarMap().begin(), 
         E = G.getScalarMap().end(); I != E; ++I)
    if (GlobalValue *GV = dyn_cast<GlobalValue>(I->first)) {
      // Map all node reachable from this global to the corresponding nodes in
      // the globals graph.
      DSGraph::computeNodeMapping(I->second.getNode(), GG.getNodeForValue(GV),
                                  GlobalsGraphNodeMapping);
    }
  
  // Loop over all of the nodes which are non-escaping, adding pool-allocatable
  // ones to the NodesToPA vector.
  std::vector<DSNode*> NodesToPA;
  for (unsigned i = 0, e = Nodes.size(); i != e; ++i)
    // We only need to make a pool if there is a heap object in it...
    if (Nodes[i]->isHeapNode())
      if (GlobalsGraphNodeMapping.count(Nodes[i])) {
        // If it is a global pool, set up the pool descriptor appropriately.
        DSNode *GGN = GlobalsGraphNodeMapping[Nodes[i]].getNode();
        assert(GGN && GlobalNodes[GGN] && "No global node found??");
        FI.PoolDescriptors[Nodes[i]] = GlobalNodes[GGN];
      } else if (!MarkedNodes.count(Nodes[i])) {
        // Otherwise, if it was not passed in from outside the function, it must
        // be a local pool!
        NodesToPA.push_back(Nodes[i]);
      }
  
  DEBUG(std::cerr << NodesToPA.size() << " nodes to pool allocate\n");
  if (!NodesToPA.empty())    // Insert pool alloca's
    CreatePools(NewF, NodesToPA, FI.PoolDescriptors);
  
  // Transform the body of the function now... collecting information about uses
  // of the pools.
  std::set<std::pair<AllocaInst*, Instruction*> > PoolUses;
  std::set<std::pair<AllocaInst*, CallInst*> > PoolFrees;
  TransformBody(G, TDDS->getDSGraph(F), FI, PoolUses, PoolFrees, NewF);

  // Create pool construction/destruction code
  if (!NodesToPA.empty())
    InitializeAndDestroyPools(NewF, NodesToPA, FI.PoolDescriptors,
                              PoolUses, PoolFrees);
}

template<class IteratorTy>
static void AllOrNoneInSet(IteratorTy S, IteratorTy E,
                           std::set<BasicBlock*> &Blocks, bool &AllIn,
                           bool &NoneIn) {
  AllIn = true;
  NoneIn = true;
  for (; S != E; ++S)
    if (Blocks.count(*S))
      NoneIn = false;
    else
      AllIn = false;
}

static void DeleteIfIsPoolFree(Instruction *I, AllocaInst *PD,
                    std::set<std::pair<AllocaInst*, CallInst*> > &PoolFrees) {
  if (CallInst *CI = dyn_cast<CallInst>(I))
    if (PoolFrees.count(std::make_pair(PD, CI))) {
      PoolFrees.erase(std::make_pair(PD, CI));
      I->getParent()->getInstList().erase(I);
      ++NumPoolFree;
    }
}

void PoolAllocate::CalculateLivePoolFreeBlocks(std::set<BasicBlock*>&LiveBlocks,
                                               Value *PD) {
  for (Value::use_iterator I = PD->use_begin(), E = PD->use_end(); I != E; ++I){
    // The only users of the pool should be call & invoke instructions.
    CallSite U = CallSite::get(*I);
    if (U.getCalledValue() != PoolFree && U.getCalledValue() != PoolDestroy) {
      // This block and every block that can reach this block must keep pool
      // frees.
      for (idf_ext_iterator<BasicBlock*, std::set<BasicBlock*> >
             DI = idf_ext_begin(U.getInstruction()->getParent(), LiveBlocks),
             DE = idf_ext_end(U.getInstruction()->getParent(), LiveBlocks);
           DI != DE; ++DI)
        /* empty */;
    }
  }
}


/// InitializeAndDestroyPools - This inserts calls to poolinit and pooldestroy
/// into the function to initialize and destroy the pools in the NodesToPA list.
///
void PoolAllocate::InitializeAndDestroyPools(Function &F,
                               const std::vector<DSNode*> &NodesToPA,
                                 std::map<DSNode*, Value*> &PoolDescriptors,
                     std::set<std::pair<AllocaInst*, Instruction*> > &PoolUses,
                     std::set<std::pair<AllocaInst*, CallInst*> > &PoolFrees) {
  TargetData &TD = getAnalysis<TargetData>();

  std::vector<Instruction*> PoolInitPoints;
  std::vector<Instruction*> PoolDestroyPoints;

  if (DisableInitDestroyOpt) {
    // Insert poolinit calls after all of the allocas...
    Instruction *InsertPoint;
    for (BasicBlock::iterator I = F.front().begin();
         isa<AllocaInst>(InsertPoint = I); ++I)
      /*empty*/;
    PoolInitPoints.push_back(InsertPoint);

    for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
      if (isa<ReturnInst>(BB->getTerminator()))
        PoolDestroyPoints.push_back(BB->getTerminator());
  }

  // Insert all of the poolalloc calls in the start of the function.
  for (unsigned i = 0, e = NodesToPA.size(); i != e; ++i) {
    DSNode *Node = NodesToPA[i];
    
    Value *ElSize =
      ConstantUInt::get(Type::UIntTy, Node->getType()->isSized() ? 
                        TD.getTypeSize(Node->getType()) : 4);
    
    AllocaInst *PD = cast<AllocaInst>(PoolDescriptors[Node]);

    // Convert the PoolUses/PoolFrees sets into something specific to this pool.
    std::set<BasicBlock*> UsingBlocks;

    std::set<std::pair<AllocaInst*, Instruction*> >::iterator PUI =
      PoolUses.lower_bound(std::make_pair(PD, (Instruction*)0));
    if (PUI != PoolUses.end() && PUI->first < PD) ++PUI;
    for (; PUI != PoolUses.end() && PUI->first == PD; ++PUI)
      UsingBlocks.insert(PUI->second->getParent());

    // To calculate all of the basic blocks which require the pool to be
    // initialized before, do a depth first search on the CFG from the using
    // blocks.
    std::set<BasicBlock*> InitializedBefore;
    std::set<BasicBlock*> DestroyedAfter;
    for (std::set<BasicBlock*>::iterator I = UsingBlocks.begin(),
           E = UsingBlocks.end(); I != E; ++I) {
      for (df_ext_iterator<BasicBlock*, std::set<BasicBlock*> >
             DI = df_ext_begin(*I, InitializedBefore),
             DE = df_ext_end(*I, InitializedBefore); DI != DE; ++DI)
        /* empty */;

      for (idf_ext_iterator<BasicBlock*, std::set<BasicBlock*> >
             DI = idf_ext_begin(*I, DestroyedAfter),
             DE = idf_ext_end(*I, DestroyedAfter); DI != DE; ++DI)
        /* empty */;
    }

    // Now that we have created the sets, intersect them.
    std::set<BasicBlock*> LiveBlocks;
    std::set_intersection(InitializedBefore.begin(), InitializedBefore.end(),
                          DestroyedAfter.begin(), DestroyedAfter.end(),
                          std::inserter(LiveBlocks, LiveBlocks.end()));
    InitializedBefore.clear();
    DestroyedAfter.clear();

    // Keep track of the blocks we have inserted poolinit/destroy in
    std::set<BasicBlock*> PoolInitInsertedBlocks, PoolDestroyInsertedBlocks;

    DEBUG(std::cerr << "POOL: " << PD->getName() << " information:\n");
    DEBUG(std::cerr << "  Live in blocks: ");
    for (std::set<BasicBlock*>::iterator I = LiveBlocks.begin(),
           E = LiveBlocks.end(); I != E; ++I) {
      BasicBlock *BB = *I;
      TerminatorInst *Term = BB->getTerminator();
      DEBUG(std::cerr << BB->getName() << " ");
      
      // Check the predecessors of this block.  If any preds are not in the
      // set, or if there are no preds, insert a pool init.
      bool AllIn, NoneIn;
      AllOrNoneInSet(pred_begin(BB), pred_end(BB), LiveBlocks, AllIn, NoneIn);

      if (NoneIn) {
        if (!PoolInitInsertedBlocks.count(BB)) {
          BasicBlock::iterator It = BB->begin();
          // Move through all of the instructions not in the pool
          while (!PoolUses.count(std::make_pair(PD, It)))
            // Advance past non-users deleting any pool frees that we run across
            DeleteIfIsPoolFree(It++, PD, PoolFrees);
          PoolInitPoints.push_back(It);
          PoolInitInsertedBlocks.insert(BB);
        }
      } else if (!AllIn) {
      TryAgainPred:
        for (pred_iterator PI = pred_begin(BB), E = pred_end(BB); PI != E; ++PI)
          if (!LiveBlocks.count(*PI) && !PoolInitInsertedBlocks.count(*PI)) {
            if (SplitCriticalEdge(BB, PI))
              // If the critical edge was split, *PI was invalidated
              goto TryAgainPred;

            // Insert at the end of the predecessor, before the terminator.
            PoolInitPoints.push_back((*PI)->getTerminator());
            PoolInitInsertedBlocks.insert(*PI);
          }
      }

      // Check the successors of this block.  If some succs are not in the set,
      // insert destroys on those successor edges.  If all succs are not in the
      // set, insert a destroy in this block.
      AllOrNoneInSet(succ_begin(BB), succ_end(BB), LiveBlocks, AllIn, NoneIn);

      if (NoneIn) {
        // Insert before the terminator.
        if (!PoolDestroyInsertedBlocks.count(BB)) {
          BasicBlock::iterator It = Term;
          
          // Rewind to the first using insruction
          while (!PoolUses.count(std::make_pair(PD, It)))
            DeleteIfIsPoolFree(It--, PD, PoolFrees);

          // Insert after the first using instruction
          PoolDestroyPoints.push_back(++It);
          PoolDestroyInsertedBlocks.insert(BB);
        }
      } else if (!AllIn) {
        for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI)
          if (!LiveBlocks.count(*SI) && !PoolDestroyInsertedBlocks.count(*SI)) {
            // If this edge is critical, split it.
            SplitCriticalEdge(BB, SI);

            // Insert at entry to the successor, but after any PHI nodes.
            BasicBlock::iterator It = (*SI)->begin();
            while (isa<PHINode>(It)) ++It;
            PoolDestroyPoints.push_back(It);
            PoolDestroyInsertedBlocks.insert(*SI);
          }
      }
    }
    DEBUG(std::cerr << "\n  Init in blocks: ");

    // Insert the calls to initialize the pool...
    for (unsigned i = 0, e = PoolInitPoints.size(); i != e; ++i) {
      new CallInst(PoolInit, make_vector((Value*)PD, ElSize, 0), "",
                   PoolInitPoints[i]);
      DEBUG(std::cerr << PoolInitPoints[i]->getParent()->getName() << " ");
    }
    PoolInitPoints.clear();

    DEBUG(std::cerr << "\n  Destroy in blocks: ");

    // Loop over all of the places to insert pooldestroy's...
    for (unsigned i = 0, e = PoolDestroyPoints.size(); i != e; ++i) {
      // Insert the pooldestroy call for this pool.
      new CallInst(PoolDestroy, make_vector((Value*)PD, 0), "",
                   PoolDestroyPoints[i]);
      DEBUG(std::cerr << PoolDestroyPoints[i]->getParent()->getName() << " ");
    }
    DEBUG(std::cerr << "\n\n");

    // We are allowed to delete any pool frees which occur between the last call
    // to poolalloc, and the call to pooldestroy.  Figure out which basic blocks
    // have this property for this pool.
    std::set<BasicBlock*> PoolFreeLiveBlocks;
    if (!DisableInitDestroyOpt)
      CalculateLivePoolFreeBlocks(PoolFreeLiveBlocks, PD);
    else
      PoolFreeLiveBlocks = LiveBlocks;
    PoolDestroyPoints.clear();

    // Delete any pool frees which are not in live blocks, for correctness.
    std::set<std::pair<AllocaInst*, CallInst*> >::iterator PFI =
      PoolFrees.lower_bound(std::make_pair(PD, (CallInst*)0));
    if (PFI != PoolFrees.end() && PFI->first < PD) ++PFI;
    for (; PFI != PoolFrees.end() && PFI->first == PD; ) {
      CallInst *PoolFree = (PFI++)->second;
      if (!LiveBlocks.count(PoolFree->getParent()) ||
          !PoolFreeLiveBlocks.count(PoolFree->getParent()))
        DeleteIfIsPoolFree(PoolFree, PD, PoolFrees);
    }
  }
}
