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

#include "PoolSlab.h"

typedef struct PoolTy {
#if 0
  // The size of a page on this system
  unsigned int PageSize;
#endif

  // NodeSize - Keep track of the object size tracked by this pool
  unsigned NodeSize;

  // Maximum number of nodes per page
  unsigned int MaxNodesPerPage;

  // Pointer to the list of slabs allocated for this pool
  struct SlabHeader * Slabs;

  // Linked list of slabs used to hold arrays
  struct SlabHeader * ArraySlabs;

  // Pointer to the fast alloc array
  struct SlabHeader * FastArray;

  // Pointer to the free list of nodes
  struct NodePointer FreeList;

#if 0
  // FreeablePool - Set to false if the memory from this pool cannot be freed
  // before destroy.
  //
  unsigned FreeablePool;
#endif /* 0 */
} PoolTy;

extern "C" {
  void poolinit(PoolTy *Pool, unsigned NodeSize);
  void poolmakeunfreeable(PoolTy *Pool);
  void pooldestroy(PoolTy *Pool);
  void *poolalloc(PoolTy *Pool, unsigned NodeSize);
  void poolfree(PoolTy *Pool, void *Node);
  void* poolallocarray(PoolTy* Pool, unsigned Size);
}

#endif
