//===- PoolAllocatorChained.cpp - Implementation of poolallocator runtime -===//
// 
//                     The LLVM Compiler Infrastructure
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

#undef assert
#define assert(X)

// In the current implementation, each slab in the pool has NODES_PER_SLAB nodes
// unless the isSingleArray flag is set in which case it contains a single array
// of size ArraySize. Small arrays (size <= NODES_PER_SLAB) are still allocated
// in the slabs of size NODES_PER_SLAB
//
static const unsigned NODES_PER_SLAB = 4096;

// PoolSlab Structure - Hold NODES_PER_SLAB objects of the current node type.
//   Invariants: FirstUnused <= UsedEnd
//
struct PoolSlab {
  PoolSlab *Next;
  unsigned isSingleArray;   // If this slab is used for exactly one array

  unsigned FirstUnused;     // First empty node in slab

  // UsedEnd - 1 past the last allocated node in slab. 0 if slab is empty
  unsigned UsedEnd;

  // AllocatedBitVector - One bit is set for every node in this slab which has
  // been allocated.
  unsigned char AllocatedBitVector[NODES_PER_SLAB/8];

  // StartOfAllocation - A bit is set if this is the start of an allocation,
  // either a unit allocation or an array.
  unsigned char StartOfAllocation[NODES_PER_SLAB/8];

  // The array is allocated from the start to the end of the slab
  unsigned ArraySize;       // The size of the array allocated 

  char Data[1];   // Buffer to hold data in this slab... VARIABLE SIZED


  bool isNodeAllocated(unsigned NodeNum) {
    return AllocatedBitVector[NodeNum >> 3] & (1 << (NodeNum & 7));
  }

  void markNodeAllocated(unsigned NodeNum) {
    AllocatedBitVector[NodeNum >> 3] |= 1 << (NodeNum & 7);
  }

  void markNodeFree(unsigned NodeNum) {
    AllocatedBitVector[NodeNum >> 3] &= ~(1 << (NodeNum & 7));
  }

  bool isStartOfAllocation(unsigned NodeNum) {
    return StartOfAllocation[NodeNum >> 3] & (1 << (NodeNum & 7));
  }
  
  void setStartBit(unsigned NodeNum) {
    StartOfAllocation[NodeNum >> 3] |= 1 << (NodeNum & 7);
  }

  void clearStartBit(unsigned NodeNum) {
    StartOfAllocation[NodeNum >> 3] &= ~(1 << (NodeNum & 7));
  }
};

// poolinit - Initialize a pool descriptor to empty
//
void poolinit(PoolTy *Pool, unsigned NodeSize) {
  assert(Pool && "Null pool pointer passed into poolinit!\n");

  // We must alway return unique pointers, even if they asked for 0 bytes
  Pool->NodeSize = NodeSize ? NodeSize : 1;
  Pool->Slabs = 0;
  Pool->FreeablePool = 1;
}

void poolmakeunfreeable(PoolTy *Pool) {
  assert (Pool && "Null pool pointer passed in to poolmakeunfreeable!\n");
  Pool->FreeablePool = 0;
}

// pooldestroy - Release all memory allocated for a pool
//
void pooldestroy(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to pooldestroy!\n");

  PoolSlab *PS = (PoolSlab*)Pool->Slabs;
  while (PS) {
    PoolSlab *Next = PS->Next;
    free(PS);
    PS = Next;
  }
}

static void *FindSlabEntry(PoolSlab *PS, unsigned NodeSize) {

  // Loop through all of the slabs looking for one with an opening */
  for (; PS; PS = PS->Next) {
    // If the slab is a single array, go on to the next slab.  Don't allocate
    // single nodes in a SingleArray slab.
    if (PS->isSingleArray) 
      continue;

    // Check to see if there are empty entries at the end of the slab...
    if (PS->UsedEnd < NODES_PER_SLAB) {
      // Mark the returned entry used
      PS->markNodeAllocated(PS->UsedEnd);
      PS->setStartBit(PS->UsedEnd);

      // If we are allocating out the first unused field, bump its index also
      if (PS->FirstUnused == (unsigned)PS->UsedEnd)
        PS->FirstUnused++;

      // Return the entry, increment UsedEnd field.
      return &PS->Data[0] + PS->UsedEnd++ * NodeSize;
    }

    // If not, check to see if this node has a declared "FirstUnused" value that
    // is less than the number of nodes allocated...
    //
    if (PS->FirstUnused < NODES_PER_SLAB) {
      // Successfully allocate out the first unused node
      unsigned Idx = PS->FirstUnused;

      PS->markNodeAllocated(Idx);
      PS->setStartBit(Idx);

      // Increment FirstUnused to point to the new first unused value...
      do {
        ++PS->FirstUnused;
      } while (PS->FirstUnused < NODES_PER_SLAB &&
               PS->isNodeAllocated(PS->FirstUnused));

      return &PS->Data[0] + Idx*NodeSize;
    }
  }

  // No empty nodes available, must grow # slabs!
  return 0;
}

void *poolalloc(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  unsigned NodeSize = Pool->NodeSize;
  PoolSlab *CurPoolSlab = (PoolSlab*)Pool->Slabs;

  // Fastpath for allocation in the common case.
  if (CurPoolSlab && !CurPoolSlab->isSingleArray &&
      CurPoolSlab->UsedEnd < NODES_PER_SLAB) {
    // Mark the returned entry used
    CurPoolSlab->markNodeAllocated(CurPoolSlab->UsedEnd);
    CurPoolSlab->setStartBit(CurPoolSlab->UsedEnd);
    
    // If we are allocating out the first unused field, bump its index also
    if (CurPoolSlab->FirstUnused == CurPoolSlab->UsedEnd)
      CurPoolSlab->FirstUnused++;
    
    // Return the entry, increment UsedEnd field.
    return &CurPoolSlab->Data[0] + CurPoolSlab->UsedEnd++ * NodeSize;
  }

  if (void *Result = FindSlabEntry(CurPoolSlab, NodeSize))
    return Result;

  // Otherwise we must allocate a new slab and add it to the list
  PoolSlab *PS = (PoolSlab*)malloc(sizeof(PoolSlab)+NodeSize*NODES_PER_SLAB-1);
  assert(PS && "poolalloc: Could not allocate memory!");

  // Initialize the slab to indicate that the first element is allocated
  PS->FirstUnused = 1;
  PS->UsedEnd = 1;

  // This is not a single array
  PS->isSingleArray = 0;
  PS->ArraySize = 0;
  
  PS->markNodeAllocated(0);
  PS->setStartBit(0);

  // Add the slab to the list...
  PS->Next = CurPoolSlab;
  Pool->Slabs = PS;
  return &PS->Data[0];
}

void poolfree(PoolTy *Pool, char *Node) {
  assert(Pool && "Null pool pointer passed in to poolfree!\n");

  // Return if this pool has size 0
  unsigned NodeSize = Pool->NodeSize;
  if (NodeSize == 0)
    return;

  PoolSlab *PS = (PoolSlab*)Pool->Slabs;
  PoolSlab **PPS = (PoolSlab**)&Pool->Slabs;

  // Search for the slab that contains this node...
  while (&PS->Data[0] > Node || &PS->Data[NodeSize*NODES_PER_SLAB-1] < Node) {
    assert(PS && "poolfree: node being free'd not found in allocation "
           " pool specified!\n");

    PPS = &PS->Next;
    PS = PS->Next;
  }

  // PS now points to the slab where Node is
  unsigned Idx = (Node-&PS->Data[0])/NodeSize;
  assert(Idx < NODES_PER_SLAB && "Pool slab searching loop broken!");

  if (PS->isSingleArray) {

    // If this slab is a SingleArray
    assert(Idx == 0 && "poolfree: Attempt to free middle of allocated array\n");
    assert(PS->isNodeAllocated(0) &&
           "poolfree: Attempt to free node that is already freed\n");

    // Mark this SingleArray slab as being free by just marking the first
    // entry as free
    PS->markNodeFree(0);
  } else {
    
    // If this slab is not a SingleArray
    assert(PS->isStartOfAllocation(Idx) &&
           "poolfree: Attempt to free middle of allocated array\n");

    // Free the first node
    assert(PS->isNodeAllocated(Idx) &&
           "poolfree: Attempt to free node that is already freed\n");

    PS->clearStartBit(Idx);
    PS->markNodeFree(Idx);
    
    // Free all nodes 
    unsigned idxiter = Idx + 1;
    while (idxiter < NODES_PER_SLAB && (!PS->isStartOfAllocation(idxiter)) && 
	   (PS->isNodeAllocated(idxiter))) {
      PS->markNodeFree(idxiter);
      ++idxiter;
    }

    // Update the first free field if this node is below the free node line
    if (Idx < PS->FirstUnused) PS->FirstUnused = Idx;
    
    // If we are not freeing the last element in a slab...
    if (idxiter != PS->UsedEnd)
      return;

    // Otherwise we are freeing the last element in a slab... shrink the
    // UsedEnd marker down to last used node.
    PS->UsedEnd = Idx+1;
    do {
      --PS->UsedEnd;
      // FIXME, this should scan the allocated array an entire byte at a time 
      // for performance!
      //
    } while (PS->UsedEnd && !PS->isNodeAllocated(PS->UsedEnd-1));
    
    assert(PS->FirstUnused <= PS->UsedEnd &&
	   "FirstUnused field was out of date!");
  }
    
  // Ok, if this slab is empty, we unlink it from the of slabs and either move
  // it to the head of the list, or free it, depending on whether or not there
  // is already an empty slab at the head of the list.
  //
  if (Pool->FreeablePool) {    // Do this only if the pool is freeable
    if (PS->isSingleArray) {
      // If it is a SingleArray, just free it
      *PPS = PS->Next;
      free(PS);
    } else if (PS->UsedEnd == 0) {   // Empty slab?
      PoolSlab *HeadSlab;
      *PPS = PS->Next;   // Unlink from the list of slabs...
      
      HeadSlab = (PoolSlab*)Pool->Slabs;
      if (HeadSlab && HeadSlab->UsedEnd == 0) {// List already has empty slab?
	free(PS);                               // Free memory for slab
      } else {
	PS->Next = HeadSlab;                    // No empty slab yet, add this
	Pool->Slabs = PS;                       // one to the head of the list
      }
    }
  } else {
    // Pool is not freeable for safety reasons
    // Leave it in the list of PoolSlabs as an empty PoolSlab
    if (!PS->isSingleArray)
      if (PS->UsedEnd == 0) {
	PS->FirstUnused = 0;
	
	// Do not free the pool, but move it to the head of the list if there is
	// no empty slab there already
	PoolSlab *HeadSlab;
	HeadSlab = (PoolSlab*)Pool->Slabs;
	if (HeadSlab && HeadSlab->UsedEnd != 0) {
	  PS->Next = HeadSlab;
	  Pool->Slabs = PS;
	}
      }
  }
}

// The poolallocarray version of FindSlabEntry
static void *FindSlabEntryArray(PoolSlab *PS, unsigned NodeSize, unsigned Size){
  // Loop through all of the slabs looking for one with an opening
  for (; PS; PS = PS->Next) {
    
    // For large array allocation
    if (Size > NODES_PER_SLAB) {
      // If this slab is a SingleArray that is free with size > Size, use it
      if (PS->isSingleArray && !PS->isNodeAllocated(0) &&PS->ArraySize >= Size){
	// Allocate the array in this slab
        // In a single array, only the first node needs to be marked
	PS->markNodeAllocated(0);
	return &PS->Data[0];
      } else
	continue;
    } else if (PS->isSingleArray)
      continue; // Do not allocate small arrays in SingleArray slabs

    // For small array allocation, check to see if there are empty entries at
    // the end of the slab...
    if (PS->UsedEnd-1 < NODES_PER_SLAB-Size) {
      // Mark the returned entry used and set the start bit
      PS->setStartBit(PS->UsedEnd);
      for (unsigned i = PS->UsedEnd; i < PS->UsedEnd + Size; ++i)
	PS->markNodeAllocated(i);

      // If we are allocating out the first unused field, bump its index also
      if (PS->FirstUnused == PS->UsedEnd)
        PS->FirstUnused += Size;

      // Increment UsedEnd
      PS->UsedEnd += Size;

      // Return the entry
      return &PS->Data[0] + (PS->UsedEnd - Size) * NodeSize;
    }

    // If not, check to see if this node has a declared "FirstUnused" value
    // starting which Size nodes can be allocated
    //
    if (PS->FirstUnused < NODES_PER_SLAB - Size + 1 &&
	(PS->UsedEnd < PS->FirstUnused+1 || 
	 PS->UsedEnd - PS->FirstUnused >= Size+1)) {
      unsigned Idx = PS->FirstUnused, foundArray;
      
      // Check if there is a continuous array of Size nodes starting FirstUnused
      foundArray = 1;
      for (unsigned i = Idx; (i < Idx + Size) && foundArray; ++i)
	if (PS->isNodeAllocated(i))
	  foundArray = 0;

      if (foundArray) {
	// Successfully allocate starting from the first unused node
	PS->setStartBit(Idx);
	for (unsigned i = Idx; i < Idx + Size; ++i)
	  PS->markNodeAllocated(i);
	
	PS->FirstUnused += Size;
	while (PS->FirstUnused < NODES_PER_SLAB &&
               PS->isNodeAllocated(PS->FirstUnused)) {
	  ++PS->FirstUnused;
	}
	return &PS->Data[0] + Idx*NodeSize;
      }
      
    }
  }

  // No empty nodes available, must grow # slabs!
  return 0;
}

void* poolallocarray(PoolTy* Pool, unsigned Size) {
  assert(Pool && "Null pool pointer passed into poolallocarray!\n");

  unsigned NodeSize = Pool->NodeSize;

  // Return if this pool has size 0
  if (NodeSize == 0)
    return 0;

  PoolSlab *PS = (PoolSlab*)Pool->Slabs;
  if (void *Result = FindSlabEntryArray(PS, NodeSize,Size))
    return Result;

  // Otherwise we must allocate a new slab and add it to the list
  if (Size > NODES_PER_SLAB) {
    // Allocate a new slab of size Size
    PS = (PoolSlab*)malloc(sizeof(PoolSlab)+NodeSize*Size-1);
    assert(PS && "poolallocarray: Could not allocate memory!\n");
    PS->isSingleArray = 1;
    PS->ArraySize = Size;
    PS->markNodeAllocated(0);
  } else {
    PS = (PoolSlab*)malloc(sizeof(PoolSlab)+NodeSize*NODES_PER_SLAB-1);
    assert(PS && "poolallocarray: Could not allocate memory!\n");

    // Initialize the slab to indicate that the first element is allocated
    PS->FirstUnused = Size;
    PS->UsedEnd = Size;
    
    PS->isSingleArray = 0;
    PS->ArraySize = 0;

    PS->setStartBit(0);
    for (unsigned i = 0; i != Size; ++i)
      PS->markNodeAllocated(i);
  }

  // Add the slab to the list...
  PS->Next = (PoolSlab*)Pool->Slabs;
  Pool->Slabs = PS;
  return &PS->Data[0];
}
