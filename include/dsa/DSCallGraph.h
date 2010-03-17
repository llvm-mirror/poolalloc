//===- DSCallGaph.h - Build call graphs -------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implement a detailed call graph for DSA.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DSCALLGRAPH_H
#define	LLVM_DSCALLGRAPH_H

#include "dsa/sv/set.h"
#include <map>

#include "llvm/Function.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/FormattedStream.h"

template<class Iter>
class KeyIterator {
  Iter I;

public:
  typedef typename Iter::difference_type difference_type;
  typedef typename Iter::value_type::first_type value_type;
  typedef typename Iter::value_type::first_type* pointer;
  typedef typename Iter::value_type::first_type& reference;
  typedef typename Iter::iterator_category iterator_category;

  KeyIterator(Iter i) : I(i) { }

  Iter base() const {
    return I;
  }

  reference operator*() const {
    return I->first;
  }

  KeyIterator operator+(difference_type n) const {
    return KeyIterator(I + n);
  }

  KeyIterator & operator++() {
    ++I;
    return *this;
  }

  KeyIterator operator++(int) {
    Iter OI = I;
    ++I;
    return KeyIterator(OI);
  }

  KeyIterator & operator+=(difference_type n) {
    I += n;
    return *this;
  }

  KeyIterator operator-(difference_type n) const {
    return KeyIterator(I - n);
  }

  KeyIterator & operator--() {
    --I;
    return *this;
  }

  KeyIterator operator--(int) {
    Iter OI = I;
    --I;
    return KeyIterator(OI);
  }

  KeyIterator & operator-=(difference_type n) {
    I -= n;
    return *this;
  }

  pointer operator->() const {
    return &I->first;
  }

  bool operator==(const KeyIterator& RHS) const {
    return I == RHS.I;
  }

  bool operator!=(const KeyIterator& RHS) const {
    return I != RHS.I;
  }
};

class DSCallGraph {
public:
  typedef sv::set<const llvm::Function*> FuncSet;
  typedef std::map<llvm::CallSite, FuncSet> ActualCalleesTy;
  typedef std::map<const llvm::Function*, FuncSet> SimpleCalleesTy;

private:
  ActualCalleesTy ActualCallees;
  SimpleCalleesTy SimpleCallees;

  FuncSet EmptyActual;
  FuncSet EmptySimple;

public:

  DSCallGraph() {}

  typedef ActualCalleesTy::mapped_type::const_iterator callee_iterator;
  typedef KeyIterator<ActualCalleesTy::const_iterator> key_iterator;
  typedef SimpleCalleesTy::mapped_type::const_iterator flat_iterator;
  typedef KeyIterator<SimpleCalleesTy::const_iterator> flat_key_iterator;

  void insert(llvm::CallSite CS, const llvm::Function* F) {
    if (F) {
      ActualCallees[CS].insert(F);
      SimpleCallees[CS.getInstruction()->getParent()->getParent()].insert(F);
      //Create an empty set for the callee, hence all called functions get to be
      // in the call graph also.  This simplifies SCC formation
      SimpleCallees[F];
    }
  }

  template<class Iterator>
  void insert(llvm::CallSite CS, Iterator _begin, Iterator _end) {
    for (; _begin != _end; ++_begin)
      insert(CS, *_begin);
  }

  callee_iterator callee_begin(llvm::CallSite CS) const {
    ActualCalleesTy::const_iterator ii = ActualCallees.find(CS);
    if (ii == ActualCallees.end())
      return EmptyActual.end();
    return ii->second.begin();
  }

  callee_iterator callee_end(llvm::CallSite CS) const {
    ActualCalleesTy::const_iterator ii = ActualCallees.find(CS);
    if (ii == ActualCallees.end())
      return EmptyActual.end();
    return ii->second.end();
  }

  key_iterator key_begin() const {
    return key_iterator(ActualCallees.begin());
  }

  key_iterator key_end() const {
    return key_iterator(ActualCallees.end());
  }

  flat_iterator flat_callee_begin(const llvm::Function* F) const {
    SimpleCalleesTy::const_iterator ii = SimpleCallees.find(F);
    if (ii == SimpleCallees.end())
      return EmptySimple.end();
    return ii->second.begin();
  }

  flat_iterator flat_callee_end(const llvm::Function* F) const {
    SimpleCalleesTy::const_iterator ii = SimpleCallees.find(F);
    if (ii == SimpleCallees.end())
      return EmptySimple.end();
    return ii->second.end();
  }

  flat_key_iterator flat_key_begin() const {
    return flat_key_iterator(SimpleCallees.begin());
  }

  flat_key_iterator flat_key_end() const {
    return flat_key_iterator(SimpleCallees.end());
  }


  unsigned callee_size(llvm::CallSite CS) const {
    ActualCalleesTy::const_iterator ii = ActualCallees.find(CS);
    if (ii == ActualCallees.end())
      return 0;
    return ii->second.size();
  }

  unsigned size() const {
    unsigned sum = 0;
    for (ActualCalleesTy::const_iterator ii = ActualCallees.begin(),
            ee = ActualCallees.end(); ii != ee; ++ii)
      sum += ii->second.size();
    return sum;
  }

  void clear() {
    ActualCallees.clear();
  }

};

class DSSCCGraph {
public:
  //SCCs, each element is an SCC
  std::map<unsigned, DSCallGraph::FuncSet> SCCs;
  //mapping of functions in SCCs to SCCs index
  std::map<const llvm::Function*, unsigned> invmap;

  unsigned nextSCC;

  //Functions we know about that aren't called
  sv::set<unsigned> knownRoots;

  std::map<unsigned, sv::set<unsigned> > SCCCallees;
  std::map<unsigned, sv::set<unsigned> > ExtCallees;

  DSCallGraph oldGraph;

private:
  typedef std::map<const llvm::Function*, unsigned> TFMap;
  typedef std::vector<const llvm::Function*> TFStack;

  bool hasPointers(const llvm::Function* F) {
    if (F->isVarArg()) return true;
    if (F->getReturnType()->isPointerTy()) return true;
    for (llvm::Function::const_arg_iterator ii = F->arg_begin(), ee = F->arg_end();
            ii != ee; ++ii)
      if (ii->getType()->isPointerTy()) return true;
    return false;
  }

  unsigned tarjan_rec(const llvm::Function* F, TFStack& Stack, unsigned &NextID,
                      TFMap& ValMap, DSCallGraph& cg) {
    assert(!ValMap.count(F) && "Shouldn't revisit functions!");
    unsigned Min = NextID++, MyID = Min;
    ValMap[F] = Min;
    Stack.push_back(F);

    // The edges out of the current node are the call site targets...
    for (DSCallGraph::flat_iterator ii = cg.flat_callee_begin(F),
            ee = cg.flat_callee_end(F); ii != ee; ++ii) {
      if (hasPointers(*ii) && !(*ii)->isDeclaration()) {
        unsigned M = Min;
        // Have we visited the destination function yet?
        TFMap::iterator It = ValMap.find(*ii);
        if (It == ValMap.end()) // No, visit it now.
          M = tarjan_rec(*ii, Stack, NextID, ValMap, cg);
        else if (std::find(Stack.begin(), Stack.end(), *ii) != Stack.end())
          M = It->second;
        if (M < Min) Min = M;
      }
    }

    assert(ValMap[F] == MyID && "SCC construction assumption wrong!");
    if (Min != MyID)
      return Min; // This is part of a larger SCC!

    // If this is a new SCC, process it now.
    ++nextSCC;

    const llvm::Function* NF = 0;
    do {
      NF = Stack.back();
      Stack.pop_back();
      assert(NF && "Null Function");
      assert(invmap.find(NF) == invmap.end() && "Function already in invmap");
      invmap[NF] = nextSCC;
      assert(SCCs[nextSCC].find(NF) == SCCs[nextSCC].end() &&
             "Function already in SCC");
      SCCs[nextSCC].insert(NF);
    } while (NF != F);

    return MyID;
  }

  void buildSCC(DSCallGraph& DSG) {
    TFStack Stack;
    TFMap ValMap;
    unsigned NextID = 1;

    for (DSCallGraph::flat_key_iterator ii = DSG.flat_key_begin(),
            ee = DSG.flat_key_end(); ii != ee; ++ii)
      if (!ValMap.count(*ii))
        tarjan_rec(*ii, Stack, NextID, ValMap, DSG);
  }

  void buildCallGraph(DSCallGraph& DSG) {
    for(DSCallGraph::flat_key_iterator ii = DSG.flat_key_begin(),
            ee = DSG.flat_key_end(); ii != ee; ++ii) {
      assert (*ii && "Null Function");
      assert(invmap.find(*ii) != invmap.end() && "Unknown Function");
      for (DSCallGraph::flat_iterator fi = DSG.flat_callee_begin(*ii),
              fe = DSG.flat_callee_end(*ii); fi != fe; ++fi) {
        assert(*fi && "Null Function");
        assert(invmap.find(*fi) != invmap.end() && "Unknown Function");
        if (invmap[*ii] != invmap[*fi]) // No self calls
          if (hasPointers(*fi)) {
            if ((*fi)->isDeclaration())
              ExtCallees[invmap[*ii]].insert(invmap[*fi]);
            else
              SCCCallees[invmap[*ii]].insert(invmap[*fi]);
          }
      }
    }
  }

  void buildRoots() {
    sv::set<unsigned> knownCallees;
    sv::set<unsigned> knownCallers;
    for (std::map<unsigned, sv::set<unsigned> >::iterator
      ii = SCCCallees.begin(), ee = SCCCallees.end(); ii != ee; ++ii) {
      knownCallees.insert(ii->second.begin(), ii->second.end());
      knownCallers.insert(ii->first);
    }
    for (sv::set<unsigned>::iterator ii = knownCallers.begin(),
            ee = knownCallers.end(); ii != ee; ++ii)
      if (!knownCallees.count(*ii))
        knownRoots.insert(*ii);
  }

  void assertMapValid() {
    for (std::map<unsigned, DSCallGraph::FuncSet>::iterator ii = SCCs.begin(),
            ee = SCCs.end(); ii != ee; ++ii) {
      for (DSCallGraph::FuncSet::iterator i = ii->second.begin(),
              e = ii->second.end(); i != e; ++i) {
        assert(*i && "Null Function in map");
        assert(invmap.find(*i) != invmap.end() && "Function not in invmap");
        assert(invmap[*i] == ii->first && "invmap doesn't match map");
      }
    }

    for (std::map<const llvm::Function*, unsigned>::iterator ii = invmap.begin(),
            ee = invmap.end(); ii != ee; ++ii) {
      assert(ii->first && "Null Function in invmap");
      assert(SCCs.find(ii->second) != SCCs.end() && "Function in invmap but not in map");
      assert(SCCs[ii->second].count(ii->first) && "SCC doesn't contain function");
    }
  }

public:

  DSSCCGraph(DSCallGraph& DSG) :nextSCC(0) {
    oldGraph = DSG;

    buildSCC(DSG);
    assertMapValid();

    buildCallGraph(DSG);

    buildRoots();
    
  }

  void dump() {
    //function map
    for (std::map<unsigned, DSCallGraph::FuncSet>::iterator ii = SCCs.begin(),
            ee = SCCs.end(); ii != ee; ++ii) {
      llvm::errs() << "Functions in " << ii->first << ":";
      for (DSCallGraph::FuncSet::iterator i = ii->second.begin(),
              e = ii->second.end(); i != e; ++i)
        llvm::errs() << " " << *i << "(" << invmap[*i] << ")";
      llvm::errs() << "\n";
    }

//    for (std::map<const llvm::Function*, unsigned>::iterator ii = invmap.begin(),
//            ee = invmap.end(); ii != ee; ++ii)
//      llvm::errs() << ii->first << " -> " << ii->second << "\n";

    //SCC map
    for (std::map<unsigned, sv::set<unsigned> >::iterator ii = SCCCallees.begin(),
            ee = SCCCallees.end(); ii != ee; ++ii) {
      llvm::errs() << "CallGraph[" << ii->first << "]";
      for (sv::set<unsigned>::iterator i = ii->second.begin(),
              e = ii->second.end(); i != e; ++i)
        llvm::errs() << " " << *i;
      llvm::errs() << "\n";
    }

    //Functions we know about that aren't called
    llvm::errs() << "Roots:";
    for (sv::set<unsigned>::iterator ii = knownRoots.begin(),
            ee = knownRoots.end(); ii != ee; ++ii)
      llvm::errs() << " " << *ii;
    llvm::errs() << "\n";
  }

};

#endif	/* LLVM_DSCALLGRAPH_H */

