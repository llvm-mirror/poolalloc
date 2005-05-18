/*===- HeapFrag.c - Routine to fragment the heap --------------------------===//
//
//                       The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines a function 'EnsureHeapFragmentation', which is used to
// artificially fragment the heap, to show the value of pool allocator even for
// silly benchmarks that never free memory and thus have no fragmentation at
// all.
//
//===----------------------------------------------------------------------===*/

#include <stdlib.h>

static void **AllocateNodes(unsigned N, unsigned Size) {
  void **Spine = (void**)malloc(N*sizeof(void*));
  unsigned i;
  for (i = 0; i != N; ++i)
    Spine[i] = malloc(Size);
  return Spine;
}

static void DeallocateNodes(void **Spine, unsigned N, unsigned Stride) {
  unsigned i;
  for (i = 0; i < N; i += Stride) {
    free(Spine[i]);
    Spine[i] = 0;
  }
}


void EnsureHeapFragmentation() {
  void **DS1 = AllocateNodes(10000, 16);
  void **DS2;
  void *A, *B, *C;
  DeallocateNodes(DS1+9000, 1000, 1);  /* Free last elements */
  DS2 = AllocateNodes(40000, 40);
  DeallocateNodes(DS1, 9000, 2);
  DeallocateNodes(DS2, 40000, 2);
  DS1 = AllocateNodes(2000, 8);
  DeallocateNodes(DS1, 2000, 2);
  DeallocateNodes(DS1, 2000, 3);
}
