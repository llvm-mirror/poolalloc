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
  unsigned int MaxNodesPerPage = Pool->MaxNodesPerPage;

  // Pointer to the new Slab
  struct SlabHeader * NewSlab;

  // Save locally the node size
  unsigned int NodeSize = Pool->NodeSize;

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
inline struct SlabHeader *
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
inline struct SlabHeader *
DataOwner (unsigned int PageSize, void * p)
{
  return reinterpret_cast<struct SlabHeader *>(reinterpret_cast<unsigned int>(p) & ~(PageSize - 1));
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
#if 0
  Pool->FreeablePool = 1;
#endif /* 0 */

  // Calculate once for this pool the maximum number of nodes per page
  Pool->MaxNodesPerPage = (PageSize - sizeof (struct SlabHeader)) / (sizeof (NodePointer) + NodeSize);

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
#if 0
  Pool->FreeablePool = 0;
#endif
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

  // Slab Pointer
  struct SlabHeader * Slabp;

  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  // Make sure we allocate something
  BytesWanted = (BytesWanted ? BytesWanted : 1);

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
  Slabp = Pool->Slabs;
  if (Slabp == NULL)
  {
    Pool->Slabs = Slabp = createSlab (Pool);
    (Slabp->NextFreeData)++;
    return (Slabp->Data);
  }

  //
  // Determine whether we can allocate from the current slab.
  //
  if (Slabp->NextFreeData < Slabp->NodesPerSlab)
  {
    //
    // Return the block and increment the index of the next free data block.
    //
    Data = (Slabp->Data + (Pool->NodeSize * Slabp->NextFreeData));
    (Slabp->NextFreeData)++;
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
    Slabp = createSlab (Pool);
    Slabp->Next = Pool->Slabs;
    Pool->Slabs = Slabp;

    (Slabp->NextFreeData)++;

    //
    // Return the block and increment the index of the next free data block.
    //
    return (Slabp->Data);
  }

  //
  // Determine which slab owns this block.
  //
  Slabp = BlockOwner (PageSize, Pool->FreeList);

  //
  // Find the data block that corresponds with this pointer.
  //
  Data = (Slabp->Data + (Pool->NodeSize * (Pool->FreeList.Next - &(Slabp->BlockList[0]))));

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
  struct SlabHeader ** Prevp = &(Pool->ArraySlabs);

  //
  // Check to see if we have an array slab that has extra space that we
  // can use.
  //
  if ((Slabp != NULL) &&
      ((Pool->MaxNodesPerPage - Slabp->NextFreeData) >= ArraySize))
  {
    //
    // Increase the reference count for this slab.
    //
    Slabp->LiveNodes++;

    //
    // Return the data to the caller.
    //
    void * Data = (Slabp->Data + (Pool->NodeSize * Slabp->NextFreeData));
    (Slabp->NextFreeData)++;
    return (Data);
  }

  //
  // Scan through all the free array slabs to see if they are large
  // enough.
  //
  for (; Slabp != NULL; Slabp=Slabp->Next)
  {
    //
    // Check to see if this slab has enough room.
    //
    if (Slabp->NodesPerSlab >= ArraySize)
    {
      //
      // Make the previous node point to the next node.
      //
      (*Prevp)->Next = Slabp->Next;

      //
      // Increase the reference count of the slab.
      //
      ++(Slabp->LiveNodes);

      //
      // Adjust the slab's index of data blocks.
      //
      Slabp->NextFreeData = ArraySize;

      //
      // Return the slab's data.
      //
      return (Slabp->Data);
    }

    //
    // Move on to the next node.
    //
    Prevp = &(Slabp->Next);
  }

  //
  // Create a new slab and mark it as an array.
  //
  Slabp = createSlab (Pool, ArraySize);
  Slabp->IsArray = 1;
  Slabp->LiveNodes = 1;
  Slabp->NextFreeData = ArraySize;

  //
  // If the array has some space, link it into the array "free" list.
  //
  if ((Slabp->IsManaged == 0) && (Slabp->NextFreeData != Pool->MaxNodesPerPage))
  {
    Slabp->Next = Pool->ArraySlabs;
    Pool->ArraySlabs = Slabp;
  }

  //
  // Return the list of blocks to the caller.
  //
  return (Slabp->Data);
}

void
poolfree (PoolTy * Pool, void * Block)
{
  assert(Pool && "Null pool pointer passed in to poolfree!\n");
  assert(Block && "Null block pointer passed in to poolfree!\n");

  //
  // Find the header of the memory block.
  //
  struct SlabHeader * slabp = DataOwner (PageSize, Block);

  //
  // If the owning slab is an array, add it back to the free array list.
  //
  if (slabp->IsArray)
  {
    if ((--slabp->LiveNodes) == 0)
    {
      slabp->Next = Pool->ArraySlabs;
      Pool->ArraySlabs = slabp;
    }
    return;
  }

  //
  // Find the node pointer that corresponds to this data block.
  //
  NodePointer Node;
  Node.Next = &(slabp->BlockList[((unsigned char *)Block - slabp->Data)/Pool->NodeSize]);

  //
  // Add the node back to the free list.
  //
  Node.Next->Next = Pool->FreeList.Next;
  Pool->FreeList.Next = Node.Next;

  return;
}

