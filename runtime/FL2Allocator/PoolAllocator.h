//===- PoolAllocator.h - Pool allocator runtime interface file --*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines the interface which is implemented by the LLVM pool
// allocator runtime library.
//
//===----------------------------------------------------------------------===//

#ifndef POOLALLOCATOR_RUNTIME_H
#define POOLALLOCATOR_RUNTIME_H

struct PoolSlab;
struct FreedNodeHeader;

// NodeHeader - Each block of memory is preceeded in the the pool by one of
// these headers.  If the node is allocated, the ObjectSize value is used, if
// the object is free, the 'Next' value is used.
union NodeHeader {
  FreedNodeHeader *Next;
  unsigned ObjectSize;
};


// When objects are on the free list, we pretend they have this header.  
struct FreedNodeHeader {
  // NormalHeader - This is the normal node header that is on allocated or free
  // blocks.
  NodeHeader NormalHeader;

  // Size - This is stored in the actual data area, indicating how many bytes
  // there are in the region.
  unsigned Size;
};


// Large Arrays are passed on to directly malloc, and are not necessarily page
// aligned.  These arrays are marked by setting the object size preheader to ~0.
// LargeArrays are on their own list to allow for efficient deletion.
struct LargeArrayHeader {
  LargeArrayHeader **Prev, *Next;
  
  // Marker: this is the ObjectSize marker which MUST BE THE LAST ELEMENT of
  // this header!
  unsigned Marker;
};


struct PoolTy {
  // Lists - the list of slabs in this pool.
  PoolSlab *Slabs;

  // LargeArrays - A doubly linked list of large array chunks, dynamically
  // allocated with malloc.
  LargeArrayHeader *LargeArrays;

  // The list of free'd nodes.
  FreedNodeHeader *FreeNodeList;
};

extern "C" {
  void poolinit(PoolTy *Pool);
  void poolmakeunfreeable(PoolTy *Pool);
  void pooldestroy(PoolTy *Pool);
  void *poolalloc(PoolTy *Pool, unsigned NumBytes);
  void poolfree(PoolTy *Pool, void *Node);
}

#endif
