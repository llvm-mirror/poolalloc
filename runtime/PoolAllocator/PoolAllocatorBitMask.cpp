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

private:
  // FirstUnused - First empty node in slab
  unsigned short FirstUnused;

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

  // createSingleArray - Create a slab for a large singlearray with NumNodes
  // entries in it, returning the pointer into the pool directly.
  static void *createSingleArray(PoolTy *Pool, unsigned NumNodes);

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

  // allocateMultiple - Allocate multiple contiguous elements from this pool,
  // returning -1 if there is no space.
  int allocateMultiple(unsigned Num);

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

void *PoolSlab::createSingleArray(PoolTy *Pool, unsigned NumNodes) {
  PoolSlab *PS = (PoolSlab*)malloc(sizeof(PoolSlab) +
                                   Pool->NodeSize*NumNodes-1);
  assert(PS && "poolalloc: Could not allocate memory!");

  PS->isSingleArray = 1;  // Not a single array!
  PS->markNodeAllocated(0);

  // Add the slab to the list...
  PS->Next = (PoolSlab*)Pool->Slabs;
  Pool->Slabs = PS;
  return &PS->Data[0];
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
    // FIXME: this should be optimized
    do {
      ++FirstUnused;
    } while (FirstUnused != NodesPerSlab && isNodeAllocated(FirstUnused));
    
    return Idx;
  }
  
  return -1;
}

// allocateMultiple - Allocate multiple contiguous elements from this pool,
// returning -1 if there is no space.
int PoolSlab::allocateMultiple(unsigned Size) {
  // Do not allocate small arrays in SingleArray slabs
  if (isSingleArray) return -1;

  // For small array allocation, check to see if there are empty entries at the
  // end of the slab...
  if (UsedEnd+Size <= NodesPerSlab) {
    // Mark the returned entry used and set the start bit
    setStartBit(UsedEnd);
    for (unsigned i = UsedEnd; i != UsedEnd + Size; ++i)
      markNodeAllocated(i);
    
    // If we are allocating out the first unused field, bump its index also
    if (FirstUnused == UsedEnd)
      FirstUnused += Size;

    // Increment UsedEnd
    UsedEnd += Size;

    // Return the entry
    return UsedEnd - Size;
  }

  // If not, check to see if this node has a declared "FirstUnused" value
  // starting which Size nodes can be allocated
  //
  unsigned Idx = FirstUnused;
  while (Idx+Size <= NodesPerSlab) {
    assert(!isNodeAllocated(Idx) && "FirstUsed is not accurate!");

    // Check if there is a continuous array of Size nodes starting FirstUnused
    unsigned LastUnused = Idx+1;
    for (; LastUnused != Idx+Size && !isNodeAllocated(LastUnused); ++LastUnused)
      /*empty*/;

    // If we found an unused section of this pool which is large enough, USE IT!
    if (LastUnused == Idx+Size) {
      setStartBit(Idx);
      // FIXME: this loop can be made more efficient!
      for (unsigned i = Idx; i != Idx + Size; ++i)
        markNodeAllocated(i);

      // This should not be allocating on the end of the pool, so we don't need
      // to bump the UsedEnd pointer.
      assert(Idx != UsedEnd && "Shouldn't allocate at end of pool!");

      // If we are allocating out the first unused field, bump its index also.
      if (Idx == FirstUnused)
        FirstUnused += Size;
      
      // Return the entry
      return Idx;
    }

    // Otherwise, try later in the pool.  Find the next unused entry.
    Idx = LastUnused;
    while (Idx+Size <= NodesPerSlab && isNodeAllocated(Idx))
      ++Idx;
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
    FirstUnused = UsedEnd = 0;               // This slab is now empty
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

  // FIXME: This should use manual strength reduction if GCC isn't producing
  // decent code (which is almost certainly isn't).
  while (ElementEndIdx != UsedEnd && !isStartOfAllocation(ElementEndIdx) && 
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
  assert(Pool && "Null pool pointer passed in to poolmakeunfreeable!\n");
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
  if (PS->isEmpty()) {
    if (PS->isSingleArray)
      if (Pool->FreeablePool) {
        // If it is a SingleArray, just free it
        *PPS = PS->Next;
        PS->destroy();
        return;
      } else {
        // If this is a non-freeable pool, we might as well use the memory
        // allocated for normal node allocations.
        PS->isSingleArray = 0;
      }

    // No more singlearray objects exist at this point.
    assert(!PS->isSingleArray);

    *PPS = PS->Next;   // Unlink from the list of slabs...
    PoolSlab *FirstSlab = (PoolSlab*)Pool->Slabs;
    
    // If we can free this pool, check to see if there are any empty slabs at
    // the start of this list.  If so, delete the FirstSlab!
    if (Pool->FreeablePool && FirstSlab->isEmpty()) {
      // Here we choose to delete FirstSlab instead of the pool we just freed
      // from because the pool we just freed from is more likely to be in the
      // processor cache.
      PoolSlab *NextSlab = FirstSlab->Next;
      FirstSlab->destroy();
      FirstSlab = NextSlab;
    }

    // Link our slab onto the head of the list so that allocations will find it
    // efficiently.    
    PS->Next = FirstSlab;
    Pool->Slabs = PS;
  }
}

void *poolallocarray(PoolTy* Pool, unsigned Size) {
  assert(Pool && "Null pool pointer passed into poolallocarray!\n");

  if (Size > PoolSlab::NodesPerSlab)
    return PoolSlab::createSingleArray(Pool, Size);    

  // Loop through all of the slabs looking for one with an opening
  PoolSlab *PS = (PoolSlab*)Pool->Slabs;
  for (; PS; PS = PS->Next) {
    int Element = PS->allocateMultiple(Size);
    if (Element != -1)
      return PS->getElementAddress(Element, Pool->NodeSize);
    PS = PS->Next;
  }
  
  PoolSlab *New = PoolSlab::create(Pool);
  int Idx = New->allocateMultiple(Size);
  assert(Idx == 0 && "New allocation didn't return zero'th node?");
  return New->getElementAddress(0, 0);
}
