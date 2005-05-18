//===- Slab.h - Implementation of poolallocator runtime -------------------===//
// 
//                       The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This header file defines structures used internally by the free list pool
// allocator library.
//
//===----------------------------------------------------------------------===//

#ifndef _POOLSLAB_H
#define _POOLSLAB_H

#include "PoolAllocator.h"
#include "PageManager.h"
#include <assert.h>
#include <stdlib.h>

//===----------------------------------------------------------------------===//
//
//  Defintion of Slab Data Structures
//
//===----------------------------------------------------------------------===//

//
// Provide a pointer type that points to pointers of itself.
//
typedef struct NodePointer
{
  struct NodePointer * Next;
} NodePointer;

//
// Structure: SlabHeader
//
// Description:
//  This structure defines the beginning of a memory slab.  A memory slab
//  consists of book keeping information, a list of pointers, and a list of
//  data blocks.
//
//  There is a 1 to 1 correspondence between the pointers and the data blocks.
//  Pointer[x] points to Pointer[y] if Data[y] is linked after Data[x] in a
//  linked list.  In other words, Pointer[x] is the "next" pointer for Data[x].
//
//  The slab is allocated on a page boundary, so it is easy to find match
//  pointers to blocks if you know the offset of one of them.
//
struct SlabHeader
{
  // Flags whether this is an array
  unsigned char IsArray : 1;

  // Flags whether this is managed by the Page Manager
  unsigned char IsManaged : 1;

  // Number of nodes per slab
  unsigned int NodesPerSlab;

  // Reference Count
  unsigned int LiveNodes;

  // Next free data block
  unsigned int NextFreeData;

  // Pointer to the next slab
  struct SlabHeader * Next;

  // Pointer to the data area (will be in the same page)
  unsigned char * Data;

  // Pointer to the list of nodes
  NodePointer BlockList [];
};

#endif /* _POOLSLAB_H */
