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
  // PoolBase - The actual pool.  Note that this MUST stay as the first element
  // of this structure.
  char *PoolBase;

  // BitVector - Bitvector of bits.  This stores two bits per node.
  unsigned long *BitVector;

  // NodeSize - The size of the nodes to actually allocate out of the pool.
  // This size already considers object padding due to alignment.
  unsigned NodeSize;

  // NumNodesInBitVector - This indicates the amount of space we have for nodes
  // in the bitvector.  We actually have space for 2* this number of bits.
  unsigned long NumNodesInBitVector;

  // NumUsed - The number of nodes that are currently initialized out of
  // NumNodesInBitVector.  Invariant: NumUsed <= NumNodesInBitVector.
  unsigned long NumUsed;


  // These fields are only used in debug mode to build stats, keep at end.
  unsigned BytesAllocated, NumObjects;
};

extern "C" {
  void poolinit_pc(PoolTy *Pool, unsigned NodeSize, unsigned ObjAlignment);
  void pooldestroy_pc(PoolTy *Pool);
  unsigned long poolalloc_pc(PoolTy *Pool, unsigned NumBytes);
  void poolfree_pc(PoolTy *Pool, unsigned long Node);
  //void *poolmemalign_pc(PoolTy *Pool, unsigned Alignment, unsigned NumBytes);

  // FIXME: Add support for bump pointer pools.  These pools don't need the
  // bitvector.
}

#endif
