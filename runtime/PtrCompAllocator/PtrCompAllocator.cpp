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
#include <string.h>
#include <sys/mman.h>

#define POOLSIZE (256*1024*1024)

#ifndef NDEBUG
// Configuration macros.  Define up to one of these.
//#define PRINT_NUM_POOLS          // Print use dynamic # pools info
//#define PRINT_POOLDESTROY_STATS  // When pools are destroyed, print stats
#define PRINT_POOL_TRACE         // Print a full trace
#endif


//===----------------------------------------------------------------------===//
// Pool Debugging stuff.
//===----------------------------------------------------------------------===//

#ifdef PRINT_POOL_TRACE
#define PRINT_POOLDESTROY_STATS

struct PoolID {
  PoolTy *PD;
  unsigned ID;
};

struct PoolID *PoolIDs = 0;
static unsigned NumLivePools = 0;
static unsigned NumPoolIDsAllocated = 0;
static unsigned CurPoolID = 0;

static unsigned addPoolNumber(PoolTy *PD) {
  if (NumLivePools == NumPoolIDsAllocated) {
    NumPoolIDsAllocated = (10+NumPoolIDsAllocated)*2;
    PoolIDs = (PoolID*)realloc(PoolIDs, sizeof(PoolID)*NumPoolIDsAllocated);
  }
  
  PoolIDs[NumLivePools].PD = PD;
  PoolIDs[NumLivePools].ID = ++CurPoolID;
  NumLivePools++;
  return CurPoolID;
}

static unsigned getPoolNumber(PoolTy *PD) {
  if (PD == 0) return 1234567;
  for (unsigned i = 0; i != NumLivePools; ++i)
    if (PoolIDs[i].PD == PD)
      return PoolIDs[i].ID;
  fprintf(stderr, "INVALID/UNKNOWN POOL DESCRIPTOR: 0x%lX\n", (unsigned long)PD);
  return 0;
}

static unsigned removePoolNumber(PoolTy *PD) {
  for (unsigned i = 0; i != NumLivePools; ++i)
    if (PoolIDs[i].PD == PD) {
      unsigned PN = PoolIDs[i].ID;
      memmove(&PoolIDs[i], &PoolIDs[i+1], sizeof(PoolID)*(NumLivePools-i-1));
      --NumLivePools;
      return PN;
    }
  fprintf(stderr, "INVALID/UNKNOWN POOL DESCRIPTOR: 0x%lX\n", (unsigned long)PD);
  return 0;
}

static void PrintPoolStats(PoolTy *Pool);
static void PrintLivePoolInfo() {
  for (unsigned i = 0; i != NumLivePools; ++i) {
    PoolTy *Pool = PoolIDs[i].PD;
    fprintf(stderr, "[%d] pool at exit ", PoolIDs[i].ID);
    PrintPoolStats(Pool);
  }
}

#define DO_IF_TRACE(X) X
#else
#define DO_IF_TRACE(X)
#endif

#ifdef PRINT_POOLDESTROY_STATS
#define DO_IF_POOLDESTROY_STATS(X) X
#define PRINT_NUM_POOLS

static void PrintPoolStats(PoolTy *Pool) {
  fprintf(stderr,
          "(0x%X) BytesAlloc=%d  NumObjs=%d"
          " bitvectorsize=%d  numused=%d  OrigSize=%d NewSize=%d\n",
          Pool, Pool->BytesAllocated, Pool->NumObjects,
          Pool->NumNodesInBitVector, Pool->NumUsed, Pool->OrigSize,
          Pool->NewSize);
}

#else
#define DO_IF_POOLDESTROY_STATS(X)
#endif

#ifdef PRINT_NUM_POOLS
static unsigned PoolCounter = 0;
static unsigned PoolsInited = 0;
static void PoolCountPrinter() {
  DO_IF_TRACE(PrintLivePoolInfo());
  fprintf(stderr, "\n\n"
          "*** %d DYNAMIC POOLS INITIALIZED ***\n\n"
          "*** %d DYNAMIC POOLS ALLOCATED FROM ***\n\n",
          PoolsInited, PoolCounter);
}

static void InitPrintNumPools() {
  static bool Initialized = 0;
  if (!Initialized) {
    Initialized = 1;
    atexit(PoolCountPrinter);
  }
}

#define DO_IF_PNP(X) X
#else
#define DO_IF_PNP(X)
#endif


//===----------------------------------------------------------------------===//
// Pointer Compression Runtime implementation
//===----------------------------------------------------------------------===//

// poolinit_pc - Initialize a pool descriptor to empty
//
void poolinit_pc(PoolTy *Pool, unsigned NewSize, unsigned OldSize,
                 unsigned ObjAlignment) {
  assert(Pool && OldSize && NewSize && ObjAlignment);
  assert((ObjAlignment & (ObjAlignment-1)) == 0 && "Alignment not 2^k!");

  DO_IF_PNP(memset(Pool, 0, sizeof(PoolTy)));
  Pool->OrigSize = OldSize;

  // Round up to the next alignment boundary.
  Pool->NewSize = (NewSize+NewSize-1) & ~(ObjAlignment-1);

  Pool->PoolBase = 0;
  Pool->BitVector = 0;

  DO_IF_TRACE(fprintf(stderr, "[%d] poolinit_pc(0x%X, %d)\n",
                      addPoolNumber(Pool), Pool, ObjAlignment));
  DO_IF_PNP(++PoolsInited);  // Track # pools initialized
  DO_IF_PNP(InitPrintNumPools());
}

// pooldestroy_pc - Release all memory allocated for a pool
//
void pooldestroy_pc(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");
  if (!Pool->PoolBase) return;  // No memory ever allocated.

  DO_IF_TRACE(fprintf(stderr, "[%d] pooldestroy_pc", removePoolNumber(Pool)));
  DO_IF_POOLDESTROY_STATS(PrintPoolStats(Pool));

  munmap(Pool->PoolBase, POOLSIZE);
  Pool->PoolBase = 0;
  free(Pool->BitVector);
}

static inline void MarkNodeAllocated(PoolTy *Pool, unsigned long NodeNum) {
  Pool->BitVector[NodeNum*2/(sizeof(long)*8)] |=
    3 << (2*(NodeNum & (sizeof(long)*8/2-1)));
}

static inline void MarkNodeFree(PoolTy *Pool, unsigned long NodeNum) {
  Pool->BitVector[NodeNum*2/(sizeof(long)*8)] &=
    ~(3 << (2*(NodeNum & (sizeof(long)*8/2-1))));
}

static void CreatePool(PoolTy *Pool) __attribute__((noinline));
static void CreatePool(PoolTy *Pool) {
  Pool->PoolBase = (char*)mmap(0, POOLSIZE, PROT_READ|PROT_WRITE,
                               MAP_PRIVATE|MAP_NORESERVE|MAP_ANONYMOUS, 0, 0);
  Pool->NumNodesInBitVector = 1024;
  Pool->BitVector = (unsigned long*)malloc(Pool->NumNodesInBitVector*2/8);

  // Mark the null pointer allocated.
  Pool->NumUsed = 1;
  MarkNodeAllocated(Pool, 0);
}

unsigned long poolalloc_pc(PoolTy *Pool, unsigned NumBytes) {
  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  DO_IF_PNP(if (Pool->NumObjects == 0) ++PoolCounter);  // Track # pools.

  unsigned OrigSize = Pool->OrigSize;
  unsigned NodesToAllocate;
  if (NumBytes == OrigSize)
    NodesToAllocate = 1;     // Common case.
  else
    NodesToAllocate = (NumBytes+OrigSize-1)/OrigSize;

  DO_IF_TRACE(fprintf(stderr, "[%d] poolalloc_pc(%d [%d node%s]) -> ",
                      getPoolNumber(Pool), NumBytes, NodesToAllocate,
                      NodesToAllocate == 1 ? "" : "s"));

  DO_IF_PNP(++Pool->NumObjects);
  DO_IF_PNP(Pool->BytesAllocated += NumBytes);

  assert(NodesToAllocate == 1 && "Array allocation not implemented yet!");

  if (Pool->PoolBase == 0)
    CreatePool(Pool);

AllocNode:
  // FIXME: This should attempt to reuse free'd nodes!

  // See if we can grow the pool without allocating new bitvector space.
  if (Pool->NumUsed < Pool->NumNodesInBitVector) {
    unsigned long Result = Pool->NumUsed++;
    MarkNodeAllocated(Pool, Result);
    DO_IF_TRACE(fprintf(stderr, "0x%X\n", (unsigned)Result));
    return Result;
  }

  // Otherwise, we need to grow the bitvector.  Double its size.
  Pool->NumNodesInBitVector <<= 1;
  Pool->BitVector = (unsigned long*)realloc(Pool->BitVector,
                                            Pool->NumNodesInBitVector*2/8);
  goto AllocNode;
}

void poolfree_pc(PoolTy *Pool, unsigned long Node) {
  if (Node == 0) return;
  assert(Pool && Node < Pool->NumUsed && "Node or pool incorrect!");
  DO_IF_TRACE(fprintf(stderr, "[%d] poolfree_pc(0x%X) ",
                      getPoolNumber(Pool), (unsigned)Node));

  // If freeing the last node, just pop it from the end.
  if (Node == Pool->NumUsed-1) {
    --Pool->NumUsed;
    DO_IF_TRACE(fprintf(stderr, "1 node\n"));
    return;
  }

  // Otherwise, mark the node free'd.
  MarkNodeFree(Pool, Node);
}
