//===- Steensgaard.cpp - Context Insensitive Alias Analysis ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass uses the data structure graphs to implement a simple context
// insensitive alias analysis.  It does this by computing the local analysis
// graphs for all of the functions, then merging them together into a single big
// graph without cloning.
//
//===----------------------------------------------------------------------===//

#include "rdsa/DataStructure.h"
#include "rdsa/DSGraph.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Module.h"
#include "llvm/Support/Debug.h"
#include <ostream>
using namespace llvm;

namespace {
  class Steens : public ModulePass, public AliasAnalysis {
    DSGraph * ResultGraph;
  public:
    static char ID;
    Steens() : ModulePass((intptr_t)&ID), ResultGraph(NULL) {}
    ~Steens() {    }

    //------------------------------------------------
    // Implement the Pass API
    //

    // run - Build up the result graph, representing the pointer graph for the
    // program.
    //
    bool runOnModule(Module &M);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AliasAnalysis::getAnalysisUsage(AU);
      AU.setPreservesAll();                    // Does not transform code...
      AU.addRequired<SteensgaardDataStructures>();   // Uses steensgaard dsgraph
    }

    //------------------------------------------------
    // Implement the AliasAnalysis API
    //

    AliasResult alias(const Value *V1, unsigned V1Size,
                      const Value *V2, unsigned V2Size);

    virtual ModRefResult getModRefInfo(CallSite CS, Value *P, unsigned Size);
    virtual ModRefResult getModRefInfo(CallSite CS1, CallSite CS2);

  };

  // Register the pass...
  RegisterPass<Steens> X("steens-aa",
                         "Steensgaard's alias analysis (DSGraph based)");

  // Register as an implementation of AliasAnalysis
  RegisterAnalysisGroup<AliasAnalysis> Y(X);
}

char Steens::ID;

ModulePass *llvm::createSteensgaardPass() { return new Steens(); }

/// run - Build up the result graph, representing the pointer graph for the
/// program.
///
bool Steens::runOnModule(Module &M) {
  InitializeAliasAnalysis(this);
  ResultGraph = getAnalysis<SteensgaardDataStructures>().getResultGraph();
  return false;
}

AliasAnalysis::AliasResult Steens::alias(const Value *V1, unsigned V1Size,
                                         const Value *V2, unsigned V2Size) {
  assert(ResultGraph && "Result graph has not been computed yet!");

  DSGraph::ScalarMapTy &GSM = ResultGraph->getScalarMap();

  DSGraph::ScalarMapTy::iterator I = GSM.find(const_cast<Value*>(V1));
  DSGraph::ScalarMapTy::iterator J = GSM.find(const_cast<Value*>(V2));
  if (I != GSM.end() && !I->second.isNull() &&
      J != GSM.end() && !J->second.isNull()) {
    DSNodeHandle &V1H = I->second;
    DSNodeHandle &V2H = J->second;

    // If at least one of the nodes is complete, we can say something about
    // this.  If one is complete and the other isn't, then they are obviously
    // different nodes.  If they are both complete, we can't say anything
    // useful.
    if (I->second.getNode()->NodeType.isCompleteNode() ||
        J->second.getNode()->NodeType.isCompleteNode()) {
      // If the two pointers point to different data structure graph nodes, they
      // cannot alias!
      if (V1H.getNode() != V2H.getNode())
        return NoAlias;

      // See if they point to different offsets...  if so, we may be able to
      // determine that they do not alias...
      unsigned O1 = I->second.getOffset(), O2 = J->second.getOffset();
      if (O1 != O2) {
        if (O2 < O1) {    // Ensure that O1 <= O2
          std::swap(V1, V2);
          std::swap(O1, O2);
          std::swap(V1Size, V2Size);
        }

        if (O1+V1Size <= O2)
          return NoAlias;
      }
    }
  }

  // If we cannot determine alias properties based on our graph, fall back on
  // some other AA implementation.
  //
  return AliasAnalysis::alias(V1, V1Size, V2, V2Size);
}

AliasAnalysis::ModRefResult
Steens::getModRefInfo(CallSite CS, Value *P, unsigned Size) {
  AliasAnalysis::ModRefResult Result = ModRef;

  // Find the node in question.
  DSGraph::ScalarMapTy &GSM = ResultGraph->getScalarMap();
  DSGraph::ScalarMapTy::iterator I = GSM.find(P);

  if (I != GSM.end() && !I->second.isNull()) {
    DSNode *N = I->second.getNode();
    if (N->NodeType.isCompleteNode()) {
      // If this is a direct call to an external function, and if the pointer
      // points to a complete node, the external function cannot modify or read
      // the value (we know it's not passed out of the program!).
      if (Function *F = CS.getCalledFunction())
        if (F->isDeclaration())
          return NoModRef;

      // Otherwise, if the node is complete, but it is only M or R, return this.
      // This can be useful for globals that should be marked const but are not.
      if (!N->NodeType.isModifiedNode())
        Result = (ModRefResult)(Result & ~Mod);
      if (!N->NodeType.isReadNode())
        Result = (ModRefResult)(Result & ~Ref);
    }
  }

  return (ModRefResult)(Result & AliasAnalysis::getModRefInfo(CS, P, Size));
}

AliasAnalysis::ModRefResult 
Steens::getModRefInfo(CallSite CS1, CallSite CS2)
{
  return AliasAnalysis::getModRefInfo(CS1,CS2);
}
