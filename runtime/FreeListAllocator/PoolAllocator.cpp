//===- PoolAllocatorChained.cpp - Implementation of poolallocator runtime -===//
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
#include "PageManager.h"
#include <assert.h>
#include <stdlib.h>

#undef assert
#define assert(X)

//===----------------------------------------------------------------------===//
//
//  PoolSlab implementation
//
//===----------------------------------------------------------------------===//
struct SlabHeader
{
  // Flags whether this is an array
  unsigned char IsArray;

  // Number of nodes per slab
  unsigned int NodesPerSlab;

  // The size of each node
  unsigned int NodeSize;

  // Reference Count
  unsigned int LiveNodes;

  // Pointer to the next slab
  struct SlabHeader * Next;

  // Pointer to the data area (will be in the same page)
  unsigned char * Data;

  // Pointer to the list of nodes
  NodePointer BlockList [];
};

//
// Function: createSlab ()
//
// Description:
//  Allocate memory for a new slab and initialize the slab.
//
struct SlabHeader *
createSlab (unsigned int NodeSize, unsigned int NodesPerSlab = 0)
{
  // Pointer to the new Slab
  struct SlabHeader * NewSlab;

  // Pointers and index for initializing memory
  NodePointer p;
  unsigned int index;

  //
  // Determine how many nodes should exist within a slab.
  //
  if (NodesPerSlab == 0)
  {
    NodesPerSlab = (PageSize - sizeof (struct SlabHeader)) / (sizeof (unsigned char *) + NodeSize);
  }

  //
  // Determine the size of the slab.
  //
  int slab_size = ((sizeof (unsigned char *) + NodeSize) * NodesPerSlab) +
                   sizeof (struct SlabHeader);

  assert (slab_size <= PageSize);

  //
  // Allocate a piece of memory for the new slab.
  //
  NewSlab = (struct SlabHeader *) AllocatePage ();
  assert (NewSlab != NULL);

  //
  // Initialize the contents of the slab.
  //
  NewSlab->IsArray = 0;
  NewSlab->NodeSize = NodeSize;
  NewSlab->NodesPerSlab = NodesPerSlab;
  NewSlab->LiveNodes = 0;
  NewSlab->Next = NULL;
  NewSlab->Data = (unsigned char *)NewSlab + sizeof (struct SlabHeader) + ((NodesPerSlab) * sizeof (NodePointer));

  //
  // Initialize each node in the list.
  //
  for (index = 0; index < NodesPerSlab - 1; index++)
  {
    NewSlab->BlockList[index].Next = &(NewSlab->BlockList[index + 1]);
  }
  NewSlab->BlockList[NodesPerSlab - 1].Next = NULL;

  return NewSlab;
}

//
// Function: BlockOwner ()
//
// Description:
//  Find the slab that owns this block.
//
struct SlabHeader *
BlockOwner (NodePointer p)
{
  //
  // Convert the node pointer into a slab pointer.
  //
  return reinterpret_cast<struct SlabHeader *>(reinterpret_cast<unsigned int>(p.Next) & ~(PageSize - 1));
}

//
// Function: DataOwner ()
//
// Description:
//  This function finds the slab that owns this data block.
//
struct SlabHeader *
DataOwner (void * p)
{
  return reinterpret_cast<struct SlabHeader *>(reinterpret_cast<unsigned int>(p) & ~(PageSize - 1));
}

//
// Function: slabAlloc()
//
// Description:
//  Increase the slab's reference count.
//
void
slabAlloc (SlabHeader * Slab)
{
  Slab->LiveNodes++;
  return;
}

//
// Function: slabFree ()
//
// Description:
//  Decrease the slab's reference count.
//
void
slabFree (SlabHeader * Slab)
{
  Slab->LiveNodes--;
  return;
}

//===----------------------------------------------------------------------===//
//
//  Pool allocator library implementation
//
//===----------------------------------------------------------------------===//

// poolinit - Initialize a pool descriptor to empty
//
void poolinit(PoolTy *Pool, unsigned int NodeSize)
{
  assert(Pool && "Null pool pointer passed into poolinit!\n");

  // We must alway return unique pointers, even if they asked for 0 bytes
  Pool->NodeSize = NodeSize ? NodeSize : 1;
  Pool->Slabs = NULL;
  Pool->FreeList.Next = NULL;
  Pool->FreeablePool = 1;

  //
  // Initialize the page manager.
  //
  InitializePageManager ();

  return;
}

void
poolmakeunfreeable(PoolTy *Pool)
{
  assert(Pool && "Null pool pointer passed in to poolmakeunfreeable!\n");
  Pool->FreeablePool = 0;
}

// pooldestroy - Release all memory allocated for a pool
//
void
pooldestroy(PoolTy *Pool)
{
  // Pointer to scan Slab list
  struct SlabHeader * Slabp;

  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");
  for (Slabp = Pool->Slabs; Slabp != NULL; Slabp=Slabp->Next)
  {
    FreePage (Slabp);
  }

  return;
}

void *
poolalloc(PoolTy *Pool)
{
  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  //
  // If there isn't an available block, we need a new slab.
  //
  if (Pool->FreeList.Next == NULL)
  {
    //
    // Create a new slab and add it to the list.
    //
    struct SlabHeader * NewSlab = createSlab (Pool->NodeSize);
    if (Pool->Slabs == NULL)
    {
      Pool->Slabs = NewSlab;
    }
    else
    {
      NewSlab->Next = Pool->Slabs;
      Pool->Slabs = NewSlab;
    }

    //
    // Take the linked list of nodes inside the slab and add them to the
    // free list.
    //
    Pool->FreeList.Next = &(NewSlab->BlockList[0]);
  }

  //
  // Increase the slab's reference count.
  //
#if 0
  slabAlloc (Pool->FreeList->Slab);
#endif /* 0 */

  //
  // Determine which slab owns this block.
  //
  struct SlabHeader * slabp = BlockOwner (Pool->FreeList);

  //
  // Find the data block that corresponds with this pointer.
  //
  void * Data = (slabp->Data + (Pool->NodeSize * (Pool->FreeList.Next - &(slabp->BlockList[0]))));

  //
  // Unlink the first block.
  //
  Pool->FreeList.Next = Pool->FreeList.Next->Next;

  return Data;
}

//
// Function: poolallocarray ()
//
// Description:
//  Allocate an array of contiguous nodes.
//
// FIXME:
//  This algorithm is not very space efficient.  This needs to be fixed.
//
void *
poolallocarray(PoolTy* Pool, unsigned ArraySize)
{
  assert(Pool && "Null pool pointer passed into poolallocarray!\n");

  //
  // Create a new slab and mark it as an array.
  //
  struct SlabHeader * NewSlab = createSlab (Pool->NodeSize, ArraySize);
  NewSlab->IsArray = 1;

  //
  // Return the list of blocks to the caller.
  //
  return (&(NewSlab->Data[0]));
}

void
poolfree (PoolTy * Pool, void * Block)
{
  assert(Pool && "Null pool pointer passed in to poolfree!\n");
  assert(Block && "Null pool pointer passed in to poolfree!\n");

  //
  // Find the header of the memory block.
  //
  struct SlabHeader * slabp = DataOwner (Block);

  //
  // If the owner is an array, just nuke the whole thing for now.
  // FIXME: Inefficient!  Danger Will Robinson!
  //
  if (slabp->IsArray)
  {
    FreePage (slabp);
    return;
  }

  NodePointer Node;
  Node.Next = &(slabp->BlockList[((unsigned char *)Block - slabp->Data)/Pool->NodeSize]);

#if 0
  //
  // Decrease the slab's reference count.
  //
  slabFree (Node.header->Slab);
#endif /* 0 */

  //
  // Add the node back to the free list.
  //
  Node.Next->Next = Pool->FreeList.Next;
  Pool->FreeList.Next = Node.Next;

  return;
}

