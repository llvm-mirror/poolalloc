//===- PtrCompAllocator.cpp - Implementation of ptr compression runtime ---===//
// 
//                       The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements the pointer compression runtime with a simple
// node-based, bitmapped allocator.
//
//===----------------------------------------------------------------------===//

#include "PtrCompAllocator.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

#define POOLSIZE (256*1024*1024)

// poolinit_pc - Initialize a pool descriptor to empty
//
void poolinit_pc(PoolTy *Pool, unsigned NewSize, unsigned OldSize,
                 unsigned ObjAlignment) {
  assert(Pool && OldSize && NewSize && ObjAlignment);
  assert((ObjAlignment & (ObjAlignment-1)) == 0 && "Alignment not 2^k!");
  Pool->OrigSize = OldSize;

  // Round up to the next alignment boundary.
  Pool->NewSize = (NewSize+NewSize-1) & ~(ObjAlignment-1);

  Pool->PoolBase = 0;
  Pool->BitVector = 0;
}

// pooldestroy_pc - Release all memory allocated for a pool
//
void pooldestroy_pc(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");
  if (!Pool->PoolBase) return;  // No memory ever allocated.

  munmap(Pool->PoolBase, POOLSIZE);
  Pool->PoolBase = 0;
  free(Pool->BitVector);
}

static void CreatePool(PoolTy *Pool) __attribute__((noinline));
static void CreatePool(PoolTy *Pool) {
  Pool->PoolBase = (char*)mmap(0, POOLSIZE, PROT_READ|PROT_WRITE,
                               MAP_PRIVATE|MAP_NORESERVE|MAP_ANONYMOUS, 0, 0);
  Pool->NumNodesInBitVector = 1024;
  Pool->BitVector = (unsigned long*)malloc(Pool->NumNodesInBitVector*2/8);
  Pool->NumUsed = 0;
}

static inline void MarkNodeAllocated(PoolTy *Pool, unsigned long NodeNum) {
  Pool->BitVector[NodeNum*2/(sizeof(long)*8)] |=
    3 << (2*(NodeNum & (sizeof(long)*8/2-1)));
}

static inline void MarkNodeFree(PoolTy *Pool, unsigned long NodeNum) {
  Pool->BitVector[NodeNum*2/(sizeof(long)*8)] &=
    ~(3 << (2*(NodeNum & (sizeof(long)*8/2-1))));
}


unsigned long poolalloc_pc(PoolTy *Pool, unsigned NumBytes) {
  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  unsigned OrigSize = Pool->OrigSize;
  unsigned NodesToAllocate;
  if (NumBytes == OrigSize)
    NodesToAllocate = 1;     // Common case.
  else
    NodesToAllocate = (NumBytes+OrigSize-1)/OrigSize;

  assert(NodesToAllocate == 1 && "Array allocation not implemented yet!");

  if (Pool->PoolBase == 0)
    CreatePool(Pool);

AllocNode:
  // FIXME: This should attempt to reuse free'd nodes!

  // See if we can grow the pool without allocating new bitvector space.
  if (Pool->NumUsed < Pool->NumNodesInBitVector) {
    unsigned long Result = Pool->NumUsed++;
    MarkNodeAllocated(Pool, Result);
    return Result;
  }

  // Otherwise, we need to grow the bitvector.  Double its size.
  Pool->NumNodesInBitVector <<= 1;
  Pool->BitVector = (unsigned long*)realloc(Pool->BitVector,
                                            Pool->NumNodesInBitVector*2/8);
  goto AllocNode;
}

void poolfree_pc(PoolTy *Pool, unsigned long Node) {
  assert(Pool && Node < Pool->NumUsed && "Node or pool incorrect!");

  // If freeing the last node, just pop it from the end.
  if (Node == Pool->NumUsed-1) {
    --Pool->NumUsed;
    return;
  }

  // Otherwise, mark the node free'd.
  MarkNodeFree(Pool, Node);
}
