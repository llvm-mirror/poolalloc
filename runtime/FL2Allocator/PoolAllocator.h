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
// these headers.
struct NodeHeader {
  unsigned long Size;
};


// When objects are on the free list, we pretend they have this header.  
struct FreedNodeHeader {
  // NormalHeader - This is the normal node header that is on allocated or free
  // blocks.
  NodeHeader Header;

  // Next - The next object in the free list.
  FreedNodeHeader *Next;

  // PrevP - The pointer that points to this node on the free list.
  FreedNodeHeader **PrevP;
};


// Large Arrays are passed on to directly malloc, and are not necessarily page
// aligned.  These arrays are marked by setting the object size preheader to ~1.
// LargeArrays are on their own list to allow for efficient deletion.
struct LargeArrayHeader {
  LargeArrayHeader **Prev, *Next;

  // Size - This contains the size of the object.
  unsigned long Size;
  
  // Marker: this is the ObjectSize marker which MUST BE THE LAST ELEMENT of
  // this header!
  unsigned long Marker;

  void UnlinkFromList() {
    *Prev = Next;
    if (Next)
      Next->Prev = Prev;
  }

  void LinkIntoList(LargeArrayHeader **List) {
    Next = *List;
    if (Next)
      Next->Prev = &Next;
    *List = this;
    Prev = List;
  }
};


// FreeList*Size - These are size classes for each of the freelists in a pool.
// An object in a particular free list is guaranteed to be <= this size.
enum {
  FreeListZeroSize = 8,
  FreeListOneSize  = 12,
  FreeListTwoSize  = 16,

  // There are four free lists.
  LargeFreeList = 3
};

struct PoolTy {
  // Lists - the list of slabs in this pool.
  PoolSlab *Slabs;

  // The free node lists for objects of various sizes.  
  FreedNodeHeader *FreeNodeLists[4];

  // Alignment - The required alignment of allocations the pool in bytes.
  unsigned Alignment;

  // NumObjects - the number of poolallocs for this pool.
  unsigned NumObjects;

  // BytesAllocated - The total number of bytes ever allocated from this pool.
  // Together with NumObjects, allows us to calculate average object size.
  unsigned BytesAllocated;

  // LargeArrays - A doubly linked list of large array chunks, dynamically
  // allocated with malloc.
  LargeArrayHeader *LargeArrays;

  // The size to allocate for the next slab.
  unsigned AllocSize;

  // The declared size of the pool, just kept for the record.
  unsigned DeclaredSize;
};

extern "C" {
  void poolinit(PoolTy *Pool, unsigned DeclaredSize, unsigned ObjAlignment);
  void poolmakeunfreeable(PoolTy *Pool);
  void pooldestroy(PoolTy *Pool);
  void *poolalloc(PoolTy *Pool, unsigned NumBytes);
  void *poolrealloc(PoolTy *Pool, void *Node, unsigned NumBytes);
  void poolfree(PoolTy *Pool, void *Node);

  /// poolobjsize - Reutrn the size of the object at the specified address, in
  /// the specified pool.  Note that this cannot be used in normal cases, as it
  /// is completely broken if things land in the system heap.  Perhaps in the
  /// future.  :(
  ///
  unsigned poolobjsize(PoolTy *Pool, void *Node);
}

#endif
