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

//===----------------------------------------------------------------------===//
//
//  PoolSlab implementation
//
//===----------------------------------------------------------------------===//

#define PageSize (18U*1024U)

// PoolSlab Structure - Hold multiple objects of the current node type.
// Invariants: FirstUnused <= UsedEnd
//
struct PoolSlab {
  // Next - This link is used when we need to traverse the list of slabs in a
  // pool, for example, to destroy them all.
  PoolSlab *Next;

public:
  static void create(PoolTy *Pool);
  void destroy();

  PoolSlab *getNext() const { return Next; }
};

// create - Create a new (empty) slab and add it to the end of the Pools list.
void PoolSlab::create(PoolTy *Pool) {
  PoolSlab *PS = (PoolSlab*)malloc(PageSize);

  // Add the body of the slab to the free list...
  FreedNodeHeader *SlabBody = (FreedNodeHeader*)(PS+1);
  SlabBody->Size = PageSize-sizeof(PoolSlab)-sizeof(FreedNodeHeader);
  SlabBody->NormalHeader.Next = Pool->FreeNodeList;
  Pool->FreeNodeList = SlabBody;

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
void poolinit(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed into poolinit!\n");
  Pool->Slabs = 0;
  Pool->FreeNodeList = 0;
  Pool->LargeArrays = 0;
}

// pooldestroy - Release all memory allocated for a pool
//
void pooldestroy(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");

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
  assert(Pool && "Null pool pointer passed in to poolalloc!\n");
  if (NumBytes == 0)
    NumBytes = 4;
  else
    NumBytes = (NumBytes+3) & ~3;  // Round up to 4 bytes...

  // Fast path.  In the common case, we can allocate a portion of the node at
  // the front of the free list.
  if (Pool->FreeNodeList) {
    FreedNodeHeader *FirstNode = Pool->FreeNodeList;
    unsigned FirstNodeSize = FirstNode->Size;
    if (FirstNodeSize > 2*NumBytes+sizeof(NodeHeader)) {
      // Put the remainder back on the list...
      FreedNodeHeader *NextNodes =
        (FreedNodeHeader*)((char*)FirstNode + sizeof(NodeHeader) + NumBytes);
      NextNodes->Size = FirstNodeSize-NumBytes-sizeof(NodeHeader);
      NextNodes->NormalHeader.Next = FirstNode->NormalHeader.Next;
      Pool->FreeNodeList = NextNodes;
      FirstNode->NormalHeader.ObjectSize = NumBytes;
      return &FirstNode->NormalHeader+1;
    } else if (FirstNodeSize > NumBytes) {
      Pool->FreeNodeList = FirstNode->NormalHeader.Next;   // Unlink
      FirstNode->NormalHeader.ObjectSize = FirstNodeSize;
      return &FirstNode->NormalHeader+1;
    }
  }

  // Perform a search of the free list, taking the front of the first free chunk
  // that is big enough.
  if (NumBytes <= PageSize-sizeof(PoolSlab)-sizeof(NodeHeader)) {
    do {
      FreedNodeHeader **FN = &Pool->FreeNodeList;
      FreedNodeHeader *FNN = *FN;

      // Search the list for the first-fit
      while (FNN && FNN->Size < NumBytes)
        FN = &FNN->NormalHeader.Next, FNN = *FN;

      if (FNN) {
        // We found a slab big enough.  If it's a perfect fit, just unlink from
        // the free list, otherwise, slice a little bit off and adjust the free
        // list.
        if (FNN->Size > 2*NumBytes+sizeof(NodeHeader)) {
         // Put the remainder back on the list...
          FreedNodeHeader *NextNodes =
            (FreedNodeHeader*)((char*)FNN + sizeof(NodeHeader) + NumBytes);
          NextNodes->Size = FNN->Size-NumBytes-sizeof(NodeHeader);
          NextNodes->NormalHeader.Next = FNN->NormalHeader.Next;
          *FN = NextNodes;
          FNN->NormalHeader.ObjectSize = NumBytes;
          return &FNN->NormalHeader+1;
        } else {
          *FN = FNN->NormalHeader.Next;   // Unlink
          FNN->NormalHeader.ObjectSize = FNN->Size;
          return &FNN->NormalHeader+1;
        }
      }

      // Oops, we didn't find anything on the free list big enough!  Allocate
      // another slab and try again.
      PoolSlab::create(Pool);
    } while (1);
  }

  // Otherwise, the allocation is a large array.  Since we're not going to be
  // able to help much for this allocation, simply pass it on to malloc.
  LargeArrayHeader *LAH = (LargeArrayHeader*)malloc(sizeof(LargeArrayHeader) + 
                                                    NumBytes);
  LAH->Next = Pool->LargeArrays;
  Pool->LargeArrays = LAH;
  LAH->Prev = &Pool->LargeArrays;
  LAH->Marker = ~0U;
  return LAH+1;
}


void poolfree(PoolTy *Pool, void *Node) {
  assert(Pool && "Null pool pointer passed in to poolfree!\n");

  // Check to see how many elements were allocated to this node...
  FreedNodeHeader *FNH = (FreedNodeHeader*)((char*)Node-sizeof(NodeHeader));
  unsigned Size = FNH->NormalHeader.ObjectSize;
  if (Size == ~0U) {
    // Must unlink from list.
    fprintf(stderr, "CANNOT FREE LARGE ARRAY YET!\n");
    return;
  }
  
  // If there are already nodes on the freelist, see if this blocks should be
  // coallesced into one of the early blocks on the front of the list.  This is
  // a simple check that prevents many horrible forms of fragmentation.
  //
  if (FreedNodeHeader *CurFrontNode = Pool->FreeNodeList) {
    if ((char*)FNH + sizeof(NodeHeader) + Size == (char*)CurFrontNode) {
      // This node immediately preceeds the node on the front of the
      // free-list.  Remove the current front of the free list, replacing it
      // with the current block.
      FNH->Size = Size + CurFrontNode->Size+sizeof(NodeHeader);
      FNH->NormalHeader.Next = CurFrontNode->NormalHeader.Next;
      Pool->FreeNodeList = FNH;
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
  FNH->NormalHeader.Next = Pool->FreeNodeList;
  Pool->FreeNodeList = FNH;
}
