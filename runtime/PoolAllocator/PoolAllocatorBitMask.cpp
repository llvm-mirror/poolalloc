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

//#undef assert
//#define assert(X)

//===----------------------------------------------------------------------===//
//
//  PoolSlab implementation
//
//===----------------------------------------------------------------------===//


// PoolSlab Structure - Hold multiple objects of the current node type.
// Invariants: FirstUnused <= UsedEnd
//
struct PoolSlab {
  // In the current implementation, each slab in the pool has NodesPerSlab
  // nodes unless the isSingleArray flag is set in which case it contains a
  // single array of size ArraySize. Small arrays (size <= NodesPerSlab) are
  // still allocated in the slabs of size NodesPerSlab
  //
  static const unsigned NodesPerSlab = 4096;

  PoolSlab *Next;
  unsigned isSingleArray;   // If this slab is used for exactly one array

  unsigned short FirstUnused;     // First empty node in slab

  // UsedEnd - 1 past the last allocated node in slab. 0 if slab is empty
  unsigned short UsedEnd;

  // AllocatedBitVector - One bit is set for every node in this slab which has
  // been allocated.
  unsigned char AllocatedBitVector[NodesPerSlab/8];

  // StartOfAllocation - A bit is set if this is the start of an allocation,
  // either a unit allocation or an array.
  unsigned char StartOfAllocation[NodesPerSlab/8];

  char Data[1];   // Buffer to hold data in this slab... VARIABLE SIZED

  bool isNodeAllocated(unsigned NodeNum) {
    return AllocatedBitVector[NodeNum >> 3] & (1 << (NodeNum & 7));
  }

  void markNodeAllocated(unsigned NodeNum) {
    AllocatedBitVector[NodeNum >> 3] |= 1 << (NodeNum & 7);
  }

  void setStartBit(unsigned NodeNum) {
    StartOfAllocation[NodeNum >> 3] |= 1 << (NodeNum & 7);
  }

private:
  bool isStartOfAllocation(unsigned NodeNum) {
    return StartOfAllocation[NodeNum >> 3] & (1 << (NodeNum & 7));
  }
  
  void markNodeFree(unsigned NodeNum) {
    AllocatedBitVector[NodeNum >> 3] &= ~(1 << (NodeNum & 7));
  }

  void clearStartBit(unsigned NodeNum) {
    StartOfAllocation[NodeNum >> 3] &= ~(1 << (NodeNum & 7));
  }

public:
  // create - Create a new (empty) slab and add it to the end of the Pools list.
  static PoolSlab *create(PoolTy *Pool);

  // destroy - Release the memory for the current object.
  void destroy() {
    free(this);
  }

  // isEmpty - This is a quick check to see if this slab is completely empty or
  // not.
  bool isEmpty() const { return UsedEnd == 0; }

  // allocateSingle - Allocate a single element from this pool, returning -1 if
  // there is no space.
  int allocateSingle();

  // getElementAddress - Return the address of the specified element.
  void *getElementAddress(unsigned ElementNum, unsigned ElementSize) {
    return &Data[ElementNum*ElementSize];
  }

  // containsElement - Return the element number of the specified address in
  // this slab.  If the address is not in slab, return -1.
  int containsElement(void *Ptr, unsigned ElementSize) const;

  // freeElement - Free the single node, small array, or entire array indicated.
  void freeElement(unsigned ElementIdx);
};


// create - Create a new (empty) slab and add it to the end of the Pools list.
PoolSlab *PoolSlab::create(PoolTy *Pool) {
  PoolSlab *PS = (PoolSlab*)malloc(sizeof(PoolSlab) +
                                   Pool->NodeSize*NodesPerSlab-1);
  assert(PS && "poolalloc: Could not allocate memory!");

  PS->isSingleArray = 0;  // Not a single array!
  PS->FirstUnused = 0;    // Nothing allocated.
  PS->UsedEnd     = 0;    // Nothing allocated.

  // Add the slab to the list...
  PS->Next = (PoolSlab*)Pool->Slabs;
  Pool->Slabs = PS;
  return PS;
}

// allocateSingle - Allocate a single element from this pool, returning -1 if
// there is no space.
int PoolSlab::allocateSingle() {
  // If the slab is a single array, go on to the next slab.  Don't allocate
  // single nodes in a SingleArray slab.
  if (isSingleArray) return -1;

  // Check to see if there are empty entries at the end of the slab...
  if (UsedEnd < NodesPerSlab) {
    // Mark the returned entry used
    markNodeAllocated(UsedEnd);
    setStartBit(UsedEnd);
    
    // If we are allocating out the first unused field, bump its index also
    if (FirstUnused == UsedEnd)
      FirstUnused++;
    
    // Return the entry, increment UsedEnd field.
    return UsedEnd++;
  }
  
  // If not, check to see if this node has a declared "FirstUnused" value that
  // is less than the number of nodes allocated...
  //
  if (FirstUnused < NodesPerSlab) {
    // Successfully allocate out the first unused node
    unsigned Idx = FirstUnused;
    
    markNodeAllocated(Idx);
    setStartBit(Idx);
    
    // Increment FirstUnused to point to the new first unused value...
    do {
      ++FirstUnused;
    } while (FirstUnused < NodesPerSlab && isNodeAllocated(FirstUnused));
    
    return Idx;
  }
  
  return -1;
}

// containsElement - Return the element number of the specified address in
// this slab.  If the address is not in slab, return -1.
int PoolSlab::containsElement(void *Ptr, unsigned ElementSize) const {
  if (&Data[0] > Ptr || &Data[ElementSize*NodesPerSlab-1] < Ptr)
    return -1;

  unsigned Index = (char*)Ptr-(char*)&Data[0];
  assert(Index % ElementSize == 0 &&
         "Freeing pointer into the middle of an element!");
  Index /= ElementSize;
  assert(Index < PoolSlab::NodesPerSlab && "Pool slab searching loop broken!");
  return Index;
}


// freeElement - Free the single node, small array, or entire array indicated.
void PoolSlab::freeElement(unsigned ElementIdx) {
  assert(isNodeAllocated(ElementIdx) &&
         "poolfree: Attempt to free node that is already freed\n");

  // Mark this element as being free!
  markNodeFree(ElementIdx);

  // If this slab is a SingleArray, there is nothing else to do.
  if (isSingleArray) {
    assert(ElementIdx == 0 &&
           "poolfree: Attempt to free middle of allocated array\n");
    return;
  }

  // If this slab is not a SingleArray
  assert(isStartOfAllocation(ElementIdx) &&
         "poolfree: Attempt to free middle of allocated array\n");
  
  // Free the first cell
  clearStartBit(ElementIdx);
  markNodeFree(ElementIdx);
  
  // Free all nodes if this was a small array allocation.
  unsigned ElementEndIdx = ElementIdx + 1;
  while (ElementEndIdx < UsedEnd && !isStartOfAllocation(ElementEndIdx) && 
         isNodeAllocated(ElementEndIdx)) {
    markNodeFree(ElementEndIdx);
    ++ElementEndIdx;
  }
  
  // Update the first free field if this node is below the free node line
  if (ElementIdx < FirstUnused) FirstUnused = ElementIdx;
  
  // If we are freeing the last element in a slab, shrink the UsedEnd marker
  // down to the last used node.
  if (ElementEndIdx == UsedEnd) {
    UsedEnd = ElementIdx;
    do {
      --UsedEnd;
      // FIXME, this should scan the allocated array an entire byte at a time 
      // for performance when skipping large empty blocks!
    } while (UsedEnd && !isNodeAllocated(UsedEnd-1));
    
    assert(FirstUnused <= UsedEnd &&
           "FirstUnused field was out of date!");
  }
}


//===----------------------------------------------------------------------===//
//
//  Pool allocator library implementation
//
//===----------------------------------------------------------------------===//

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
    PS->destroy();
    PS = Next;
  }
}

void *poolalloc(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  unsigned NodeSize = Pool->NodeSize;
  PoolSlab *PS = (PoolSlab*)Pool->Slabs;

  // Fastpath for allocation in the common case.
  if (PS) {
    int Element = PS->allocateSingle();
    if (Element != -1)
      return PS->getElementAddress(Element, NodeSize);
    PS = PS->Next;
  }

  // Loop through all of the slabs looking for one with an opening
  for (; PS; PS = PS->Next) {
    int Element = PS->allocateSingle();
    if (Element != -1)
      return PS->getElementAddress(Element, NodeSize);
  }

  // Otherwise we must allocate a new slab and add it to the list
  PoolSlab *New = PoolSlab::create(Pool);
  int Idx = New->allocateSingle();
  assert(Idx == 0 && "New allocation didn't return zero'th node?");
  return New->getElementAddress(0, 0);
}

void poolfree(PoolTy *Pool, void *Node) {
  assert(Pool && "Null pool pointer passed in to poolfree!\n");
  unsigned NodeSize = Pool->NodeSize;

  PoolSlab *PS = (PoolSlab*)Pool->Slabs;
  PoolSlab **PPS = (PoolSlab**)&Pool->Slabs;

  // Search for the slab that contains this node...
  int Idx = PS->containsElement(Node, NodeSize);
  for (; Idx == -1; PPS = &PS->Next, PS = PS->Next) {
    assert(PS && "poolfree: node being free'd not found in allocation "
           " pool specified!\n");
    Idx = PS->containsElement(Node, NodeSize);
  }

  // Free the actual element now!
  PS->freeElement(Idx);

  // Ok, if this slab is empty, we unlink it from the of slabs and either move
  // it to the head of the list, or free it, depending on whether or not there
  // is already an empty slab at the head of the list.
  //
  if (Pool->FreeablePool) {    // Do this only if the pool is freeable
    if (PS->isSingleArray) {
      // If it is a SingleArray, just free it
      *PPS = PS->Next;
      PS->destroy();
    } else if (PS->isEmpty()) {   // Empty slab?
      PoolSlab *HeadSlab;
      *PPS = PS->Next;   // Unlink from the list of slabs...
      
      HeadSlab = (PoolSlab*)Pool->Slabs;
      if (HeadSlab && HeadSlab->isEmpty()) { // List already has empty slab?
	PS->destroy();                       // Free memory for slab
      } else {
	PS->Next = HeadSlab;                 // No empty slab yet, add this
	Pool->Slabs = PS;                    // one to the head of the list
      }
    }
  } else {
    // Pool is not freeable for safety reasons
    // Leave it in the list of PoolSlabs as an empty PoolSlab
    if (!PS->isSingleArray)
      if (PS->isEmpty()) {
	PS->FirstUnused = 0;
	
	// Do not free the pool, but move it to the head of the list if there is
	// no empty slab there already
	PoolSlab *HeadSlab;
	HeadSlab = (PoolSlab*)Pool->Slabs;
	if (HeadSlab && !HeadSlab->isEmpty()) {
	  PS->Next = HeadSlab;
	  Pool->Slabs = PS;
	}
      }
  }
}

// The poolallocarray version of FindSlabEntry
static void *FindSlabEntryArray(PoolSlab *PS, unsigned NodeSize, unsigned Size){
  if (Size > PoolSlab::NodesPerSlab) return 0;

  // Loop through all of the slabs looking for one with an opening
  for (; PS; PS = PS->Next) {
    if (PS->isSingleArray)
      continue; // Do not allocate small arrays in SingleArray slabs

    // For small array allocation, check to see if there are empty entries at
    // the end of the slab...
    if (PS->UsedEnd-1 < PoolSlab::NodesPerSlab-Size) {
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
    if (PS->FirstUnused < PoolSlab::NodesPerSlab - Size + 1 &&
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
	while (PS->FirstUnused < PoolSlab::NodesPerSlab &&
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
  if (Size > PoolSlab::NodesPerSlab) {
    // Allocate a new slab of size Size
    PS = (PoolSlab*)malloc(sizeof(PoolSlab)+NodeSize*Size-1);
    assert(PS && "poolallocarray: Could not allocate memory!\n");
    PS->isSingleArray = 1;
    PS->markNodeAllocated(0);
  } else {
    PS = (PoolSlab*)malloc(sizeof(PoolSlab)+NodeSize*PoolSlab::NodesPerSlab-1);
    assert(PS && "poolallocarray: Could not allocate memory!\n");

    // Initialize the slab to indicate that the first element is allocated
    PS->FirstUnused = Size;
    PS->UsedEnd = Size;
    
    PS->isSingleArray = 0;

    PS->setStartBit(0);
    for (unsigned i = 0; i != Size; ++i)
      PS->markNodeAllocated(i);
  }

  // Add the slab to the list...
  PS->Next = (PoolSlab*)Pool->Slabs;
  Pool->Slabs = PS;
  return &PS->Data[0];
}
