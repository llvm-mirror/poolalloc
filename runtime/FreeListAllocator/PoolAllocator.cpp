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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#undef assert
#define assert(X)

typedef union
{
  unsigned char * header;
  unsigned char ** next;
} NodePointer;

//===----------------------------------------------------------------------===//
//
//  PoolSlab implementation
//
//===----------------------------------------------------------------------===//
struct SlabHeader
{
  // Number of nodes per slab
  unsigned int NodesPerSlab;

  // The size of each node
  unsigned int NodeSize;

  // Reference Count
  unsigned int LiveNodes;

  // Pointer to the next slab
  struct SlabHeader * Next;

  // Pointer to the list of nodes
  unsigned char Data [];
};

//
// Function: createSlab ()
//
// Description:
//  Allocate memory for a new slab and initialize the slab.
//
struct SlabHeader *
createSlab (unsigned int NodeSize)
{
  // Pointer to the new Slab
  struct SlabHeader * NewSlab;

  // The number of elements in the slab
  const unsigned int NodesPerSlab = 128;

  // Pointers and index for initializing memory
  NodePointer p;

  //
  // Determine the size of the slab.
  //
  int slab_size = ((sizeof (unsigned char *) + NodeSize) * NodesPerSlab) +
                   sizeof (struct SlabHeader);

  //
  // Allocate a piece of memory for the new slab.
  //
  NewSlab = (struct SlabHeader *) malloc (slab_size);
  assert (NewSlab != NULL);

  //
  // Initialize the contents of the slab.
  //
  NewSlab->NodeSize = NodeSize;
  NewSlab->NodesPerSlab = NodesPerSlab;
  NewSlab->LiveNodes = 0;
  NewSlab->Next = NULL;

  //
  // Initialize each node in the list.
  //
  p.header = &(NewSlab->Data[0]);
  while (p.header < (&(NewSlab->Data[0]) + slab_size - sizeof (struct SlabHeader)))
  {
    //
    // Calculate the position of the next header and put its address in
    // this current header.
    //
    *(p.next) = p.header + (sizeof (unsigned char *) + NodeSize);

    //
    // Move on to the next header.
    //
    p.header = *(p.next);
  }

  p.header = (&(NewSlab->Data[0]) + slab_size - sizeof (struct SlabHeader)
                                     - sizeof (unsigned char *)
                                     - NodeSize);
  *(p.next) = NULL;

  return NewSlab;
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
  Pool->FreeList = NULL;
  Pool->FreeablePool = 1;

  return;
}

void poolmakeunfreeable(PoolTy *Pool)
{
  assert(Pool && "Null pool pointer passed in to poolmakeunfreeable!\n");
  Pool->FreeablePool = 0;
}

// pooldestroy - Release all memory allocated for a pool
//
void pooldestroy(PoolTy *Pool)
{
  // Pointer to scan Slab list
  struct SlabHeader * Slabp;

  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");
  for (Slabp = Pool->Slabs; Slabp != NULL; Slabp=Slabp->Next)
  {
    free (Slabp);
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
  if (Pool->FreeList == NULL)
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
    Pool->FreeList = &(Pool->Slabs->Data[0]);
  }

  //
  // Increase the slab's reference count.
  //
#if 0
  slabAlloc (Pool->FreeList->Slab);
#endif /* 0 */

  //
  // Grab the first element from the free list and return it.
  //
  NodePointer MemoryBlock;

  MemoryBlock.header = Pool->FreeList;
  Pool->FreeList=*(MemoryBlock.next);
  return (MemoryBlock.header += sizeof (unsigned char *));
}

void *
poolallocarray(PoolTy* Pool, unsigned Size)
{
  assert(Pool && "Null pool pointer passed into poolallocarray!\n");
  assert (0 && "Not implemented yet")
  return NULL;
}

void
poolfree (PoolTy * Pool, void * Block)
{
  assert(Pool && "Null pool pointer passed in to poolfree!\n");
  assert(Block && "Null pool pointer passed in to poolfree!\n");

  // Pointer to the node corresponding to this memory block
  NodePointer Node;

  //
  // Find the header of the memory block.
  //
  Node.header = (unsigned char *)(Block) - (sizeof (unsigned char *));

#if 0
  //
  // Decrease the slab's reference count.
  //
  slabFree (Node.header->Slab);
#endif /* 0 */

  //
  // Add the node back to the free list.
  //
  *(Node.next) = Pool->FreeList;
  Pool->FreeList = Node.header;

  return;
}

