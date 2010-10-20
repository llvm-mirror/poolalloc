//===- DataStructure.h - Build data structure graphs ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implement the LLVM data structure analysis library.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_DATA_STRUCTURE_H
#define LLVM_ANALYSIS_DATA_STRUCTURE_H

#include "llvm/Pass.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CallSite.h"
#include "llvm/ADT/EquivalenceClasses.h"

#include "poolalloc/ADT/HashExtras.h"

#include <map>
#include <vector>
#include <algorithm>

namespace llvm {

class Type;
class Instruction;
class GlobalValue;
class DSGraph;
class DSCallSite;
class DSNode;
class DSNodeHandle;

FunctionPass *createDataStructureStatsPass();
FunctionPass *createDataStructureGraphCheckerPass();

class InstCallGraph {
  typedef hash_map<const Instruction*, std::vector<const Function*> > ActualCalleesTy;

  // Callgraph
  ActualCalleesTy ActualCallees;

public:

  InstCallGraph() {
    //Dummy node for empty call sites
    ActualCallees[0];
  }

  typedef std::vector<const Function*>::const_iterator iterator;

  void add(const Instruction* I, const Function* F) {
    std::vector<const Function*>& callees = ActualCallees[I];
    std::vector<const Function*>::iterator ii 
      = std::lower_bound(callees.begin(), callees.end(), F);
    if (ii != callees.end() && *ii == F) return;
    callees.insert(ii, F);
  }

  iterator begin(const Instruction *I) const {
    ActualCalleesTy::const_iterator ii = ActualCallees.find(I);
    if (ii == ActualCallees.end())
      ii = ActualCallees.find(0);
    return ii->second.begin();
  }

  iterator end(const Instruction *I) const {
    ActualCalleesTy::const_iterator ii = ActualCallees.find(I);
    if (ii == ActualCallees.end())
      ii = ActualCallees.find(0);
    return ii->second.end();
  }

  unsigned size() const {
    unsigned sum = 0;
    for (ActualCalleesTy::const_iterator ii = ActualCallees.begin(),
           ee = ActualCallees.end(); ii != ee; ++ii)
      sum += ii->second.size();
    return sum;
  }

  template<class OutputIterator>
  void get_keys(OutputIterator keys) {
    for (ActualCalleesTy::const_iterator ii = ActualCallees.begin(),
           ee = ActualCallees.end(); ii != ee; ++ii)
      *keys++ = ii->first;
  }

  void clear() {
    ActualCallees.clear();
  }
};



class DataStructures : public ModulePass {
  typedef hash_map<const Function*, DSGraph*> DSInfoTy;

  /// TargetData, comes in handy
  TargetData* TD;

  /// Pass to get Graphs from
  DataStructures* GraphSource;

  /// Do we clone Graphs or steal them?
  bool Clone;

  /// do we reset the aux list to the func list?
  bool resetAuxCalls;

  /// Were are DSGraphs stolen by another pass?
  bool DSGraphsStolen;

  // DSInfo, one graph for each function
  DSInfoTy DSInfo;

  // Name for printing
  const char* printname;

protected:

  /// The Globals Graph contains all information on the globals
  DSGraph *GlobalsGraph;

  /// GlobalECs - The equivalence classes for each global value that is merged
  /// with other global values in the DSGraphs.
  EquivalenceClasses<const GlobalValue*> GlobalECs;


  void init(DataStructures* D, bool clone, bool useAuxCalls, bool copyGlobalAuxCalls, bool resetAux);
  void init(TargetData* T);

  void formGlobalECs();


  DataStructures(intptr_t id, const char* name) 
    : ModulePass(id), TD(0), GraphSource(0), printname(name), GlobalsGraph(0) {
    
    // For now, the graphs are owned by this pass
    DSGraphsStolen = false;
  }

public:
  /// print - Print out the analysis results...
  ///
  void print(std::ostream &O, const Module *M) const;
  void dumpCallGraph() const;

  typedef InstCallGraph calleeTy;
  calleeTy callee;

  virtual void releaseMemory();

  virtual bool hasDSGraph(const Function* F) const {
    return DSInfo.find(F) != DSInfo.end();
  }

  /// getDSGraph - Return the data structure graph for the specified function.
  ///
  virtual DSGraph *getDSGraph(const Function* F) const {
    hash_map<const Function*, DSGraph*>::const_iterator I = DSInfo.find(F);
    assert(I != DSInfo.end() && "Function not in module!");
    return I->second;
  }

  void setDSGraph(const Function* F, DSGraph* G) {
    DSInfo[F] = G;
  }

  DSGraph* getOrFetchDSGraph(const Function* F);


  DSGraph* getGlobalsGraph() const { return GlobalsGraph; }

  EquivalenceClasses<const GlobalValue*> &getGlobalECs() { return GlobalECs; }

  TargetData& getTargetData() const { return *TD; }

  /// deleteValue/copyValue - Interfaces to update the DSGraphs in the program.
  /// These correspond to the interfaces defined in the AliasAnalysis class.
  void deleteValue(Value *V);
  void copyValue(Value *From, Value *To);
};

// LocalDataStructures - The analysis that computes the local data structure
// graphs for all of the functions in the program.
//
// FIXME: This should be a Function pass that can be USED by a Pass, and would
// be automatically preserved.  Until we can do that, this is a Pass.
//
class LocalDataStructures : public DataStructures {
public:
  static char ID;
  LocalDataStructures() : DataStructures((intptr_t)&ID, "local.") {}
  ~LocalDataStructures() { releaseMemory(); }

  virtual bool runOnModule(Module &M);

  /// getAnalysisUsage - This obviously provides a data structure graph.
  ///
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetData>();
    AU.setPreservesAll();
  }
};

// StdLibDataStructures - This analysis recognizes common standard c library
// functions and generates graphs for them.
class StdLibDataStructures : public DataStructures {
  void eraseCallsTo(Function* F);
public:
  static char ID;
  StdLibDataStructures() : DataStructures((intptr_t)&ID, "stdlib.") {}
  ~StdLibDataStructures() { releaseMemory(); }

  virtual bool runOnModule(Module &M);

  /// getAnalysisUsage - This obviously provides a data structure graph.
  ///
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LocalDataStructures>();
    AU.addPreserved<TargetData>();
    AU.setPreservesCFG();
  }
};

/// BUDataStructures - The analysis that computes the interprocedurally closed
/// data structure graphs for all of the functions in the program.  This pass
/// only performs a "Bottom Up" propagation (hence the name).
///
class BUDataStructures : public DataStructures {
protected:

  // This map is only maintained during construction of BU Graphs
  std::map<std::vector<const Function*>,
           std::pair<DSGraph*, std::vector<DSNodeHandle> > > IndCallGraphMap;

  const char* debugname;
  bool useCallGraph;
  bool ReInlineGlobals;

public:
  static char ID;
  //Child constructor (CBU)
  BUDataStructures(intptr_t CID, const char* name, const char* printname)
    : DataStructures(CID, printname), debugname(name), useCallGraph(true),
      ReInlineGlobals(true) {}
  //main constructor
  BUDataStructures() 
    : DataStructures((intptr_t)&ID, "bu."), debugname("dsa-bu"),
      useCallGraph(false), ReInlineGlobals(false) {}
  ~BUDataStructures() { releaseMemory(); }

  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<StdLibDataStructures>();
    AU.addPreserved<TargetData>();
    AU.setPreservesCFG();
  }

protected:
  bool runOnModuleInternal(Module &M);

private:
  void calculateGraph(DSGraph* G);

  void inlineUnresolved(DSGraph* G);

  unsigned calculateGraphs(const Function *F, 
                           std::vector<const Function*> &Stack,
                           unsigned &NextID,
                           hash_map<const Function*, unsigned> &ValMap);


  void CloneAuxIntoGlobal(DSGraph* G);
  void finalizeGlobals(void);
};

/// CompleteBUDataStructures - This is the exact same as the bottom-up graphs,
/// but we use take a completed call graph and inline all indirect callees into
/// their callers graphs, making the result more useful for things like pool
/// allocation.
///
class CompleteBUDataStructures : public  BUDataStructures {
protected:
  void buildIndirectFunctionSets(Module &M);
public:
  static char ID;
  CompleteBUDataStructures(intptr_t CID = (intptr_t)&ID, 
                           const char* name = "dsa-cbu", 
                           const char* printname = "cbu.")
    : BUDataStructures(CID, name, printname) {}
  ~CompleteBUDataStructures() { releaseMemory(); }

  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<BUDataStructures>();
    AU.addPreserved<TargetData>();
    AU.setPreservesCFG();
  }

};

/// EquivBUDataStructures - This is the same as the complete bottom-up graphs, but
/// with functions partitioned into equivalence classes and a single merged
/// DS graph for all functions in an equivalence class.  After this merging,
/// graphs are inlined bottom-up on the SCCs of the final (CBU) call graph.
///
class EquivBUDataStructures : public CompleteBUDataStructures {
  void mergeGraphsByGlobalECs();
public:
  static char ID;
  EquivBUDataStructures()
    : CompleteBUDataStructures((intptr_t)&ID, "dsa-eq", "eq.") {}
  ~EquivBUDataStructures() { releaseMemory(); }

  virtual bool runOnModule(Module &M);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<CompleteBUDataStructures>();
    AU.addPreserved<TargetData>();
    AU.setPreservesCFG();
  }

};

/// TDDataStructures - Analysis that computes new data structure graphs
/// for each function using the closed graphs for the callers computed
/// by the bottom-up pass.
///
class TDDataStructures : public DataStructures {
  hash_set<const Function*> ArgsRemainIncomplete;

  /// CallerCallEdges - For a particular graph, we keep a list of these records
  /// which indicates which graphs call this function and from where.
  struct CallerCallEdge {
    DSGraph *CallerGraph;        // The graph of the caller function.
    const DSCallSite *CS;        // The actual call site.
    const Function *CalledFunction;    // The actual function being called.

    CallerCallEdge(DSGraph *G, const DSCallSite *cs, const Function *CF)
      : CallerGraph(G), CS(cs), CalledFunction(CF) {}

    bool operator<(const CallerCallEdge &RHS) const {
      return CallerGraph < RHS.CallerGraph ||
            (CallerGraph == RHS.CallerGraph && CS < RHS.CS);
    }
  };

  std::map<DSGraph*, std::vector<CallerCallEdge> > CallerEdges;


  // IndCallMap - We memoize the results of indirect call inlining operations
  // that have multiple targets here to avoid N*M inlining.  The key to the map
  // is a sorted set of callee functions, the value is the DSGraph that holds
  // all of the caller graphs merged together, and the DSCallSite to merge with
  // the arguments for each function.
  std::map<std::vector<const Function*>, DSGraph*> IndCallMap;

  bool useEQBU;

public:
  static char ID;
  TDDataStructures(intptr_t CID = (intptr_t)&ID, const char* printname = "td.", bool useEQ = false)
    : DataStructures(CID, printname), useEQBU(useEQ) {}
  ~TDDataStructures() { releaseMemory(); }

  virtual bool runOnModule(Module &M);

  /// getAnalysisUsage - This obviously provides a data structure graph.
  ///
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    if (useEQBU) {
      AU.addRequired<EquivBUDataStructures>();
    } else {
      AU.addRequired<BUDataStructures>();
      AU.addPreserved<BUDataStructures>();
    }
    AU.addPreserved<TargetData>();
    AU.setPreservesCFG();
  }

private:
  void markReachableFunctionsExternallyAccessible(DSNode *N,
                                                  hash_set<DSNode*> &Visited);

  void InlineCallersIntoGraph(DSGraph* G);
  void ComputePostOrder(const Function* F, hash_set<DSGraph*> &Visited,
                        std::vector<DSGraph*> &PostOrder);
};

/// EQTDDataStructures - Analysis that computes new data structure graphs
/// for each function using the closed graphs for the callers computed
/// by the EQ bottom-up pass.
///
class EQTDDataStructures : public TDDataStructures {
public:
  static char ID;
  EQTDDataStructures()
    :TDDataStructures((intptr_t)&ID, "eqtd.", false)
  {}
};

/// SteensgaardsDataStructures - Analysis that computes a context-insensitive
/// data structure graphs for the whole program.
///
class SteensgaardDataStructures : public DataStructures {
  DSGraph * ResultGraph;
  DataStructures * DS;
  void ResolveFunctionCall(const Function *F, const DSCallSite &Call,
                             DSNodeHandle &RetVal);
  bool runOnModuleInternal(Module &M);

public:
  static char ID;
  SteensgaardDataStructures() : 
    DataStructures((intptr_t)&ID, "steensgaard."),
    ResultGraph(NULL) {}
  ~SteensgaardDataStructures();
  virtual bool runOnModule(Module &M);
  virtual void releaseMemory();

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<TargetData>();
    AU.addRequired<StdLibDataStructures>();
    AU.setPreservesAll();
  }
  
  /// getDSGraph - Return the data structure graph for the specified function.
  ///
  virtual DSGraph *getDSGraph(const Function* F) const {
    return getResultGraph();
  }
  
  virtual bool hasDSGraph(const Function* F) const {
    return true;
  }

  /// getDSGraph - Return the data structure graph for the whole program.
  ///
  DSGraph *getResultGraph() const {
    return ResultGraph;
  }

  void print(OStream O, const Module *M) const;
  void print(std::ostream &O, const Module *M) const;

};


} // End llvm namespace

#endif
