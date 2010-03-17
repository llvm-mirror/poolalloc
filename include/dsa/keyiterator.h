//===- keyiterator.h - Iterator over just the keys in a map -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implement a map key iterator
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_KEYITERATOR_H
#define	LLVM_KEYITERATOR_H

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

#endif	/* LLVM_KEYITERATOR_H */

