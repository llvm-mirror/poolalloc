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
#include "Support/VectorExtras.h"
#include "Support/Statistic.h"
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

  AddPoolPrototypes();
  BuildIndirectFunctionSets(M);

  if (SetupGlobalPools(M))
    return true;

  // Loop over the functions in the original program finding the pool desc.
  // arguments necessary for each function that is indirectly callable.
  // For each equivalence class, make a list of pool arguments and update
  // the PoolArgFirst and PoolArgLast values for each function.
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


// Prints out the functions mapped to the leader of the equivalence class they
// belong to.
void PoolAllocate::printFuncECs() {
  std::map<Function*, Function*> &leaderMap = FuncECs.getLeaderMap();
  std::cerr << "Indirect Function Map \n";
  for (std::map<Function*, Function*>::iterator LI = leaderMap.begin(),
	 LE = leaderMap.end(); LI != LE; ++LI) {
    std::cerr << LI->first->getName() << ": leader is "
	      << LI->second->getName() << "\n";
  }
}

static void printNTOMap(std::map<Value*, const Value*> &NTOM) {
  std::cerr << "NTOM MAP\n";
  for (std::map<Value*, const Value *>::iterator I = NTOM.begin(), 
	 E = NTOM.end(); I != E; ++I) {
    if (!isa<Function>(I->first) && !isa<BasicBlock>(I->first))
      std::cerr << *I->first << " to " << *I->second << "\n";
  }
}

// BuildIndirectFunctionSets - Iterate over the module looking for indirect
// calls to functions
void PoolAllocate::BuildIndirectFunctionSets(Module &M) {
  // Get top down DSGraph for the functions
  TDDS = &getAnalysis<TDDataStructures>();
  
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    DEBUG(std::cerr << "Processing indirect calls function:" <<  MI->getName()
                    << "\n");

    if (MI->isExternal())
      continue;

    DSGraph &TDG = TDDS->getDSGraph(*MI);

    const std::vector<DSCallSite> &callSites = TDG.getFunctionCalls();

    // For each call site in the function
    // All the functions that can be called at the call site are put in the
    // same equivalence class.
    for (std::vector<DSCallSite>::const_iterator CSI = callSites.begin(), 
	   CSE = callSites.end(); CSI != CSE ; ++CSI) {
      if (CSI->isIndirectCall()) {
	DSNode *DSN = CSI->getCalleeNode();
	if (DSN->isIncomplete())
	  std::cerr << "Incomplete node: "
                    << *CSI->getCallSite().getInstruction();
	// assert(DSN->isGlobalNode());
	const std::vector<GlobalValue*> &Callees = DSN->getGlobals();
	if (Callees.empty())
	  std::cerr << "No targets: " << *CSI->getCallSite().getInstruction();
        Function *RunningClass = 0;
        for (std::vector<GlobalValue*>::const_iterator CalleesI = 
               Callees.begin(), CalleesE = Callees.end(); 
             CalleesI != CalleesE; ++CalleesI)
          if (Function *calledF = dyn_cast<Function>(*CalleesI)) {
            CallSiteTargets.insert(std::make_pair(CSI->getCallSite(), calledF));
            if (RunningClass == 0) {
              RunningClass = calledF;
              FuncECs.addElement(RunningClass);
            } else {
              FuncECs.unionSetsWith(RunningClass, calledF);
            }
          }
      }
    }
  }
  
  // Print the equivalence classes
  DEBUG(printFuncECs());
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




// Inline the DSGraphs of functions corresponding to the potential targets at
// indirect call sites into the DS Graph of the callee.
// This is required to know what pools to create/pass at the call site in the 
// caller
//
void PoolAllocate::InlineIndirectCalls(Function &F, DSGraph &G, 
                                       hash_set<Function*> &visited) {
  const std::vector<DSCallSite> &callSites = G.getFunctionCalls();
  
  visited.insert(&F);

  // For each indirect call site in the function, inline all the potential
  // targets
  for (std::vector<DSCallSite>::const_iterator CSI = callSites.begin(),
         CSE = callSites.end(); CSI != CSE; ++CSI) {
    if (CSI->isIndirectCall()) {
      CallSite CS = CSI->getCallSite();
      std::pair<std::multimap<CallSite, Function*>::iterator,
        std::multimap<CallSite, Function*>::iterator> Targets =
        CallSiteTargets.equal_range(CS);
      for (std::multimap<CallSite, Function*>::iterator TFI = Targets.first,
             TFE = Targets.second; TFI != TFE; ++TFI)
        if (!TFI->second->isExternal()) {
          DSGraph &TargetG = BU->getDSGraph(*TFI->second);
          // Call the function recursively if the callee is not yet inlined and
          // if it hasn't been visited in this sequence of calls The latter is
          // dependent on the fact that the graphs of all functions in an SCC
          // are actually the same
          if (InlinedFuncs.find(TFI->second) == InlinedFuncs.end() && 
              visited.find(TFI->second) == visited.end()) {
            InlineIndirectCalls(*TFI->second, TargetG, visited);
          }
          G.mergeInGraph(*CSI, *TFI->second, TargetG, DSGraph::KeepModRefBits | 
                         DSGraph::KeepAllocaBit | DSGraph::DontCloneCallNodes |
                         DSGraph::DontCloneAuxCallNodes); 
        }
    }
  }
  
  // Mark this function as one whose graph is inlined with its indirect 
  // function targets' DS Graphs.  This ensures that every function is inlined
  // exactly once
  InlinedFuncs.insert(&F);
}

void PoolAllocate::FindFunctionPoolArgs(Function &F) {
  DSGraph &G = BU->getDSGraph(F);
 
  // Inline the potential targets of indirect calls
  hash_set<Function*> visitedFuncs;
  InlineIndirectCalls(F, G, visitedFuncs);

  // At this point the DS Graphs have been modified in place including
  // information about indirect calls, making it useful for pool allocation.
  std::vector<DSNode*> &Nodes = G.getNodes();
  if (Nodes.empty()) return;  // No memory activity, nothing is required

  FuncInfo &FI = FunctionInfo[&F];   // Create a new entry for F
  
  FI.Clone = 0;
  
  // Initialize the PoolArgFirst and PoolArgLast for the function depending on
  // whether there have been other functions in the equivalence class that have
  // pool arguments so far in the analysis.
  if (!FuncECs.findClass(&F)) {
    FI.PoolArgFirst = FI.PoolArgLast = 0;
  } else {
    if (EqClass2LastPoolArg.find(FuncECs.findClass(&F)) != 
	EqClass2LastPoolArg.end())
      FI.PoolArgFirst = FI.PoolArgLast = 
	EqClass2LastPoolArg[FuncECs.findClass(&F)] + 1;
    else
      FI.PoolArgFirst = FI.PoolArgLast = 0;
  }
  
  // Find DataStructure nodes which are allocated in pools non-local to the
  // current function.  This set will contain all of the DSNodes which require
  // pools to be passed in from outside of the function.
  hash_set<DSNode*> &MarkedNodes = FI.MarkedNodes;
  
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

  if (MarkedNodes.empty())   // We don't need to clone the function if there
    return;                  // are no incoming arguments to be added.

  FI.PoolArgLast += MarkedNodes.size();

  // Update the equivalence class last pool argument information
  // only if there actually were pool arguments to the function.
  // Also, there is no entry for the Eq. class in EqClass2LastPoolArg
  // if there are no functions in the equivalence class with pool arguments.
  if (FuncECs.findClass(&F) && FI.PoolArgLast != FI.PoolArgFirst)
    EqClass2LastPoolArg[FuncECs.findClass(&F)] = FI.PoolArgLast - 1;
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
  
  hash_set<DSNode*> &MarkedNodes = FI.MarkedNodes;
  
  if (!FuncECs.findClass(&F)) {
    // Not in any equivalence class
    if (MarkedNodes.empty())
      return 0;
  } else {
    // No need to clone if there are no pool arguments in any function in the
    // equivalence class
    if (!EqClass2LastPoolArg.count(FuncECs.findClass(&F)))
      return 0;
  }
      
  // Figure out what the arguments are to be for the new version of the function
  const FunctionType *OldFuncTy = F.getFunctionType();
  std::vector<const Type*> ArgTys;
  if (!FuncECs.findClass(&F)) {
    ArgTys.reserve(OldFuncTy->getParamTypes().size() + MarkedNodes.size());
    FI.ArgNodes.reserve(MarkedNodes.size());
    for (hash_set<DSNode*>::iterator I = MarkedNodes.begin(),
	   E = MarkedNodes.end(); I != E; ++I) {
      ArgTys.push_back(PoolDescPtrTy);    // Add the appropriate # of pool descs
      FI.ArgNodes.push_back(*I);
    }
    if (FI.ArgNodes.empty()) return 0;      // No nodes to be pool allocated!

  } else {
    // This function is a member of an equivalence class and needs to be cloned 
    ArgTys.reserve(OldFuncTy->getParamTypes().size() + 
		   EqClass2LastPoolArg[FuncECs.findClass(&F)] + 1);
    FI.ArgNodes.reserve(EqClass2LastPoolArg[FuncECs.findClass(&F)] + 1);
    
    for (int i = 0; i <= EqClass2LastPoolArg[FuncECs.findClass(&F)]; ++i)
      ArgTys.push_back(PoolDescPtrTy);    // Add the appropriate # of pool descs

    for (hash_set<DSNode*>::iterator I = MarkedNodes.begin(),
	   E = MarkedNodes.end(); I != E; ++I) {
      FI.ArgNodes.push_back(*I);
    }

    assert((FI.ArgNodes.size() == (unsigned)(FI.PoolArgLast-FI.PoolArgFirst)) &&
           "Number of ArgNodes equal to the number of pool arguments used by "
           "this function");

    if (FI.ArgNodes.empty()) return 0;
  }
      
  NumArgsAdded += ArgTys.size();
  ++NumCloned;

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
  
  if (FuncECs.findClass(&F)) {
    // If the function belongs to an equivalence class
    for (int i = 0; i <= EqClass2LastPoolArg[FuncECs.findClass(&F)]; ++i, ++NI)
      NI->setName("PDa");
    
    NI = New->abegin();
    if (FI.PoolArgFirst > 0)
      for (int i = 0; i < FI.PoolArgFirst; ++NI, ++i)
	;

    for (unsigned i = 0, e = FI.ArgNodes.size(); i != e; ++i, ++NI)
      PoolDescriptors.insert(std::make_pair(FI.ArgNodes[i], NI));

    NI = New->abegin();
    if (EqClass2LastPoolArg.count(FuncECs.findClass(&F)))
      for (int i = 0; i <= EqClass2LastPoolArg[FuncECs.findClass(&F)]; ++i,++NI)
	;
  } else {
    // If the function does not belong to an equivalence class
    if (FI.ArgNodes.size())
      for (unsigned i = 0, e = FI.ArgNodes.size(); i != e; ++i, ++NI) {
	NI->setName("PDa");  // Add pd entry
	PoolDescriptors.insert(std::make_pair(FI.ArgNodes[i], NI));
      }
    NI = New->abegin();
    if (FI.ArgNodes.size())
      for (unsigned i = 0; i < FI.ArgNodes.size(); ++NI, ++i)
	;
  }

  // Map the existing arguments of the old function to the corresponding
  // arguments of the new function.
  std::map<const Value*, Value*> ValueMap;
  if (NI != New->aend()) 
    for (Function::aiterator I = F.abegin(), E = F.aend(); I != E; ++I, ++NI) {
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
