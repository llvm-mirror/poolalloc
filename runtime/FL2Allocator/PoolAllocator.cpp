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
//
//===----------------------------------------------------------------------===//

#include "PoolAllocator.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//===----------------------------------------------------------------------===//
//
//  PoolSlab implementation
//
//===----------------------------------------------------------------------===//

#define PageSize (4*1024U)

//#define DEBUG(X) X

#ifndef DEBUG
#define DEBUG(X)
#endif

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
  Size = Size / SizeHint * SizeHint;
  PoolSlab *PS = (PoolSlab*)malloc(Size);

  // Add the body of the slab to the free list...
  FreedNodeHeader *SlabBody = (FreedNodeHeader*)(PS+1);
  SlabBody->Size = Size-sizeof(PoolSlab)-sizeof(FreedNodeHeader);
  SlabBody->NormalHeader.Next = Pool->FreeNodeLists[LargeFreeList];
  Pool->FreeNodeLists[LargeFreeList] = SlabBody;

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
  Pool->AllocSize = PageSize;
  Pool->DeclaredSize = DeclaredSize;
  DEBUG(printf("init pool 0x%X\n", Pool));
}

// pooldestroy - Release all memory allocated for a pool
//
void pooldestroy(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");

  DEBUG(printf("destroy pool 0x%X  NextAllocSize = %d AvgObjSize = %d\n", Pool,
               Pool->AllocSize,
               Pool->NumObjects ? Pool->BytesAllocated/Pool->NumObjects : 0));

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

static inline unsigned getSizeClass(unsigned NumBytes) {
  if (NumBytes <= FreeListOneSize)
    return NumBytes > FreeListZeroSize;
  else
    return 2 + (NumBytes > FreeListTwoSize);
}

static void AddNodeToFreeList(PoolTy *Pool, FreedNodeHeader *FreeNode) {
  unsigned SizeClass = getSizeClass(FreeNode->Size);
  FreeNode->NormalHeader.Next = Pool->FreeNodeLists[SizeClass];
  Pool->FreeNodeLists[SizeClass] = FreeNode;
}


void *poolalloc(PoolTy *Pool, unsigned NumBytes) {
  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system malloc.
  if (Pool == 0) return malloc(NumBytes);
  if (NumBytes == 0) return 0;
  NumBytes = (NumBytes+3) & ~3;  // Round up to 4 bytes...

  ++Pool->NumObjects;
  Pool->BytesAllocated += NumBytes;

  // Figure out which size class to start scanning from.
  unsigned SizeClass = getSizeClass(NumBytes);

  // Scan for a class that has entries!
  while (SizeClass < 3 && Pool->FreeNodeLists[SizeClass] == 0)
    ++SizeClass;

  FreedNodeHeader *SizeClassFreeNodeList = Pool->FreeNodeLists[SizeClass];

  // Fast path.  In the common case, we can allocate a portion of the node at
  // the front of the free list.
  if (FreedNodeHeader *FirstNode = SizeClassFreeNodeList) {
    unsigned FirstNodeSize = FirstNode->Size;
    if (FirstNodeSize > NumBytes) {
      if (FirstNodeSize >= 2*NumBytes+sizeof(NodeHeader)) {
        // Put the remainder back on the list...
        FreedNodeHeader *NextNodes =
          (FreedNodeHeader*)((char*)FirstNode + sizeof(NodeHeader) + NumBytes);

        // Remove from list
        Pool->FreeNodeLists[SizeClass] = FirstNode->NormalHeader.Next;

        NextNodes->Size = FirstNodeSize-NumBytes-sizeof(NodeHeader);
        AddNodeToFreeList(Pool, NextNodes);

        FirstNode->NormalHeader.ObjectSize = NumBytes;
        DEBUG(printf("alloc Pool:0x%X Bytes:%d -> 0x%X\n", Pool, NumBytes, 
                     &FirstNode->NormalHeader+1));
        return &FirstNode->NormalHeader+1;
      }

      Pool->FreeNodeLists[SizeClass] = FirstNode->NormalHeader.Next; // Unlink
      FirstNode->NormalHeader.ObjectSize = FirstNodeSize;
      DEBUG(printf("alloc Pool:0x%X Bytes:%d -> 0x%X\n", Pool, NumBytes, 
                   &FirstNode->NormalHeader+1));
      return &FirstNode->NormalHeader+1;
    }
  }

  // Perform a search of the free list, taking the front of the first free chunk
  // that is big enough.
  if (NumBytes <= PageSize-sizeof(PoolSlab)-sizeof(NodeHeader)) {
    do {
      FreedNodeHeader **FN = &Pool->FreeNodeLists[SizeClass];
      FreedNodeHeader *FNN = *FN;

      // Search the list for the first-fit
      while (FNN && FNN->Size < NumBytes)
        FN = &FNN->NormalHeader.Next, FNN = *FN;

      if (FNN) {
        // We found a slab big enough.  If it's a perfect fit, just unlink from
        // the free list, otherwise, slice a little bit off and adjust the free
        // list.
        if (FNN->Size > 2*NumBytes+sizeof(NodeHeader)) {
          *FN = FNN->NormalHeader.Next;   // Unlink

          // Put the remainder back on the list...
          FreedNodeHeader *NextNodes =
            (FreedNodeHeader*)((char*)FNN + sizeof(NodeHeader) + NumBytes);
          NextNodes->Size = FNN->Size-NumBytes-sizeof(NodeHeader);
          AddNodeToFreeList(Pool, NextNodes);

          FNN->NormalHeader.ObjectSize = NumBytes;
          DEBUG(printf("alloc Pool:0x%X Bytes:%d -> 0x%X\n", Pool, NumBytes, 
                       &FNN->NormalHeader+1));
          return &FNN->NormalHeader+1;
        } else {
          *FN = FNN->NormalHeader.Next;   // Unlink
          FNN->NormalHeader.ObjectSize = FNN->Size;
          DEBUG(printf("alloc Pool:0x%X Bytes:%d -> 0x%X\n", Pool, NumBytes,
                       &FNN->NormalHeader+1));
          return &FNN->NormalHeader+1;
        }
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
  LAH->Marker = ~0U;
  DEBUG(printf("alloc large Pool:0x%X NumBytes:%d -> 0x%X\n", Pool,
               NumBytes, LAH+1));
  return LAH+1;
}


void poolfree(PoolTy *Pool, void *Node) {
  // If a null pool descriptor is passed in, this is not a pool allocated data
  // structure.  Hand off to the system free.
  if (Pool == 0) { free(Node); return; }
  if (Node == 0) return;

  // Check to see how many elements were allocated to this node...
  FreedNodeHeader *FNH = (FreedNodeHeader*)((char*)Node-sizeof(NodeHeader));
  unsigned Size = FNH->NormalHeader.ObjectSize;

  DEBUG(printf("free  Pool:0x%X <- 0x%X  Size:%d\n", Pool, Node, Size));

  if (Size == ~0U) goto LargeArrayCase;
  
  // If there are already nodes on the freelist, see if these blocks can be
  // coallesced into one of the early blocks on the front of the list.  This is
  // a simple check that prevents many horrible forms of fragmentation.
  //
  for (unsigned SizeClass = 0; SizeClass <= LargeFreeList; ++SizeClass)
    if (FreedNodeHeader *CurFrontNode = Pool->FreeNodeLists[SizeClass]) {
      if ((char*)FNH + sizeof(NodeHeader) + Size == (char*)CurFrontNode) {
        // This node immediately preceeds the node on the front of the
        // free-list.  Remove the current front of the free list, replacing it
        // with the current block.
        FNH->Size = Size + CurFrontNode->Size+sizeof(NodeHeader);
        FNH->NormalHeader.Next = CurFrontNode->NormalHeader.Next;
        Pool->FreeNodeLists[SizeClass] = FNH;
        return;
      }
      
      if ((char*)CurFrontNode + sizeof(NodeHeader) + CurFrontNode->Size ==
          (char*)FNH) {
        // This node immediately follows the node on the front of the free-list.
        // No list manipulation is required.
        CurFrontNode->Size += Size+sizeof(NodeHeader);
        return;
      }
    }

  FNH->Size = Size;
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
