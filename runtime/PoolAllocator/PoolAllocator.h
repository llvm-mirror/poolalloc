//===- PoolAllocator.h - Pool allocator runtime interface file ------------===//
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

typedef struct PoolTy {
  // Data - An implementation specified data pointer.
  void *Data;

  // NodeSize - Keep track of the object size tracked by this pool
  unsigned NodeSize;

  // FreeablePool - Set to false if the memory from this pool cannot be freed
  // before destroy.
  //
  unsigned FreeablePool;
} PoolTy;

extern "C" {
  void poolinit(PoolTy *Pool, unsigned NodeSize);
  void poolmakeunfreeable(PoolTy *Pool);
  void pooldestroy(PoolTy *Pool);
  void *poolalloc(PoolTy *Pool);
  void poolfree(PoolTy *Pool, char *Node);
  void* poolallocarray(PoolTy* Pool, unsigned Size);
}

#endif
