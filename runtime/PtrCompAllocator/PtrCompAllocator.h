//===- PtrCompAllocator.h - Runtime Lib for pointer compression -*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines the interface which is implemented by the pointer
// compression runtime library.
//
//===----------------------------------------------------------------------===//

#ifndef PTRCOMP_ALLOCATOR_H
#define PTRCOMP_ALLOCATOR_H

struct PoolTy {
  char *PoolBase;   // The actual pool.
  unsigned long *BitVector;// Bitvector of bits.  This stores two bits per node.

  // OrigSize - The size of the nodes in a non-compressed pool.
  unsigned OrigSize;

  // NewSize - The size of the nodes to actually allocate out of the pool.  This
  // size already considers object padding due to alignment.
  unsigned NewSize;

  // NumNodesInBitVector - This indicates the amount of space we have for nodes
  // in the bitvector.  We actually have space for 2* this number of bits.
  unsigned long NumNodesInBitVector;

  // NumUsed - The number of nodes that are currently initialized out of
  // NumNodesInBitVector.  Invariant: NumUsed <= NumNodesInBitVector.
  unsigned long NumUsed;


  // These fields are only used in debug mode.
  unsigned BytesAllocated, NumObjects;
};

extern "C" {
  void poolinit_pc(PoolTy *Pool, unsigned NewSize, unsigned OldSize,
                   unsigned ObjAlignment);
  void pooldestroy_pc(PoolTy *Pool);
  unsigned long poolalloc_pc(PoolTy *Pool, unsigned NumBytes);
  void poolfree_pc(PoolTy *Pool, unsigned long Node);
  //void *poolmemalign_pc(PoolTy *Pool, unsigned Alignment, unsigned NumBytes);

  // FIXME: Add support for bump pointer pools.  These pools don't need the
  // bitvector.
}

#endif
