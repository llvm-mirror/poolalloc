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
static void PoolCountPrinter() {
  DO_IF_TRACE(PrintLivePoolInfo());
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
  char *PoolBody = (char*)(PS+1);

  // If the Alignment is greater than the size of the FreedNodeHeader, skip over
  // some space so that the a "free pointer + sizeof(FreedNodeHeader)" is always
  // aligned.
  unsigned Alignment = Pool->Alignment;
  if (Alignment > sizeof(FreedNodeHeader)) {
    PoolBody += Alignment-sizeof(FreedNodeHeader);
    Size -= Alignment-sizeof(FreedNodeHeader);
  }
      
  // Add the body of the slab to the free list.
  FreedNodeHeader *SlabBody = (FreedNodeHeader*)PoolBody;
  SlabBody->Header.Size = Size;
  AddNodeToFreeList(Pool, SlabBody);

  // Make sure to add a marker at the end of the slab to prevent the coallescer
  // from trying to merge off the end of the page.
  FreedNodeHeader *End =
      (FreedNodeHeader*)(PoolBody + sizeof(NodeHeader) + Size);
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
  Pool->Alignment = __alignof(double);

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

  DO_IF_TRACE(fprintf(stderr, "[%d] pooldestroy", removePoolNumber(Pool)));
  DO_IF_POOLDESTROY_STATS(PrintPoolStats(Pool));

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
  if (Pool == 0) {
    void *Result = malloc(NumBytes);
    DO_IF_TRACE(fprintf(stderr, "0x%X [malloc]\n", Result));
                return Result;
  }
  DO_IF_PNP(if (Pool->NumObjects == 0) ++PoolCounter);  // Track # pools.

  // Objects must be at least 8 bytes to hold the FreedNodeHeader object when
  // they are freed.  This also handles allocations of 0 bytes.
  if (NumBytes < (sizeof(FreedNodeHeader)-sizeof(NodeHeader)))
    NumBytes = sizeof(FreedNodeHeader)-sizeof(NodeHeader);

  // Adjust the size so that memory allocated from the pool is always on the
  // proper alignment boundary.
  unsigned Alignment = Pool->Alignment;
  NumBytes = NumBytes+sizeof(FreedNodeHeader)+(Alignment-1);      // Round up
  NumBytes = (NumBytes & ~(Alignment-1))-sizeof(FreedNodeHeader); // Truncate

  ++Pool->NumObjects;
  Pool->BytesAllocated += NumBytes;

  if (NumBytes >= LARGE_SLAB_SIZE-sizeof(PoolSlab)-sizeof(NodeHeader))
    goto LargeObject;

  // Figure out which size class to start scanning from.
  unsigned SizeClass;
  SizeClass = getSizeClass(NumBytes);

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

void poolfree(PoolTy *Pool, void *Node) {
  if (Node == 0) return;
  DO_IF_TRACE(fprintf(stderr, "[%d] poolfree(0x%X) ",
                      getPoolNumber(Pool), Node));

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system free.
  if (Pool == 0) {
    free(Node);
    DO_IF_TRACE(fprintf(stderr, "[free]\n"));
    return;
  }

  // Check to see how many elements were allocated to this node...
  FreedNodeHeader *FNH = (FreedNodeHeader*)((char*)Node-sizeof(NodeHeader));
  assert((FNH->Header.Size & 1) && "Node not allocated!");
  unsigned Size = FNH->Header.Size & ~1;

  if (Size == ~1U) goto LargeArrayCase;
  DO_IF_TRACE(fprintf(stderr, "%d bytes\n", Size));
  
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
  // a simple check that prevents many horrible forms of fragmentation,
  // particularly when freeing objects in allocation order.
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
  DO_IF_TRACE(fprintf(stderr, "%d bytes [large]\n", LAH->Size));

  // Unlink it from the list of large arrays and free it.
  LAH->UnlinkFromList();
  free(LAH);
}

void *poolrealloc(PoolTy *Pool, void *Node, unsigned NumBytes) {
  DO_IF_TRACE(fprintf(stderr, "[%d] poolrealloc(0x%X, %d) -> ",
                      getPoolNumber(Pool), Node, NumBytes));

  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system realloc.
  if (Pool == 0) {
    void *Result = realloc(Node, NumBytes);
    DO_IF_TRACE(fprintf(stderr, "0x%X (system realloc)\n", Result));
    return Result;
  }
  if (Node == 0) return poolalloc(Pool, NumBytes);
  if (NumBytes == 0) {
    poolfree(Pool, Node);
    DO_IF_TRACE(fprintf(stderr, "freed\n"));
    return 0;
  }

  FreedNodeHeader *FNH = (FreedNodeHeader*)((char*)Node-sizeof(NodeHeader));
  assert((FNH->Header.Size & 1) && "Node not allocated!");
  unsigned Size = FNH->Header.Size & ~1;
  if (Size != ~1U) {
    // FIXME: This is obviously much worse than it could be.  In particular, we
    // never try to expand something in a pool.  This might hurt some programs!
    void *New = poolalloc(Pool, NumBytes);
    assert(New != 0 && "Our poolalloc doesn't ever return null for failure!");
    
    // Copy the min of the new and old sizes over.
    memcpy(New, Node, Size < NumBytes ? Size : NumBytes);
    poolfree(Pool, Node);
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
