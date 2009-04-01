//===-- poolalloc/ADT/HashExtras.h - STL hashing for LLVM -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains some templates that are useful if you are working with the
// STL Hashed containers.
//
// No library is required when using these functions.
//
// This header files is a modification of HashExtras.h from LLVM.  It is GNU
// C++ specific.
//
//===----------------------------------------------------------------------===//

#ifndef POOLALLOC_ADT_HASHEXTRAS_H
#define POOLALLOC_ADT_HASHEXTRAS_H

#include <functional>
#include <ext/hash_map>
#include <ext/hash_set>
#include <string>

// Cannot specialize hash template from outside of the std namespace.
namespace __gnu_cxx {

// Provide a hash function for arbitrary pointers...
template <class T> struct hash<T *> {
  inline size_t operator()(const T *Val) const {
    return reinterpret_cast<size_t>(Val);
  }
};

template <> struct hash<std::string> {
  size_t operator()(std::string const &str) const {
    return hash<char const *>()(str.c_str());
  }
};

}  // End namespace std

// Use the namespace so that we don't need to state it explictly.
using namespace __gnu_cxx;

#endif
