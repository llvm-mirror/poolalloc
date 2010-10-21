//===- PoolAllocator.cpp - Simple free-list based pool allocator ----------===//
// 
//                       The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file is one possible implementation of the LLVM pool allocator runtime
// library.
//
// FIXME:
//  The pointer compression functions are not thread safe.
//===----------------------------------------------------------------------===//

#include "PoolAllocator.h"
#include "poolalloc/MMAPSupport.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef long intptr_t;
typedef unsigned long uintptr_t;

// Performance tweaking macros.
#define INITIAL_SLAB_SIZE 4096
#define LARGE_SLAB_SIZE   4096

#ifndef NDEBUG
#define NDEBUG
#endif

#ifndef NDEBUG
// Configuration macros.  Define up to one of these.
#define PRINT_NUM_POOLS          // Print use dynamic # pools info
//#define PRINT_POOLDESTROY_STATS  // When pools are destroyed, print stats
//#define PRINT_POOL_TRACE         // Print a full trace
#define ENABLE_POOL_IDS            // PID for access/pool traces


// ALWAYS_USE_MALLOC_FREE - Make poolalloc/free always call malloc/free.  Note
// that if the poolfree optimization is in use that this will cause memory
// leaks!
//#define ALWAYS_USE_MALLOC_FREE
#endif

//===----------------------------------------------------------------------===//
// Pool Debugging stuff.
//===----------------------------------------------------------------------===//

#if defined(ALWAYS_USE_MALLOC_FREE)
#define DO_IF_FORCE_MALLOCFREE(x) x
#else
#define DO_IF_FORCE_MALLOCFREE(x)
#endif


#if !defined(PRINT_POOL_TRACE)
#define DO_IF_TRACE(X)
#else
#define ENABLE_POOL_IDS
#define DO_IF_TRACE(X) X
#define PRINT_POOLDESTROY_STATS
#endif

#if defined(ENABLE_POOL_IDS)
struct PoolID {
  void *PD;
  unsigned ID;
};

struct PoolID *PoolIDs = 0;
static unsigned NumLivePools = 0;
static unsigned NumPoolIDsAllocated = 0;
static unsigned CurPoolID = 0;

static unsigned addPoolNumber(void *PD) {
  if (NumLivePools == NumPoolIDsAllocated) {
    NumPoolIDsAllocated = (10+NumPoolIDsAllocated)*2;
    PoolIDs = (PoolID*)realloc(PoolIDs, sizeof(PoolID)*NumPoolIDsAllocated);
  }
  
  PoolIDs[NumLivePools].PD = PD;
  PoolIDs[NumLivePools].ID = ++CurPoolID;
  NumLivePools++;
  return CurPoolID;
}

static unsigned getPoolNumber(void *PD) {
  if (PD == 0) return ~0;
  for (unsigned i = 0; i != NumLivePools; ++i)
    if (PoolIDs[i].PD == PD)
      return PoolIDs[i].ID;
  fprintf(stderr, "INVALID/UNKNOWN POOL DESCRIPTOR: 0x%lX\n",(unsigned long)PD);
  return 0;
}

static unsigned removePoolNumber(void *PD) {
  for (unsigned i = 0; i != NumLivePools; ++i)
    if (PoolIDs[i].PD == PD) {
      unsigned PN = PoolIDs[i].ID;
      memmove(&PoolIDs[i], &PoolIDs[i+1], sizeof(PoolID)*(NumLivePools-i-1));
      --NumLivePools;
      return PN;
    }
  fprintf(stderr, "INVALID/UNKNOWN POOL DESCRIPTOR: 0x%lX\n",(unsigned long)PD);
  return 0;
}

static void PrintPoolStats(void *Pool);
template<typename PoolTraits>
static void PrintLivePoolInfo() {
  for (unsigned i = 0; i != NumLivePools; ++i) {
    fprintf(stderr, "[%d] pool at exit ", PoolIDs[i].ID);
    PrintPoolStats((PoolTy<PoolTraits>*)PoolIDs[i].PD);
  }
}
#endif

#ifdef PRINT_POOLDESTROY_STATS
#define DO_IF_POOLDESTROY_STATS(X) X
#define PRINT_NUM_POOLS

template<typename PoolTraits>
static void PrintPoolStats(PoolTy<PoolTraits> *Pool) {
  fprintf(stderr,
          "(0x%X) BytesAlloc=%d  NumObjs=%d"
          " AvgObjSize=%d  NextAllocSize=%d  DeclaredSize=%d\n",
          Pool, Pool->BytesAllocated, Pool->NumObjects,
          Pool->NumObjects ? Pool->BytesAllocated/Pool->NumObjects : 0,
          Pool->AllocSize, Pool->DeclaredSize);
}

#else
#define DO_IF_POOLDESTROY_STATS(X)
#endif

#ifdef PRINT_NUM_POOLS
static unsigned PoolCounter = 0;
static unsigned PoolsInited = 0;

// MaxHeapSize - The maximum size of the heap ever.
static unsigned MaxHeapSize = 0;

// CurHeapSize - The current size of the heap.
static unsigned CurHeapSize = 0;

template<typename PoolTraits>
static void PoolCountPrinter() {
  DO_IF_TRACE(PrintLivePoolInfo<PoolTraits>());
  fprintf(stderr, "\n\n"
          "*** %d DYNAMIC POOLS INITIALIZED ***\n\n"
          "*** %d DYNAMIC POOLS ALLOCATED FROM ***\n\n",
          PoolsInited, PoolCounter);
  fprintf(stderr, "MaxHeapSize = %fKB  HeapSizeAtExit = %fKB   "
          "NOTE: only valid if using Heuristic=AllPools and no "
          "bumpptr/realloc!\n", MaxHeapSize/1024.0, CurHeapSize/1024.0);
}

template<typename PoolTraits>
static void InitPrintNumPools() {
  static bool Initialized = 0;
  if (!Initialized) {
    Initialized = 1;
    atexit(PoolCountPrinter<PoolTraits>);
  }
}

#define DO_IF_PNP(X) X
#else
#define DO_IF_PNP(X)
#endif

//===----------------------------------------------------------------------===//
//  PoolSlab implementation
//===----------------------------------------------------------------------===//


template<typename PoolTraits>
static void AddNodeToFreeList(PoolTy<PoolTraits> *Pool,
                              FreedNodeHeader<PoolTraits> *FreeNode) {
  typename PoolTraits::FreeNodeHeaderPtrTy *FreeList;
  if (FreeNode->Header.Size == Pool->DeclaredSize)
    FreeList = &Pool->ObjFreeList;
  else
    FreeList = &Pool->OtherFreeList;

  void *PoolBase = Pool->Slabs;

  typename PoolTraits::FreeNodeHeaderPtrTy FreeNodeIdx = 
    PoolTraits::FNHPtrToIndex(FreeNode, PoolBase);

  FreeNode->Prev = 0;   // First on the list.
  FreeNode->Next = *FreeList;
  *FreeList = FreeNodeIdx;
  if (FreeNode->Next)
    PoolTraits::IndexToFNHPtr(FreeNode->Next, PoolBase)->Prev = FreeNodeIdx;
}

template<typename PoolTraits>
static void UnlinkFreeNode(PoolTy<PoolTraits> *Pool,
                           FreedNodeHeader<PoolTraits> *FNH) {
  void *PoolBase = Pool->Slabs;

  // Make the predecessor point to our next node.
  if (FNH->Prev)
    PoolTraits::IndexToFNHPtr(FNH->Prev, PoolBase)->Next = FNH->Next;
  else {
    typename PoolTraits::FreeNodeHeaderPtrTy NodeIdx = 
      PoolTraits::FNHPtrToIndex(FNH, PoolBase);

    if (Pool->ObjFreeList == NodeIdx)
      Pool->ObjFreeList = FNH->Next;
    else {
      assert(Pool->OtherFreeList == NodeIdx &&
             "Prev Ptr is null but not at head of free list?");
      Pool->OtherFreeList = FNH->Next;
    }
  }

  if (FNH->Next)
    PoolTraits::IndexToFNHPtr(FNH->Next, PoolBase)->Prev = FNH->Prev;
}


// PoolSlab Structure - Hold multiple objects of the current node type.
// Invariants: FirstUnused <= UsedEnd
//
template<typename PoolTraits>
struct PoolSlab {
  // Next - This link is used when we need to traverse the list of slabs in a
  // pool, for example, to destroy them all.
  PoolSlab<PoolTraits> *Next;

public:
  static void create(PoolTy<PoolTraits> *Pool, unsigned SizeHint);
  static void *create_for_bp(PoolTy<PoolTraits> *Pool);
  static void create_for_ptrcomp(PoolTy<PoolTraits> *Pool,
                                 void *Mem, unsigned Size);
  void destroy();

  PoolSlab<PoolTraits> *getNext() const { return Next; }
};

// create - Create a new (empty) slab and add it to the end of the Pools list.
template<typename PoolTraits>
void PoolSlab<PoolTraits>::create(PoolTy<PoolTraits> *Pool, unsigned SizeHint) {
  if (Pool->DeclaredSize == 0) {
    unsigned Align = Pool->Alignment;
    if (SizeHint < sizeof(FreedNodeHeader<PoolTraits>) - 
                   sizeof(NodeHeader<PoolTraits>))
      SizeHint = sizeof(FreedNodeHeader<PoolTraits>) -
                 sizeof(NodeHeader<PoolTraits>);
    SizeHint = SizeHint+sizeof(FreedNodeHeader<PoolTraits>)+(Align-1);
    SizeHint = (SizeHint & ~(Align-1))-sizeof(FreedNodeHeader<PoolTraits>);
    Pool->DeclaredSize = SizeHint;
  }

  unsigned Size = Pool->AllocSize;
  Pool->AllocSize <<= 1;
  Size = (Size+SizeHint-1) / SizeHint * SizeHint;
  PoolSlab *PS = (PoolSlab*)malloc(Size+sizeof(PoolSlab<PoolTraits>) +
                                   sizeof(NodeHeader<PoolTraits>) +
                                   sizeof(FreedNodeHeader<PoolTraits>));
  char *PoolBody = (char*)(PS+1);

  // If the Alignment is greater than the size of the FreedNodeHeader, skip over
  // some space so that the a "free pointer + sizeof(FreedNodeHeader)" is always
  // aligned.
  unsigned Alignment = Pool->Alignment;
  if (Alignment > sizeof(FreedNodeHeader<PoolTraits>)) {
    PoolBody += Alignment-sizeof(FreedNodeHeader<PoolTraits>);
    Size -= Alignment-sizeof(FreedNodeHeader<PoolTraits>);
  }

  // Add the body of the slab to the free list.
  FreedNodeHeader<PoolTraits> *SlabBody =(FreedNodeHeader<PoolTraits>*)PoolBody;
  SlabBody->Header.Size = Size;
  AddNodeToFreeList(Pool, SlabBody);

  // Make sure to add a marker at the end of the slab to prevent the coallescer
  // from trying to merge off the end of the page.
  FreedNodeHeader<PoolTraits> *End =
      (FreedNodeHeader<PoolTraits>*)(PoolBody + sizeof(NodeHeader<PoolTraits>)+
                                     Size);
  End->Header.Size = ~0; // Looks like an allocated chunk

  // Add the slab to the list...
  PS->Next = Pool->Slabs;
  Pool->Slabs = PS;
}

/// create_for_bp - This creates a slab for a bump-pointer pool.
template<typename PoolTraits>
void *PoolSlab<PoolTraits>::create_for_bp(PoolTy<PoolTraits> *Pool) {
  unsigned Size = Pool->AllocSize;
  Pool->AllocSize <<= 1;
  PoolSlab *PS = (PoolSlab*)malloc(Size+sizeof(PoolSlab));
  char *PoolBody = (char*)(PS+1);
  if (sizeof(PoolSlab) == 4)
    PoolBody += 4;            // No reason to start out unaligned.

  // Update the end pointer.
  Pool->OtherFreeList = (FreedNodeHeader<PoolTraits>*)((char*)(PS+1)+Size);

  // Add the slab to the list...
  PS->Next = Pool->Slabs;
  Pool->Slabs = PS;
  return PoolBody;
}

/// create_for_ptrcomp - Initialize a chunk of memory 'Mem' of size 'Size' for
/// pointer compression.
template<typename PoolTraits>
void PoolSlab<PoolTraits>::create_for_ptrcomp(PoolTy<PoolTraits> *Pool, 
                                              void *SMem, unsigned Size) {
  if (Pool->DeclaredSize == 0) {
    unsigned Align = Pool->Alignment;
    unsigned SizeHint = sizeof(FreedNodeHeader<PoolTraits>) -
                        sizeof(NodeHeader<PoolTraits>);
    SizeHint = SizeHint+sizeof(FreedNodeHeader<PoolTraits>)+(Align-1);
    SizeHint = (SizeHint & ~(Align-1))-sizeof(FreedNodeHeader<PoolTraits>);
    Pool->DeclaredSize = SizeHint;
  }

  Size -= sizeof(PoolSlab) + sizeof(NodeHeader<PoolTraits>) +
          sizeof(FreedNodeHeader<PoolTraits>);
  PoolSlab *PS = (PoolSlab*)SMem;
  char *PoolBody = (char*)(PS+1);

  // If the Alignment is greater than the size of the NodeHeader, skip over some
  // space so that the a "free pointer + sizeof(NodeHeader)" is always aligned
  // for user data.
  unsigned Alignment = Pool->Alignment;
  if (Alignment > sizeof(NodeHeader<PoolTraits>)) {
    PoolBody += Alignment-sizeof(NodeHeader<PoolTraits>);
    Size -= Alignment-sizeof(NodeHeader<PoolTraits>);
  }

  // Add the body of the slab to the free list.
  FreedNodeHeader<PoolTraits> *SlabBody =(FreedNodeHeader<PoolTraits>*)PoolBody;
  SlabBody->Header.Size = Size;
  AddNodeToFreeList(Pool, SlabBody);

  // Make sure to add a marker at the end of the slab to prevent the coallescer
  // from trying to merge off the end of the page.
  FreedNodeHeader<PoolTraits> *End =
    (FreedNodeHeader<PoolTraits>*)(PoolBody + sizeof(NodeHeader<PoolTraits>) +
                                   Size);
  End->Header.Size = ~0; // Looks like an allocated chunk
  PS->Next = 0;
}


template<typename PoolTraits>
void PoolSlab<PoolTraits>::destroy() {
  free(this);
}

//===----------------------------------------------------------------------===//
//
//  Bump-pointer pool allocator library implementation
//
//===----------------------------------------------------------------------===//

void poolinit_bp(PoolTy<NormalPoolTraits> *Pool, unsigned ObjAlignment) {
  DO_IF_PNP(memset(Pool, 0, sizeof(PoolTy<NormalPoolTraits>)));
  pthread_mutex_init(&Pool->pool_lock,NULL);
  Pool->Slabs = 0;
  if (ObjAlignment < 4) ObjAlignment = __alignof(double);
  Pool->AllocSize = INITIAL_SLAB_SIZE;
  Pool->Alignment = ObjAlignment;
  Pool->LargeArrays = 0;
  Pool->ObjFreeList = 0;     // This is our bump pointer.
  Pool->OtherFreeList = 0;   // This is our end pointer.

#ifdef ENABLE_POOL_IDS
  unsigned PID;
  PID = addPoolNumber(Pool);

  DO_IF_TRACE(fprintf(stderr, "[%d] poolinit_bp(0x%X, %d)\n",
                      PID, Pool, ObjAlignment));
#endif
  DO_IF_PNP(++PoolsInited);  // Track # pools initialized
  DO_IF_PNP(InitPrintNumPools<NormalPoolTraits>());
}

void *poolalloc_bp(PoolTy<NormalPoolTraits> *Pool, unsigned NumBytes) {
  DO_IF_FORCE_MALLOCFREE(return malloc(NumBytes));
  assert(Pool && "Bump pointer pool does not support null PD!");
  DO_IF_TRACE(fprintf(stderr, "[%d] poolalloc_bp(%d) -> ",
                      getPoolNumber(Pool), NumBytes));
  DO_IF_PNP(if (Pool->NumObjects == 0) ++PoolCounter);  // Track # pools.

  pthread_mutex_lock(&Pool->pool_lock);

  if (NumBytes >= LARGE_SLAB_SIZE)
    goto LargeObject;

  DO_IF_PNP(++Pool->NumObjects);
  DO_IF_PNP(Pool->BytesAllocated += NumBytes);

  if (NumBytes < 1) NumBytes = 1;

  uintptr_t Alignment;
  char *BumpPtr, *EndPtr;
  Alignment = Pool->Alignment-1;
  BumpPtr = (char*)Pool->ObjFreeList; // Get our bump pointer.
  EndPtr  = (char*)Pool->OtherFreeList; // Get our end pointer.

TryAgain:
  // Align the bump pointer to the required boundary.
  BumpPtr = (char*)(intptr_t((BumpPtr+Alignment)) & ~Alignment);

  if (BumpPtr + NumBytes < EndPtr) {
    void *Result = BumpPtr;
    // Update bump ptr.
    Pool->ObjFreeList = (FreedNodeHeader<NormalPoolTraits>*)(BumpPtr+NumBytes);
    DO_IF_TRACE(fprintf(stderr, "%p\n", Result));
    pthread_mutex_unlock(&Pool->pool_lock);
    return Result;
  }
  
  BumpPtr = (char*)PoolSlab<NormalPoolTraits>::create_for_bp(Pool);
  EndPtr  = (char*)Pool->OtherFreeList; // Get our updated end pointer.  
  goto TryAgain;

LargeObject:
  // Otherwise, the allocation is a large array.  Since we're not going to be
  // able to help much for this allocation, simply pass it on to malloc.
  LargeArrayHeader *LAH = (LargeArrayHeader*)malloc(sizeof(LargeArrayHeader) + 
                                                    NumBytes);
  LAH->Size = NumBytes;
  LAH->Marker = ~0U;
  LAH->LinkIntoList(&Pool->LargeArrays);
  DO_IF_TRACE(fprintf(stderr, "%p  [large]\n", LAH+1));
  pthread_mutex_unlock(&Pool->pool_lock);
  return LAH+1;
}

void pooldestroy_bp(PoolTy<NormalPoolTraits> *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");

#ifdef ENABLE_POOL_IDS
  unsigned PID;
  PID = removePoolNumber(Pool);
  DO_IF_TRACE(fprintf(stderr, "[%d] pooldestroy_bp", PID));
#endif
  DO_IF_POOLDESTROY_STATS(PrintPoolStats(Pool));

  pthread_mutex_destroy(&Pool->pool_lock);

  // Free all allocated slabs.
  PoolSlab<NormalPoolTraits> *PS = Pool->Slabs;
  while (PS) {
    PoolSlab<NormalPoolTraits> *Next = PS->getNext();
    PS->destroy();
    PS = Next;
  }

  // Free all of the large arrays.
  LargeArrayHeader *LAH = Pool->LargeArrays;
  while (LAH) {
    LargeArrayHeader *Next = LAH->Next;
    free(LAH);
    LAH = Next;
  }
}



//===----------------------------------------------------------------------===//
//
//  Pool allocator library implementation
//
//===----------------------------------------------------------------------===//

// poolinit - Initialize a pool descriptor to empty
//
template<typename PoolTraits>
static void poolinit_internal(PoolTy<PoolTraits> *Pool,
                              unsigned DeclaredSize, unsigned ObjAlignment) {
  assert(Pool && "Null pool pointer passed into poolinit!\n");
  memset(Pool, 0, sizeof(PoolTy<PoolTraits>));
  Pool->thread_refcount = 1;
  pthread_mutex_init(&Pool->pool_lock,NULL);
  Pool->AllocSize = INITIAL_SLAB_SIZE;

  if (ObjAlignment < 4) ObjAlignment = __alignof(double);
  Pool->Alignment = ObjAlignment;

  // Round the declared size up to an alignment boundary-header size, just like
  // we have to do for objects.
  if (DeclaredSize) {
    if (DeclaredSize < sizeof(FreedNodeHeader<PoolTraits>) -
                       sizeof(NodeHeader<PoolTraits>))
      DeclaredSize = sizeof(FreedNodeHeader<PoolTraits>) -
                     sizeof(NodeHeader<PoolTraits>);
    DeclaredSize = DeclaredSize+sizeof(FreedNodeHeader<PoolTraits>) + 
                   (ObjAlignment-1);
    DeclaredSize = (DeclaredSize & ~(ObjAlignment-1)) -
                   sizeof(FreedNodeHeader<PoolTraits>);
  }

  Pool->DeclaredSize = DeclaredSize;

#ifdef ENABLE_POOL_IDS
  unsigned PID;
  PID = addPoolNumber(Pool);
  DO_IF_TRACE(fprintf(stderr, "[%d] poolinit%s(0x%X, %d, %d)\n",
                      PID, PoolTraits::getSuffix(),
                      Pool, DeclaredSize, ObjAlignment));
#endif
  DO_IF_PNP(++PoolsInited);  // Track # pools initialized
  DO_IF_PNP(InitPrintNumPools<PoolTraits>());
}

void poolinit(PoolTy<NormalPoolTraits> *Pool,
              unsigned DeclaredSize, unsigned ObjAlignment) {
  poolinit_internal(Pool, DeclaredSize, ObjAlignment);
}

// pooldestroy - Release all memory allocated for a pool
//
void pooldestroy(PoolTy<NormalPoolTraits> *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");

#ifdef USE_DYNCALL
  __sync_fetch_and_add(&Pool->thread_refcount,-1);
#else
  Pool->thread_refcount--;
#endif
  if(Pool->thread_refcount)
	  return;

  pthread_mutex_destroy(&Pool->pool_lock);

#ifdef ENABLE_POOL_IDS
  unsigned PID;
  PID = removePoolNumber(Pool);
  DO_IF_TRACE(fprintf(stderr, "[%d] pooldestroy", PID));
#endif
  DO_IF_POOLDESTROY_STATS(PrintPoolStats(Pool));

  // Free all allocated slabs.
  PoolSlab<NormalPoolTraits> *PS = Pool->Slabs;
  while (PS) {
    PoolSlab<NormalPoolTraits> *Next = PS->getNext();
    PS->destroy();
    PS = Next;
  }

  // Free all of the large arrays.
  LargeArrayHeader *LAH = Pool->LargeArrays;
  while (LAH) {
    LargeArrayHeader *Next = LAH->Next;
    free(LAH);
    LAH = Next;
  }
}

template<typename PoolTraits>
static void *poolalloc_internal(PoolTy<PoolTraits> *Pool, unsigned NumBytesA) {
  DO_IF_TRACE(fprintf(stderr, "[%d] poolalloc%s(%d) -> ",
                      getPoolNumber(Pool), PoolTraits::getSuffix(), NumBytesA));

  unsigned NumBytes = NumBytesA;

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system malloc.
  if (Pool == 0) {
    void *Result = malloc(NumBytes);
    DO_IF_TRACE(fprintf(stderr, "0x%X [malloc]\n", Result));
                return Result;
  }
  DO_IF_PNP(if (Pool->NumObjects == 0) ++PoolCounter);  // Track # pools.

  // Objects must be at least 8 bytes to hold the FreedNodeHeader object when
  // they are freed.  This also handles allocations of 0 bytes.
  if (NumBytes < (sizeof(FreedNodeHeader<PoolTraits>) - 
                  sizeof(NodeHeader<PoolTraits>)))
    NumBytes = sizeof(FreedNodeHeader<PoolTraits>) - 
               sizeof(NodeHeader<PoolTraits>);

  // Adjust the size so that memory allocated from the pool is always on the
  // proper alignment boundary.
  unsigned Alignment = Pool->Alignment;
  NumBytes = NumBytes+sizeof(FreedNodeHeader<PoolTraits>) + 
             (Alignment-1);      // Round up
  NumBytes = (NumBytes & ~(Alignment-1)) - 
             sizeof(FreedNodeHeader<PoolTraits>); // Truncate

  DO_IF_PNP(CurHeapSize += (NumBytes + sizeof(NodeHeader<PoolTraits>)));
  DO_IF_PNP(if (CurHeapSize > MaxHeapSize) MaxHeapSize = CurHeapSize);

  DO_IF_PNP(++Pool->NumObjects);
  DO_IF_PNP(Pool->BytesAllocated += NumBytes);

  // Fast path - allocate objects off the object list.
  if (NumBytes == Pool->DeclaredSize && Pool->ObjFreeList != 0) {
    typename PoolTraits::FreeNodeHeaderPtrTy NodeIdx = Pool->ObjFreeList;
    void *PoolBase = Pool->Slabs;
    FreedNodeHeader<PoolTraits> *Node =
      PoolTraits::IndexToFNHPtr(NodeIdx, PoolBase);
    UnlinkFreeNode(Pool, Node);
    assert(NumBytes == Node->Header.Size);

    Node->Header.Size = NumBytes|1;   // Mark as allocated
    DO_IF_TRACE(fprintf(stderr, "0x%X\n", &Node->Header+1));
    return &Node->Header+1;
  }

  if (PoolTraits::UseLargeArrayObjects &&
      NumBytes >= LARGE_SLAB_SIZE-sizeof(PoolSlab<PoolTraits>) - 
      sizeof(NodeHeader<PoolTraits>))
    goto LargeObject;

  // Fast path.  In the common case, we can allocate a portion of the node at
  // the front of the free list.
  do {
    void *PoolBase = Pool->Slabs;
    FreedNodeHeader<PoolTraits> *FirstNode =
      PoolTraits::IndexToFNHPtr(Pool->OtherFreeList, PoolBase);
    if (FirstNode) {
      unsigned FirstNodeSize = FirstNode->Header.Size;
      if (FirstNodeSize >= NumBytes) {
        if (FirstNodeSize >= 2*NumBytes+sizeof(NodeHeader<PoolTraits>)) {
          // Put the remainder back on the list...
          FreedNodeHeader<PoolTraits> *NextNodes =
            (FreedNodeHeader<PoolTraits>*)((char*)FirstNode +
                                   sizeof(NodeHeader<PoolTraits>) +NumBytes);
          
          // Remove from list
          UnlinkFreeNode(Pool, FirstNode);
          
          NextNodes->Header.Size = FirstNodeSize-NumBytes -
                                   sizeof(NodeHeader<PoolTraits>);
          AddNodeToFreeList(Pool, NextNodes);
          
        } else {
          UnlinkFreeNode(Pool, FirstNode);
          NumBytes = FirstNodeSize;
        }
        FirstNode->Header.Size = NumBytes|1;   // Mark as allocated
        DO_IF_TRACE(fprintf(stderr, "0x%X\n", &FirstNode->Header+1));
        return &FirstNode->Header+1;
      }

      // Perform a search of the free list, taking the front of the first free
      // chunk that is big enough.
      typename PoolTraits::FreeNodeHeaderPtrTy *FN = &Pool->OtherFreeList;
      FreedNodeHeader<PoolTraits> *FNN = FirstNode;
      
      // Search the list for the first-fit.
      while (FNN && FNN->Header.Size < NumBytes) {
        // Advance FN to point to the Next field of FNN.
        FN = &FNN->Next;

        // Advance FNN to point to whatever the next node points to (null or the
        // next node in the free list).
        FNN = PoolTraits::IndexToFNHPtr(*FN, PoolBase);
      }
      
      if (FNN) {
        // We found a slab big enough.  If it's a perfect fit, just unlink
        // from the free list, otherwise, slice a little bit off and adjust
        // the free list.
        if (FNN->Header.Size > 2*NumBytes+sizeof(NodeHeader<PoolTraits>)) {
          UnlinkFreeNode(Pool, FNN);
          
          // Put the remainder back on the list...
          FreedNodeHeader<PoolTraits> *NextNodes =
            (FreedNodeHeader<PoolTraits>*)((char*)FNN +
                                           sizeof(NodeHeader<PoolTraits>) +
                                           NumBytes);
          NextNodes->Header.Size = FNN->Header.Size-NumBytes -
            sizeof(NodeHeader<PoolTraits>);
          AddNodeToFreeList(Pool, NextNodes);
        } else {
          UnlinkFreeNode(Pool, FNN);
          NumBytes = FNN->Header.Size;
        }
        FNN->Header.Size = NumBytes|1;   // Mark as allocated
        DO_IF_TRACE(fprintf(stderr, "0x%X\n", &FNN->Header+1));
        return &FNN->Header+1;
      }
    }

    // If we are not allowed to grow this pool, don't.
    if (!PoolTraits::CanGrowPool) {
      DO_IF_TRACE(fprintf(stderr, "Pool Overflow, not growable\n"));
      abort();
      return 0;
    }

    // Oops, we didn't find anything on the free list big enough!  Allocate
    // another slab and try again.
    PoolSlab<PoolTraits>::create(Pool, NumBytes);
  } while (1);

LargeObject:
  // Otherwise, the allocation is a large array.  Since we're not going to be
  // able to help much for this allocation, simply pass it on to malloc.
  LargeArrayHeader *LAH = (LargeArrayHeader*)malloc(sizeof(LargeArrayHeader) + 
                                                    NumBytes);
  LAH->Size = NumBytes;
  LAH->Marker = ~0U;
  LAH->LinkIntoList(&Pool->LargeArrays);
  DO_IF_TRACE(fprintf(stderr, "0x%X  [large]\n", LAH+1));
  return LAH+1;
}

template<typename PoolTraits>
static void poolfree_internal(PoolTy<PoolTraits> *Pool, void *Node) {
  if (Node == 0) return;
  DO_IF_TRACE(fprintf(stderr, "[%d] poolfree%s(%p) ",
                      getPoolNumber(Pool), PoolTraits::getSuffix(), Node));

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system free.
  if (Pool == 0) {
    free(Node);
    DO_IF_TRACE(fprintf(stderr, "[free]\n"));
    return;
  }

  // Check to see how many elements were allocated to this node...
  FreedNodeHeader<PoolTraits> *FNH =
    (FreedNodeHeader<PoolTraits>*)((char*)Node-sizeof(NodeHeader<PoolTraits>));
  assert((FNH->Header.Size & 1) && "Node not allocated!");
  unsigned Size = FNH->Header.Size & ~1;

  if (Size == ~1U) goto LargeArrayCase;
  DO_IF_TRACE(fprintf(stderr, "%d bytes\n", Size));

  DO_IF_PNP(CurHeapSize -= (Size + sizeof(NodeHeader<PoolTraits>)));
  
  // If the node immediately after this one is also free, merge it into node.
  FreedNodeHeader<PoolTraits> *NextFNH;
  NextFNH = (FreedNodeHeader<PoolTraits>*)((char*)Node+Size);
  while ((NextFNH->Header.Size & 1) == 0) {
    // Unlink NextFNH from the freelist that it is in.
    UnlinkFreeNode(Pool, NextFNH);
    Size += sizeof(NodeHeader<PoolTraits>)+NextFNH->Header.Size;
    NextFNH = (FreedNodeHeader<PoolTraits>*)((char*)Node+Size);
  }

  // If there are already nodes on the freelist, see if these blocks can be
  // coallesced into one of the early blocks on the front of the list.  This is
  // a simple check that prevents many horrible forms of fragmentation,
  // particularly when freeing objects in allocation order.
  //
  if (Pool->ObjFreeList) {
    void *PoolBase = Pool->Slabs;
    FreedNodeHeader<PoolTraits> *ObjFNH = 
      PoolTraits::IndexToFNHPtr(Pool->ObjFreeList, PoolBase);

    if ((char*)ObjFNH + sizeof(NodeHeader<PoolTraits>) +
        ObjFNH->Header.Size == (char*)FNH) {
      // Merge this with a node that is already on the object size free list.
      // Because the object is growing, we will never be able to find it if we
      // leave it on the object freelist.
      UnlinkFreeNode(Pool, ObjFNH);
      ObjFNH->Header.Size += Size+sizeof(NodeHeader<PoolTraits>);
      AddNodeToFreeList(Pool, ObjFNH);
      return;
    }
  }

  if (Pool->OtherFreeList) {
    void *PoolBase = Pool->Slabs;
    FreedNodeHeader<PoolTraits> *OFNH = 
      PoolTraits::IndexToFNHPtr(Pool->OtherFreeList, PoolBase);

    if ((char*)OFNH + sizeof(NodeHeader<PoolTraits>) +
        OFNH->Header.Size == (char*)FNH) {
      // Merge this with a node that is already on the object size free list.
      OFNH->Header.Size += Size+sizeof(NodeHeader<PoolTraits>);
      return;
    }
  }

  FNH->Header.Size = Size;
  AddNodeToFreeList(Pool, FNH);
  return;

LargeArrayCase:
  LargeArrayHeader *LAH = ((LargeArrayHeader*)Node)-1;
  DO_IF_TRACE(fprintf(stderr, "%d bytes [large]\n", LAH->Size));
  DO_IF_PNP(CurHeapSize -= LAH->Size);

  // Unlink it from the list of large arrays and free it.
  LAH->UnlinkFromList();
  free(LAH);
}

template<typename PoolTraits>
static void *poolrealloc_internal(PoolTy<PoolTraits> *Pool, void *Node,
                                  unsigned NumBytes) {
  DO_IF_TRACE(fprintf(stderr, "[%d] poolrealloc%s(0x%X, %d) -> ",
                      getPoolNumber(Pool), PoolTraits::getSuffix(),
                      Node, NumBytes));

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system realloc.
  if (Pool == 0) {
    void *Result = realloc(Node, NumBytes);
    DO_IF_TRACE(fprintf(stderr, "0x%X (system realloc)\n", Result));
    return Result;
  }
  if (Node == 0) return poolalloc_internal(Pool, NumBytes);
  if (NumBytes == 0) {
    poolfree_internal(Pool, Node);
    DO_IF_TRACE(fprintf(stderr, "freed\n"));
    return 0;
  }

  FreedNodeHeader<PoolTraits> *FNH =
    (FreedNodeHeader<PoolTraits>*)((char*)Node-sizeof(NodeHeader<PoolTraits>));
  assert((FNH->Header.Size & 1) && "Node not allocated!");
  unsigned Size = FNH->Header.Size & ~1;
  if (Size != ~1U) {
    // FIXME: This is obviously much worse than it could be.  In particular, we
    // never try to expand something in a pool.  This might hurt some programs!
    void *New = poolalloc_internal(Pool, NumBytes);
    assert(New != 0 && "Our poolalloc doesn't ever return null for failure!");
    
    // Copy the min of the new and old sizes over.
    memcpy(New, Node, Size < NumBytes ? Size : NumBytes);
    poolfree_internal(Pool, Node);
    DO_IF_TRACE(fprintf(stderr, "0x%X (moved)\n", New));
    return New;
  }

  // Otherwise, we have a large array.  Perform the realloc using the system
  // realloc function.  This case is actually quite common as many large blocks
  // end up being realloc'd it seems.
  LargeArrayHeader *LAH = ((LargeArrayHeader*)Node)-1;
  LAH->UnlinkFromList();

  LargeArrayHeader *NewLAH =
    (LargeArrayHeader*)realloc(LAH, sizeof(LargeArrayHeader)+NumBytes);
  
  DO_IF_TRACE(if (LAH == NewLAH)
                fprintf(stderr, "resized in place (system realloc)\n");
              else
                fprintf(stderr, "0x%X (moved by system realloc)\n", NewLAH+1));
  NewLAH->LinkIntoList(&Pool->LargeArrays);
  return NewLAH+1;
}

unsigned poolobjsize(PoolTy<NormalPoolTraits> *Pool, void *Node) {
  if (Node == 0) return 0;

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  We don't really have any way to service this!!
  if (Pool == 0) {
    fprintf(stderr, "ERROR: Cannot call poolobjsize on a pool that is getting"
            " memory from the heap.  Sorry!\n");
    abort();
  }

  // Check to see how many bytes were allocated to this node.
  FreedNodeHeader<NormalPoolTraits> *FNH =
    (FreedNodeHeader<NormalPoolTraits>*)((char*)Node -
                                         sizeof(NodeHeader<NormalPoolTraits>));
  assert((FNH->Header.Size & 1) && "Node not allocated!");
  unsigned Size = FNH->Header.Size & ~1;
  if (Size != ~1U) return Size;

  // Otherwise, we have a large array.
  LargeArrayHeader *LAH = ((LargeArrayHeader*)Node)-1;
  return LAH->Size;
}


void *poolalloc(PoolTy<NormalPoolTraits> *Pool, unsigned NumBytes) {
  DO_IF_FORCE_MALLOCFREE(return malloc(NumBytes));
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  void* to_return = poolalloc_internal(Pool, NumBytes);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
  return to_return;
}

void *poolcalloc(PoolTy<NormalPoolTraits> *Pool,
                 unsigned NumBytes,
                 unsigned NumElements) {
  void * p = poolalloc (Pool, NumBytes * NumElements);
  if (p) {
    memset (p, 0, NumBytes * NumElements);
  }
  return p;
}

void *poolmemalign(PoolTy<NormalPoolTraits> *Pool,
                   unsigned Alignment, unsigned NumBytes) {
  //punt and use pool alloc.
  //I don't know if this is safe or breaks any assumptions in the runtime
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  intptr_t base = (intptr_t)poolalloc_internal(Pool, NumBytes + Alignment - 1);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
  return (void*)((base + (Alignment - 1)) & ~((intptr_t)Alignment -1));
}

void poolfree(PoolTy<NormalPoolTraits> *Pool, void *Node) {
  DO_IF_FORCE_MALLOCFREE(free(Node); return);
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  poolfree_internal(Pool, Node);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
}

void *poolrealloc(PoolTy<NormalPoolTraits> *Pool, void *Node,
                  unsigned NumBytes) {
  DO_IF_FORCE_MALLOCFREE(return realloc(Node, NumBytes));
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  void* to_return = poolrealloc_internal(Pool, Node, NumBytes);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
  return to_return;
}

#ifdef USE_DYNCALL
#include <dyncall.h>
#include <pthread.h>
#include <stdarg.h>

void* poolalloc_thread_start(void* arg_)
{
	void** arg = (void**)arg_;
	DCCallVM* callVM = dcNewCallVM((size_t)arg[1]*sizeof(size_t)+108);
	arg[2+(size_t)arg[1]+2] = callVM;
	int i;
	for(i=0; i<(size_t)arg[1]; i++)
		dcArgPointer(callVM,arg[2+i]);
	dcArgPointer(callVM,arg[2+i]);
	void* to_return = dcCallPointer(callVM,arg[0]);
	return to_return;
}

void* poolalloc_thread_manager(void* args_)
{
	void** args = (void**)args_;
	size_t num_pools = (size_t)args[1];

        void* to_return;
	pthread_join((pthread_t)(args[2+num_pools+1]),&to_return);

	for(int i=0; i<num_pools; i++)
		if(args[2+i])
			pooldestroy((PoolTy<NormalPoolTraits>*)args[2+i]);

	dcFree((DCCallVM*)args[2+num_pools+2]);
	free(args);
        return to_return;
}

int poolalloc_pthread_create(pthread_t* thread,
							 const pthread_attr_t* attr,
							 void *(*start_routine)(void*), int num_pools, ...)
{
	void** arg_array = (void**)malloc(sizeof(void*)*(5+num_pools));
	arg_array[0] = (void*)start_routine;
	arg_array[1] = (void*)num_pools;
	va_list argpools;
	va_start(argpools,num_pools);
	int i;
	for(i=0; i<num_pools; i++)
	{
		arg_array[2+i]=va_arg(argpools,void*);
		PoolTy<NormalPoolTraits>* pool_ptr = reinterpret_cast<PoolTy<NormalPoolTraits>*>(arg_array[2+i]);
		if(pool_ptr)
			__sync_fetch_and_add(&pool_ptr->thread_refcount,1);
	}
	arg_array[2+i]=va_arg(argpools,void*);
	va_end(argpools);

	pthread_t dummy;
	int to_return = pthread_create(&dummy,attr,poolalloc_thread_start,arg_array);
	arg_array[2+i+1] = (void*)(dummy);
	to_return |= pthread_create(thread,NULL,poolalloc_thread_manager,arg_array);

	return to_return;
}

#endif

//===----------------------------------------------------------------------===//
// Pointer Compression runtime library.  Most of these are just wrappers
// around the normal pool routines.
//===----------------------------------------------------------------------===//

// For now, use address space reservation of 256MB.
#define POOLSIZE (256*1024*1024)

// Pools - When we are done with a pool, don't munmap it, keep it around for
// next time.
static PoolSlab<CompressedPoolTraits> *Pools[4] = { 0, 0, 0, 0 };

void *poolinit_pc(PoolTy<CompressedPoolTraits> *Pool,
                  unsigned DeclaredSize, unsigned ObjAlignment) {
  poolinit_internal(Pool, DeclaredSize, ObjAlignment);

  // The number of nodes to stagger in the mmap'ed pool
  static unsigned stagger=0;

  // Create the pool.  We have to do this eagerly (instead of on the first
  // allocation), because code may want to eagerly copy the pool base into a
  // register.

  // If we already have a pool mapped, reuse it.
  for (unsigned i = 0; i != 4; ++i)
    if (Pools[i]) {
      Pool->Slabs = Pools[i];
      Pools[i] = 0;
      break;
    }

  //
  // Wrap the stagger value back to zero if we're past the size of the pool.
  // This way, we always reserve less than 2*POOLSIZE of the virtual address
  // space.
  //
  if ((stagger * DeclaredSize) >= POOLSIZE)
    stagger = 0;

  if (Pool->Slabs == 0) {
    //
    // Didn't find an existing pool, create one.
    //
    // To create a pool, we stagger the beginning of the pool so that pools
    // do not end up starting on the same page boundary (creating extra cache
    // conflicts).
    //
    Pool->Slabs = (PoolSlab<CompressedPoolTraits>*)
                      AllocateSpaceWithMMAP(POOLSIZE + (DeclaredSize * stagger), true);
    Pool->Slabs += (DeclaredSize * stagger);

    // Increase the stagger amount by one node.
    stagger++;
    DO_IF_TRACE(fprintf(stderr, "RESERVED ADDR SPACE: %p -> %p\n",
                        Pool->Slabs, (char*)Pool->Slabs+POOLSIZE));
  }
  PoolSlab<CompressedPoolTraits>::create_for_ptrcomp(Pool, Pool->Slabs,
                                                     POOLSIZE);
  return Pool->Slabs;
}

void pooldestroy_pc(PoolTy<CompressedPoolTraits> *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");
  pthread_mutex_destroy(&Pool->pool_lock);
  if (Pool->Slabs == 0)
    return;   // no memory allocated from this pool.

#ifdef ENABLE_POOL_IDS
  unsigned PID;
  PID = removePoolNumber(Pool);
  DO_IF_TRACE(fprintf(stderr, "[%d] pooldestroy_pc", PID));
#endif
  DO_IF_POOLDESTROY_STATS(PrintPoolStats(Pool));

  // If there is space to remember this pool, do so.
  for (unsigned i = 0; i != 4; ++i)
    if (Pools[i] == 0) {
      Pools[i] = Pool->Slabs;
      return;
    }

  // Otherwise, just munmap it.
  DO_IF_TRACE(fprintf(stderr, "UNMAPPING ADDR SPACE: %p -> %p\n",
                      Pool->Slabs, (char*)Pool->Slabs+POOLSIZE));
  munmap(Pool->Slabs, POOLSIZE);
}

unsigned long long poolalloc_pc(PoolTy<CompressedPoolTraits> *Pool,
                                unsigned NumBytes) {
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  void *Result = poolalloc_internal(Pool, NumBytes);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
  return (char*)Result-(char*)Pool->Slabs;
}

void poolfree_pc(PoolTy<CompressedPoolTraits> *Pool, unsigned long long Node) {
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  poolfree_internal(Pool, (char*)Pool->Slabs+Node);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
}

unsigned long long poolrealloc_pc(PoolTy<CompressedPoolTraits> *Pool,
                                  unsigned long long Node, unsigned NumBytes) {
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  void *Result = poolrealloc_internal(Pool, (char*)Pool->Slabs+Node, NumBytes);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
  return (char*)Result-(char*)Pool->Slabs;
}

// Alternate Pointer Compression
void *poolinit_pca(PoolTy<CompressedPoolTraits> *Pool, unsigned NodeSize,
		   unsigned ObjAlignment)
{
  return poolinit_pc(Pool, NodeSize, ObjAlignment);
}

void pooldestroy_pca(PoolTy<CompressedPoolTraits> *Pool)
{
  pooldestroy_pc(Pool);
}

void* poolalloc_pca(PoolTy<CompressedPoolTraits> *Pool, unsigned NumBytes)
{
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  void* to_return = poolalloc_internal(Pool, NumBytes);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
  return to_return;
}

void poolfree_pca(PoolTy<CompressedPoolTraits> *Pool, void* Node)
{
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  poolfree_internal(Pool, Node);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
}

void* poolrealloc_pca(PoolTy<CompressedPoolTraits> *Pool, void* Node, 
		      unsigned NumBytes)
{
  if (Pool) pthread_mutex_lock(&Pool->pool_lock);
  void* to_return = poolrealloc_internal(Pool, Node, NumBytes);
  if (Pool) pthread_mutex_unlock(&Pool->pool_lock);
  return to_return;
}

//===----------------------------------------------------------------------===//
// Access Tracing Runtime Library Support
//===----------------------------------------------------------------------===//

static FILE *FD = 0;
void poolaccesstraceinit() {
#ifdef ALWAYS_USE_MALLOC_FREE
  FD = fopen("trace.malloc.csv", "w");
#else
  FD = fopen("trace.pa.csv", "w");
#endif
}

#define NUMLRU 2
static void *LRUWindow[NUMLRU];

void poolaccesstrace(void *Ptr, void *PD) {
  static unsigned Time = ~0U;

  // Not pool memory?
  if (PD == 0) return;
  
  // Filter out stuff that is not to the heap.
  ++Time;
  if ((uintptr_t)Ptr > 1000000000UL)
    return;

  Ptr = (void*)((intptr_t)Ptr & ~31L);

#if 1
  // Drop duplicate points.
  for (unsigned i = 0; i != NUMLRU; ++i)
    if (Ptr == LRUWindow[i]) {
      memmove(LRUWindow+1, LRUWindow, sizeof(void*)*i);
      LRUWindow[0] = Ptr;
      return;
    }

  // Rotate LRU window.
  memmove(LRUWindow+1, LRUWindow, sizeof(void*)*(NUMLRU-1));
  LRUWindow[0] = Ptr;
#endif

  // Delete many points to reduce data.
  static unsigned int Ctr;
  if ((++Ctr & 31)) return;


  fprintf(FD, "%d", Time);
#if defined(ENABLE_POOL_IDS)
  for (unsigned PID = getPoolNumber(PD)+1; PID; --PID)
    fprintf(FD,"\t?");
#else
  fprintf(FD, "\t%p ", PD);
#endif
  fprintf(FD, "\t%lu\n", (intptr_t)Ptr);
}
