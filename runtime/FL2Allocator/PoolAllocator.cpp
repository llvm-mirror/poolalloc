//===- FreeListAllocator.cpp - Simple linked-list based pool allocator ----===//
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
//===----------------------------------------------------------------------===//

#include "PoolAllocator.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Performance tweaking macros.
#define INITIAL_SLAB_SIZE 4096
#define LARGE_SLAB_SIZE   4096


// Configuration macros.  Define up to one of these.
//#define PRINT_NUM_POOLS          // Print use dynamic # pools info
//#define PRINT_POOLDESTROY_STATS  // When pools are destroyed, print stats
//#define PRINT_POOL_TRACE         // Print a full trace


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
  fprintf(stderr, "INVALID/UNKNOWN POOL DESCRIPTOR: 0x%X\n", (unsigned)PD);
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
  fprintf(stderr, "INVALID/UNKNOWN POOL DESCRIPTOR: 0x%X\n", (unsigned)PD);
  return 0;
}

#define DO_IF_TRACE(X) X
#else
#define DO_IF_TRACE(X)
#endif

#ifdef PRINT_POOLDESTROY_STATS
#define DO_IF_POOLDESTROY_STATS(X) X
#define PRINT_NUM_POOLS
#else
#define DO_IF_POOLDESTROY_STATS(X)
#endif

#ifdef PRINT_NUM_POOLS
static unsigned PoolCounter = 0;
static unsigned PoolsInited = 0;
static void PoolCountPrinter() {
  fprintf(stderr, "\n\n"
          "*** %d DYNAMIC POOLS INITIALIZED ***\n\n"
          "*** %d DYNAMIC POOLS ALLOCATED FROM ***\n\n",
          PoolsInited, PoolCounter);
}

#define DO_IF_PNP(X) X
#else
#define DO_IF_PNP(X)
#endif

//===----------------------------------------------------------------------===//
//  PoolSlab implementation
//===----------------------------------------------------------------------===//

static inline unsigned getSizeClass(unsigned NumBytes) {
  if (NumBytes <= FreeListOneSize)
    return NumBytes > FreeListZeroSize;
  else
    return 2 + (NumBytes > FreeListTwoSize);
}

static void AddNodeToFreeList(PoolTy *Pool, FreedNodeHeader *FreeNode) {
  unsigned SizeClass = getSizeClass(FreeNode->Header.Size);
  FreeNode->PrevP = &Pool->FreeNodeLists[SizeClass];
  FreeNode->Next = Pool->FreeNodeLists[SizeClass];
  Pool->FreeNodeLists[SizeClass] = FreeNode;
  if (FreeNode->Next)
    FreeNode->Next->PrevP = &FreeNode->Next;
}

static void UnlinkFreeNode(FreedNodeHeader *FNH) {
  *FNH->PrevP = FNH->Next;
  if (FNH->Next)
    FNH->Next->PrevP = FNH->PrevP;
}


// PoolSlab Structure - Hold multiple objects of the current node type.
// Invariants: FirstUnused <= UsedEnd
//
struct PoolSlab {
  // Next - This link is used when we need to traverse the list of slabs in a
  // pool, for example, to destroy them all.
  PoolSlab *Next;

public:
  static void create(PoolTy *Pool, unsigned SizeHint);
  void destroy();

  PoolSlab *getNext() const { return Next; }
};

// create - Create a new (empty) slab and add it to the end of the Pools list.
void PoolSlab::create(PoolTy *Pool, unsigned SizeHint) {
  unsigned Size = Pool->AllocSize;
  Pool->AllocSize <<= 1;
  Size = (Size+SizeHint-1) / SizeHint * SizeHint;
  PoolSlab *PS = (PoolSlab*)malloc(Size+sizeof(PoolSlab) + sizeof(NodeHeader) +
                                   sizeof(FreedNodeHeader));

  // Add the body of the slab to the free list...
  FreedNodeHeader *SlabBody = (FreedNodeHeader*)(PS+1);
  SlabBody->Header.Size = Size;
  AddNodeToFreeList(Pool, SlabBody);

  // Make sure to add a marker at the end of the slab to prevent the coallescer
  // from trying to merge off the end of the page.
  FreedNodeHeader *End =
    (FreedNodeHeader*)((char*)SlabBody + sizeof(NodeHeader) + Size);
  End->Header.Size = ~0; // Looks like an allocated chunk

  // Add the slab to the list...
  PS->Next = Pool->Slabs;
  Pool->Slabs = PS;
}

void PoolSlab::destroy() {
  free(this);
}


//===----------------------------------------------------------------------===//
//
//  Pool allocator library implementation
//
//===----------------------------------------------------------------------===//

// poolinit - Initialize a pool descriptor to empty
//
void poolinit(PoolTy *Pool, unsigned DeclaredSize) {
  assert(Pool && "Null pool pointer passed into poolinit!\n");
  memset(Pool, 0, sizeof(PoolTy));
  Pool->AllocSize = INITIAL_SLAB_SIZE;
  Pool->DeclaredSize = DeclaredSize;

  DO_IF_TRACE(fprintf(stderr, "[%d] poolinit(0x%X, %d)\n", addPoolNumber(Pool),
                      Pool, DeclaredSize));
  DO_IF_PNP(++PoolsInited);  // Track # pools initialized

#ifdef PRINT_NUM_POOLS
  static bool Initialized = 0;
  if (!Initialized) {
    Initialized = 1;
    atexit(PoolCountPrinter);
  }
#endif
}

// pooldestroy - Release all memory allocated for a pool
//
void pooldestroy(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");

  DO_IF_TRACE(fprintf(stderr, "[%d] ", removePoolNumber(Pool)));
  DO_IF_POOLDESTROY_STATS(fprintf(stderr,
                 "pooldestroy(0x%X) BytesAlloc=%d  NumObjs=%d"
                 " AvgObjSize=%d  NextAllocSize=%d\n",
                 Pool, Pool->BytesAllocated, Pool->NumObjects,
                 Pool->NumObjects ? Pool->BytesAllocated/Pool->NumObjects : 0,
                 Pool->AllocSize));


  // Free all allocated slabs.
  PoolSlab *PS = Pool->Slabs;
  while (PS) {
    PoolSlab *Next = PS->getNext();
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

void *poolalloc(PoolTy *Pool, unsigned NumBytes) {
  DO_IF_TRACE(fprintf(stderr, "[%d] poolalloc(%d) -> ",
                      getPoolNumber(Pool), NumBytes));

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system malloc.
  if (Pool == 0) return malloc(NumBytes);
  if (NumBytes == 0) NumBytes = 1;
  unsigned PtrSize = sizeof(int*);
  NumBytes = (NumBytes+(PtrSize-1)) & ~(PtrSize-1);  // Round up to 4/8 bytes...

  // Objects must be at least 8 bytes to hold the FreedNodeHeader object when
  // they are freed.
  if (NumBytes < (sizeof(FreedNodeHeader)-sizeof(NodeHeader)))
    NumBytes = sizeof(FreedNodeHeader)-sizeof(NodeHeader);

  DO_IF_PNP(if (Pool->NumObjects == 0) ++PoolCounter);  // Track # pools.

  ++Pool->NumObjects;
  Pool->BytesAllocated += NumBytes;

  // Figure out which size class to start scanning from.
  unsigned SizeClass = getSizeClass(NumBytes);

  // Scan for a class that has entries!
  while (SizeClass < 3 && Pool->FreeNodeLists[SizeClass] == 0)
    ++SizeClass;

  // Fast path.  In the common case, we can allocate a portion of the node at
  // the front of the free list.
  if (FreedNodeHeader *FirstNode = Pool->FreeNodeLists[SizeClass]) {
    unsigned FirstNodeSize = FirstNode->Header.Size;
    if (FirstNodeSize > NumBytes) {
      if (FirstNodeSize >= 2*NumBytes+sizeof(NodeHeader)) {
        // Put the remainder back on the list...
        FreedNodeHeader *NextNodes =
          (FreedNodeHeader*)((char*)FirstNode + sizeof(NodeHeader) + NumBytes);

        // Remove from list
        UnlinkFreeNode(FirstNode);

        NextNodes->Header.Size = FirstNodeSize-NumBytes-sizeof(NodeHeader);
        AddNodeToFreeList(Pool, NextNodes);

      } else {
        UnlinkFreeNode(FirstNode);
        NumBytes = FirstNodeSize;
      }
      FirstNode->Header.Size = NumBytes|1;   // Mark as allocated
      DO_IF_TRACE(fprintf(stderr, "0x%X\n", &FirstNode->Header+1));
      return &FirstNode->Header+1;
    }
  }

  // Perform a search of the free list, taking the front of the first free chunk
  // that is big enough.
  if (NumBytes <= LARGE_SLAB_SIZE-sizeof(PoolSlab)-sizeof(NodeHeader)) {
    do {
      FreedNodeHeader **FN = &Pool->FreeNodeLists[SizeClass];
      FreedNodeHeader *FNN = *FN;

      // Search the list for the first-fit
      while (FNN && FNN->Header.Size < NumBytes)
        FN = &FNN->Next, FNN = *FN;

      if (FNN) {
        // We found a slab big enough.  If it's a perfect fit, just unlink from
        // the free list, otherwise, slice a little bit off and adjust the free
        // list.
        if (FNN->Header.Size > 2*NumBytes+sizeof(NodeHeader)) {
          UnlinkFreeNode(FNN);

          // Put the remainder back on the list...
          FreedNodeHeader *NextNodes =
            (FreedNodeHeader*)((char*)FNN + sizeof(NodeHeader) + NumBytes);
          NextNodes->Header.Size = FNN->Header.Size-NumBytes-sizeof(NodeHeader);
          AddNodeToFreeList(Pool, NextNodes);
        } else {
          UnlinkFreeNode(FNN);
          NumBytes = FNN->Header.Size;
        }
        FNN->Header.Size = NumBytes|1;   // Mark as allocated
        DO_IF_TRACE(fprintf(stderr, "0x%X\n", &FNN->Header+1));
        return &FNN->Header+1;
      }

      if (SizeClass < LargeFreeList) {
        ++SizeClass;
      } else {
        // Oops, we didn't find anything on the free list big enough!  Allocate
        // another slab and try again.
        PoolSlab::create(Pool, NumBytes);
      }
    } while (1);
  }

  // Otherwise, the allocation is a large array.  Since we're not going to be
  // able to help much for this allocation, simply pass it on to malloc.
  LargeArrayHeader *LAH = (LargeArrayHeader*)malloc(sizeof(LargeArrayHeader) + 
                                                    NumBytes);
  LAH->Next = Pool->LargeArrays;
  if (LAH->Next)
    LAH->Next->Prev = &LAH->Next;
  Pool->LargeArrays = LAH;
  LAH->Prev = &Pool->LargeArrays;
  LAH->Size = NumBytes;
  LAH->Marker = ~0U;
  DO_IF_TRACE(fprintf(stderr, "0x%X  [large]\n", LAH+1));
  return LAH+1;
}

void poolfree(PoolTy *Pool, void *Node) {
  DO_IF_TRACE(fprintf(stderr, "[%d] poolfree(0x%X) ",
                      getPoolNumber(Pool), Node));

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system free.
  if (Pool == 0) { free(Node); return; }
  if (Node == 0) return;

  // Check to see how many elements were allocated to this node...
  FreedNodeHeader *FNH = (FreedNodeHeader*)((char*)Node-sizeof(NodeHeader));
  assert((FNH->Header.Size & 1) && "Node not allocated!");
  unsigned Size = FNH->Header.Size & ~1;
  DO_IF_TRACE(fprintf(stderr, "%d bytes\n", Size));

  if (Size == ~1U) goto LargeArrayCase;
  
  // If the node immediately after this one is also free, merge it into node.
  FreedNodeHeader *NextFNH;
  NextFNH = (FreedNodeHeader*)((char*)Node+Size);
  while ((NextFNH->Header.Size & 1) == 0) {
    // Unlink NextFNH from the freelist that it is in.
    UnlinkFreeNode(NextFNH);
    Size += sizeof(NodeHeader)+NextFNH->Header.Size;
    NextFNH = (FreedNodeHeader*)((char*)Node+Size);
  }

  // If there are already nodes on the freelist, see if these blocks can be
  // coallesced into one of the early blocks on the front of the list.  This is
  // a simple check that prevents many horrible forms of fragmentation.
  //
  for (unsigned SizeClass = 0; SizeClass <= LargeFreeList; ++SizeClass)
    if (FreedNodeHeader *CurFrontNode = Pool->FreeNodeLists[SizeClass])
      if ((char*)CurFrontNode + sizeof(NodeHeader) + CurFrontNode->Header.Size==
          (char*)FNH) {
        // This node immediately follows the node on the front of the free-list.
        // No list manipulation is required.
        CurFrontNode->Header.Size += Size+sizeof(NodeHeader);
        return;
      }

  FNH->Header.Size = Size;
  AddNodeToFreeList(Pool, FNH);
  return;

LargeArrayCase:
  LargeArrayHeader *LAH = ((LargeArrayHeader*)Node)-1;

  // Unlink it from the list of large arrays...
  *LAH->Prev = LAH->Next;
  if (LAH->Next)
    LAH->Next->Prev = LAH->Prev;
  free(LAH);
}

void *poolrealloc(PoolTy *Pool, void *Node, unsigned NumBytes) {
  DO_IF_TRACE(fprintf(stderr, "[%d] poolrealloc(0x%X, %d)\n",
                      getPoolNumber(Pool), Node, NumBytes));

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system realloc.
  if (Pool == 0) return realloc(Node, NumBytes);
  if (Node == 0) return poolalloc(Pool, NumBytes);
  if (NumBytes == 0) {
    poolfree(Pool, Node);
    return 0;
  }

  // FIXME: This is obviously much worse than it could be.  In particular, we
  // never try to expand something in a pool.  This might hurt some programs!
  void *New = poolalloc(Pool, NumBytes);
  assert(New != 0 && "Our poolalloc doesn't ever return null for failure!");

  // Copy the min of the new and old sizes over.
  unsigned Size = poolobjsize(Pool, Node);
  memcpy(New, Node, Size < NumBytes ? Size : NumBytes);
  poolfree(Pool, Node);
  return New;
}

unsigned poolobjsize(PoolTy *Pool, void *Node) {
  if (Node == 0) return 0;

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  We don't really have any way to service this!!
  if (Pool == 0) {
    fprintf(stderr, "ERROR: Cannot call poolobjsize on a pool that is getting"
            " memory from the heap.  Sorry!\n");
    abort();
  }

  // Check to see how many bytes were allocated to this node.
  FreedNodeHeader *FNH = (FreedNodeHeader*)((char*)Node-sizeof(NodeHeader));
  assert((FNH->Header.Size & 1) && "Node not allocated!");
  unsigned Size = FNH->Header.Size & ~1;
  if (Size != ~1U) return Size;

  // Otherwise, we have a large array.
  LargeArrayHeader *LAH = ((LargeArrayHeader*)Node)-1;
  return LAH->Size;
}
