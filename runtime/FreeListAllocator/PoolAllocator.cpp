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
#include <stdio.h>

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
createSlab (PoolTy * Pool, unsigned int NodesPerSlab = 0)
{
  // Maximum number of nodes per page
  unsigned int MaxNodesPerPage;

  // Pointer to the new Slab
  struct SlabHeader * NewSlab;

  // Save locally the node size
  unsigned int NodeSize = Pool->NodeSize;
  unsigned int PageSize = Pool->PageSize;

  //
  // Determine how many nodes can exist within a regular slab.
  //
  MaxNodesPerPage = (PageSize - sizeof (struct SlabHeader)) / (sizeof (NodePointer) + NodeSize);

  //
  // If we can't fit a node into a page, give up.
  //
  if (NodeSize > PageSize)
  {
    fprintf (stderr, "Node size %d is larger than page size %d.\n", NodeSize, PageSize);
    fflush (stderr);
    abort();
  }

  if (MaxNodesPerPage == 0)
  {
    fprintf (stderr, "Node size is too large\n");
    fflush (stderr);
    abort();
  }

  //
  // Allocate the memory for the slab and initialize its contents.
  //
  if (NodesPerSlab > MaxNodesPerPage)
  {
    NewSlab = (struct SlabHeader *) GetPages ((NodeSize * NodesPerSlab / PageSize) + 1);
    if (NewSlab == NULL)
    {
      fprintf (stderr, "Failed large allocation\n");
      fflush (stderr);
      abort();
    }
    NewSlab->IsArray = 1;
    NewSlab->IsManaged = 0;
  }
  else
  {
    NewSlab = (struct SlabHeader *) AllocatePage ();
    if (NewSlab == NULL)
    {
      fprintf (stderr, "Failed regular allocation\n");
      fflush (stderr);
      abort();
    }
    NewSlab->IsArray = 0;
    NewSlab->IsManaged = 1;

    //
    // Bump the number of nodes in the slab up to the maximum.
    //
    NodesPerSlab = MaxNodesPerPage;
  }

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
BlockOwner (unsigned int PageSize, NodePointer p)
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
DataOwner (unsigned int PageSize, void * p)
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

//
// Function: poolinit ()
//
// Description:
//    Initialize a pool descriptor for a new pool.
//
// Inputs:
//    NodeSize - The typical size allocated for this pool.
//
// Outputs:
//    Pool - An initialized pool.
//
void
poolinit (PoolTy *Pool, unsigned int NodeSize)
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
  Pool->PageSize = InitializePageManager ();

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
  struct SlabHeader * Nextp;

  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");

  //
  // Deallocate all of the pages.
  //
  Slabp = Pool->Slabs;
  while (Slabp != NULL)
  {
    // Record the next step
    Nextp = Slabp->Next;

    // Deallocate the memory if it is managed.
    if (Slabp->IsManaged)
    {
      FreePage (Slabp);
    }

    // Move to the next node.
    Slabp = Nextp;
  }

  return;
}

//
// Function: poolalloc ()
//
// Description:
//    Allocates memory from the pool.  Typically, we will allocate memory
//    in the same size chunks as we usually do, but sometimes, we will have to
//    allocate more.
//
void *
poolalloc(PoolTy *Pool, unsigned BytesWanted)
{
  // Pointer to the data block to return
  void * Data;

  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  //
  // Determine if we can satisfy this request normally.  If not, then
  // we need to use the array allocation instead.
  //
  if (Pool->NodeSize < BytesWanted)
  {
    return (poolallocarray (Pool, (BytesWanted+Pool->NodeSize-1)/Pool->NodeSize));
  }

  //
  // If we don't have a slab, this is our first initialization.  Do some
  // quick stuff.
  //
  if (Pool->Slabs == NULL)
  {
    Pool->Slabs = createSlab (Pool);
    (Pool->Slabs->NextFreeData)++;
    return (Pool->Slabs->Data);
  }

  //
  // Determine whether we can allocate from the current slab.
  //
  if (Pool->Slabs->NextFreeData < Pool->Slabs->NodesPerSlab)
  {
    //
    // Return the block and increment the index of the next free data block.
    //
    Data = (Pool->Slabs->Data + (Pool->NodeSize * Pool->Slabs->NextFreeData));
    (Pool->Slabs->NextFreeData)++;
    return (Data);
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
    struct SlabHeader * NewSlab = createSlab (Pool);
    NewSlab->Next = Pool->Slabs;
    Pool->Slabs = NewSlab;

    (NewSlab->NextFreeData)++;

    //
    // Return the block and increment the index of the next free data block.
    //
    return (Pool->Slabs->Data);
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
  struct SlabHeader * slabp = BlockOwner (Pool->PageSize, Pool->FreeList);

  //
  // Find the data block that corresponds with this pointer.
  //
  Data = (slabp->Data + (Pool->NodeSize * (Pool->FreeList.Next - &(slabp->BlockList[0]))));

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
  struct SlabHeader * NewSlab = createSlab (Pool, ArraySize);
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
  struct SlabHeader * slabp = DataOwner (Pool->PageSize, Block);

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

