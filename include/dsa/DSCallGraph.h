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

#include "llvm/Support/CallSite.h"

namespace llvm {
class Function;
}

template<class Iter>
class KeyIterator {
  Iter I;
public:
  typedef typename Iter::difference_type difference_type;
  typedef typename Iter::value_type::first_type value_type;
  typedef typename Iter::value_type::first_type* pointer;
  typedef typename Iter::value_type::first_type& reference;
  typedef typename Iter::iterator_category iterator_category;

  KeyIterator(Iter i) :I(i) {}
  Iter base() const { return I; }
  reference    operator*() const { return I->first; }
  KeyIterator  operator+(difference_type n) const { return KeyIterator(I + n); }
  KeyIterator& operator++() { ++I; return *this; }
  KeyIterator  operator++(int) { Iter OI = I; ++I; return KeyIterator(OI); }
  KeyIterator& operator+= (difference_type n) { I += n; return *this; }
  KeyIterator  operator- (difference_type n) const { return KeyIterator(I - n); }
  KeyIterator& operator--() { --I; return *this; }
  KeyIterator  operator--(int) { Iter OI = I; --I; return KeyIterator(OI); }
  KeyIterator& operator-=(difference_type n) { I -= n; return *this; }
  pointer operator->() const { return &I->first; }
  bool operator==(const KeyIterator& RHS) const { return I == RHS.I; }
  bool operator!=(const KeyIterator& RHS) const { return I != RHS.I; }
};

class DSCallGraph {

  typedef sv::set<const llvm::Function*> FuncSet;
  typedef std::map<llvm::CallSite, FuncSet> ActualCalleesTy;

  ActualCalleesTy ActualCallees;

public:

  typedef ActualCalleesTy::mapped_type::const_iterator iterator;
  typedef KeyIterator<ActualCalleesTy::const_iterator> key_iterator;

  void insert(llvm::CallSite CS, const llvm::Function* F) {
    if (F) ActualCallees[CS].insert(F);
  }

  template<class Iterator>
  void insert(llvm::CallSite CS, Iterator _begin, Iterator _end) {
    for(; _begin != _end; ++_begin)
      insert(CS, *_begin);
  }

  iterator callee_begin(llvm::CallSite CS) const {
    ActualCalleesTy::const_iterator ii = ActualCallees.find(CS);
    if (ii == ActualCallees.end())
      ii = ActualCallees.find(llvm::CallSite());
    return ii->second.begin();
  }

  iterator callee_end(llvm::CallSite CS) const {
    ActualCalleesTy::const_iterator ii = ActualCallees.find(CS);
    if (ii == ActualCallees.end())
      ii = ActualCallees.find(llvm::CallSite());
    return ii->second.end();
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

  key_iterator key_begin() const {
    return key_iterator(ActualCallees.begin());
  }

  key_iterator key_end() const {
    return key_iterator(ActualCallees.end());
  }

};

#endif	/* LLVM_DSCALLGRAPH_H */

