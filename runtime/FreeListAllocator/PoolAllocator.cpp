//===- PoolAllocatorChained.cpp - Implementation of poolallocator runtime -===//
// 
//                       The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file is yet another implementation of the LLVM pool allocator runtime
// library.
//
//===----------------------------------------------------------------------===//

#include "PoolAllocator.h"
#include "PageManager.h"
#include "PoolSlab.h"
#include <assert.h>
#include <stdlib.h>

#undef assert
#define assert(X)

//===----------------------------------------------------------------------===//
//
//  PoolSlab implementation
//
//===----------------------------------------------------------------------===//

//
// Function: createSlab ()
//
// Description:
//  Allocate memory for a new slab and initialize the slab.
//
struct SlabHeader *
createSlab (unsigned int NodeSize, unsigned int NodesPerSlab = 0)
{
  // Maximum number of nodes per slab
  unsigned int MaxNodesPerSlab;

  // Pointer to the new Slab
  struct SlabHeader * NewSlab;

  // Pointers and index for initializing memory
  NodePointer p;
  unsigned int index;

  //
  // Determine how many nodes should exist within a slab.
  //
  MaxNodesPerSlab = (PageSize - sizeof (struct SlabHeader)) / (sizeof (unsigned char *) + NodeSize);
  assert ((MaxNodesPerSlab >= NodesPerSlab) && "Too many nodes requested");

  //
  // Regardless of what the caller asked for, allocate the maximum number of
  // nodes available in a page.
  //
  // This is the default behavior for regular nodes, and for arrays, we want to
  // round up to the nearest slab so that we don't waste space when we use the
  // array memory.
  //
  // FIXME:
  //  Specifying the number of nodes that you want will come into play once
  //  we allow pool allocations to span multiple memory pages.
  //
  NodesPerSlab = MaxNodesPerSlab;

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
  NewSlab->NodesPerSlab = NodesPerSlab;
  NewSlab->NextFreeData = NewSlab->LiveNodes = 0;
  NewSlab->Next = NULL;
  NewSlab->Data = (unsigned char *)NewSlab + sizeof (struct SlabHeader) + ((NodesPerSlab) * sizeof (NodePointer));

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
  Pool->Slabs = Pool->ArraySlabs = NULL;
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
  // Check to see if we have a slab.  If we don't, get one.
  //
  if (Pool->Slabs == NULL)
  {
    Pool->Slabs = createSlab (Pool->NodeSize);
  }

  //
  // Determine whether we can allocate from the current slab.
  //
  if (Pool->Slabs->NextFreeData < Pool->Slabs->NodesPerSlab)
  {
    //
    // Return the block and increment the index of the next free data block.
    //
    return (Pool->Slabs->Data + (Pool->NodeSize * Pool->Slabs->NextFreeData++));
  }

  //
  // We have a slab, but it doesn't have any new blocks.
  // Check the free list to see if we can use any recycled blocks.
  //
  if (Pool->FreeList.Next == NULL)
  {
    //
    // Create a new slab and add it to the list.
    //
    struct SlabHeader * NewSlab = createSlab (Pool->NodeSize);
    NewSlab->Next = Pool->Slabs;
    Pool->Slabs = NewSlab;

    //
    // Return the block and increment the index of the next free data block.
    //
    return (Pool->Slabs->Data + (Pool->NodeSize * Pool->Slabs->NextFreeData++));
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
// Inputs:
//  Pool - The pool from which to allocate memory.
//  ArraySize - The size of the array in number of elements (not bytes).
//
// FIXME:
//  This algorithm is not very space efficient.  This needs to be fixed.
//
void *
poolallocarray(PoolTy* Pool, unsigned ArraySize)
{
  assert(Pool && "Null pool pointer passed into poolallocarray!\n");

  //
  // Scan the list of array slabs to see if there is one that fits.
  //
  struct SlabHeader * Slabp = Pool->ArraySlabs;
  struct SlabHeader * Prevp = NULL;

  for (; Slabp != NULL; Prevp = Slabp, Slabp=Slabp->Next)
  {
    //
    // Check to see if this slab has enough room.
    //
    if (Slabp->NodesPerSlab >= ArraySize)
    {
      if (Prevp == NULL)
      {
        //
        // This is the first item.  Change the head of the list.
        //
        Pool->ArraySlabs = Slabp->Next;
      }
      else
      {
        //
        // This is some other item.  Modify the preceding item.
        //
        Prevp->Next = Slabp->Next;
      }
      return (&(Slabp->Data[0]));
    }
  }

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
  assert(Block && "Null block pointer passed in to poolfree!\n");

  //
  // Find the header of the memory block.
  //
  struct SlabHeader * slabp = DataOwner (Block);

  //
  // If the owning slab is an array, add it back to the free array list.
  //
  if (slabp->IsArray)
  {
    slabp->Next = Pool->ArraySlabs;
    Pool->ArraySlabs = slabp;
    return;
  }

  //
  // Find the node pointer that corresponds to this data block.
  //
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

