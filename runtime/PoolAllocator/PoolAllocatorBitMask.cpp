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
// This uses the 'Ptr1' field to maintain a linked list of slabs that are either
// empty or are partially allocated from.  The 'Ptr2' field of the PoolTy is
// used to track a linked list of slabs which are full, ie, all elements have
// been allocated from them.
//
//===----------------------------------------------------------------------===//

#include "PoolAllocator.h"
#include "PageManager.h"
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


// PoolSlab Structure - Hold multiple objects of the current node type.
// Invariants: FirstUnused <= UsedEnd
//
struct PoolSlab {
  PoolSlab **PrevPtr, *Next;
  bool isSingleArray;   // If this slab is used for exactly one array

private:
  // FirstUnused - First empty node in slab
  unsigned short FirstUnused;

  // UsedEnd - 1 past the last allocated node in slab. 0 if slab is empty
  unsigned short UsedEnd;

  // NumNodesInSlab - This contains the number of nodes in this slab, which
  // effects the size of the NodeFlags vector, and indicates the number of nodes
  // which are in the slab.
  unsigned short NumNodesInSlab;

  // NodeFlagsVector - This array contains two bits for each node in this pool
  // slab.  The first (low address) bit indicates whether this node has been
  // allocated, and the second (next higher) bit indicates whether this is the
  // start of an allocation.
  //
  // This is a variable sized array, which has 2*NumNodesInSlab bits (rounded up
  // to 4 bytes).
  unsigned NodeFlagsVector[];
  
  bool isNodeAllocated(unsigned NodeNum) {
    return NodeFlagsVector[NodeNum/16] & (1 << (NodeNum & 15));
  }

  void markNodeAllocated(unsigned NodeNum) {
    NodeFlagsVector[NodeNum/16] |= 1 << (NodeNum & 15);
  }

  void markNodeFree(unsigned NodeNum) {
    NodeFlagsVector[NodeNum/16] &= ~(1 << (NodeNum & 15));
  }

  void setStartBit(unsigned NodeNum) {
    NodeFlagsVector[NodeNum/16] |= 1 << ((NodeNum & 15)+16);
  }

  bool isStartOfAllocation(unsigned NodeNum) {
    return NodeFlagsVector[NodeNum/16] & (1 << ((NodeNum & 15)+16));
  }
  
  void clearStartBit(unsigned NodeNum) {
    NodeFlagsVector[NodeNum/16] &= ~(1 << ((NodeNum & 15)+16));
  }

public:
  // create - Create a new (empty) slab and add it to the end of the Pools list.
  static PoolSlab *create(PoolTy *Pool);

  // createSingleArray - Create a slab for a large singlearray with NumNodes
  // entries in it, returning the pointer into the pool directly.
  static void *createSingleArray(PoolTy *Pool, unsigned NumNodes);

  // getSlabSize - Return the number of nodes that each slab should contain.
  static unsigned getSlabSize(PoolTy *Pool) {
    // We need space for the header...
    unsigned NumNodes = PageSize-sizeof(PoolSlab);
    
    // We need space for the NodeFlags...
    unsigned NodeFlagsBytes = NumNodes/Pool->NodeSize * 2 / 8;
    NumNodes -= (NodeFlagsBytes+3) & ~3;  // Round up to int boundaries.

    // Divide the remainder among the nodes!
    return NumNodes / Pool->NodeSize;
  }

  void addToList(PoolSlab **PrevPtrPtr) {
    PoolSlab *InsertBefore = *PrevPtrPtr;
    *PrevPtrPtr = this;
    PrevPtr = PrevPtrPtr;
    Next = InsertBefore;
    if (InsertBefore) InsertBefore->PrevPtr = &Next;
  }

  void unlinkFromList() {
    *PrevPtr = Next;
    if (Next) Next->PrevPtr = PrevPtr;
  }

  unsigned getSlabSize() const {
    return NumNodesInSlab;
  }

  // destroy - Release the memory for the current object.
  void destroy();

  // isEmpty - This is a quick check to see if this slab is completely empty or
  // not.
  bool isEmpty() const { return UsedEnd == 0; }

  // isFull - This is a quick check to see if the slab is completely allocated.
  //
  bool isFull() const { return isSingleArray || FirstUnused == getSlabSize(); }

  // allocateSingle - Allocate a single element from this pool, returning -1 if
  // there is no space.
  int allocateSingle();

  // allocateMultiple - Allocate multiple contiguous elements from this pool,
  // returning -1 if there is no space.
  int allocateMultiple(unsigned Num);

  // getElementAddress - Return the address of the specified element.
  void *getElementAddress(unsigned ElementNum, unsigned ElementSize) {
    char *Data = (char*)&NodeFlagsVector[((unsigned)NumNodesInSlab+15)/16];
    return &Data[ElementNum*ElementSize];
  }
  const void *getElementAddress(unsigned ElementNum, unsigned ElementSize)const{
    const char *Data =
      (const char *)&NodeFlagsVector[(unsigned)(NumNodesInSlab+15)/16];
    return &Data[ElementNum*ElementSize];
  }

  // containsElement - Return the element number of the specified address in
  // this slab.  If the address is not in slab, return -1.
  int containsElement(void *Ptr, unsigned ElementSize) const;

  // freeElement - Free the single node, small array, or entire array indicated.
  void freeElement(unsigned ElementIdx);
  
  // lastNodeAllocated - Return one past the last node in the pool which is
  // before ScanIdx, that is allocated.  If there are no allocated nodes in this
  // slab before ScanIdx, return 0.
  unsigned lastNodeAllocated(unsigned ScanIdx);
};

// create - Create a new (empty) slab and add it to the end of the Pools list.
PoolSlab *PoolSlab::create(PoolTy *Pool) {
  unsigned NodesPerSlab = getSlabSize(Pool);

  unsigned Size = sizeof(PoolSlab) + 4*((NodesPerSlab+15)/16) +
                  Pool->NodeSize*getSlabSize(Pool);
  assert(Size < PageSize && "Trying to allocate a slab larger than a page!");
  PoolSlab *PS = (PoolSlab*)AllocatePage();

  PS->NumNodesInSlab = NodesPerSlab;
  PS->isSingleArray = 0;  // Not a single array!
  PS->FirstUnused = 0;    // Nothing allocated.
  PS->UsedEnd     = 0;    // Nothing allocated.

  // Add the slab to the list...
  PS->addToList((PoolSlab**)&Pool->Ptr1);
  return PS;
}

void *PoolSlab::createSingleArray(PoolTy *Pool, unsigned NumNodes) {
  if (0) {
    printf("PoolSlab::createSingleArray has not been tested and is probably broken!");
    abort();
  }
  unsigned NodesPerSlab = getSlabSize(Pool);
  assert(NumNodes > NodesPerSlab && "No need to create a single array!");
  PoolSlab *PS = (PoolSlab*)malloc(sizeof(PoolSlab) + 4*((NodesPerSlab+15)/16) +
                                   Pool->NodeSize*NumNodes);
  assert(PS && "poolalloc: Could not allocate memory!");

  PS->NumNodesInSlab = NodesPerSlab;  // FIXME: Calculate right.
  PS->isSingleArray = 1;  // Not a single array!
  PS->markNodeAllocated(0);

  // Add the slab to the list...
  PS->addToList((PoolSlab**)&Pool->Ptr1);
  return PS->getElementAddress(0, 0);
}

void PoolSlab::destroy() {
  FreePage(this);
}

// allocateSingle - Allocate a single element from this pool, returning -1 if
// there is no space.
int PoolSlab::allocateSingle() {
  // If the slab is a single array, go on to the next slab.  Don't allocate
  // single nodes in a SingleArray slab.
  if (isSingleArray) return -1;

  unsigned SlabSize = getSlabSize();

  // Check to see if there are empty entries at the end of the slab...
  if (UsedEnd < SlabSize) {
    // Mark the returned entry used
    unsigned short UE = UsedEnd;
    markNodeAllocated(UE);
    setStartBit(UE);
    
    // If we are allocating out the first unused field, bump its index also
    if (FirstUnused == UE)
      FirstUnused++;
    
    // Return the entry, increment UsedEnd field.
    return UsedEnd++;
  }
  
  // If not, check to see if this node has a declared "FirstUnused" value that
  // is less than the number of nodes allocated...
  //
  if (FirstUnused < SlabSize) {
    // Successfully allocate out the first unused node
    unsigned Idx = FirstUnused;
    markNodeAllocated(Idx);
    setStartBit(Idx);
    
    // Increment FirstUnused to point to the new first unused value...
    // FIXME: this should be optimized
    unsigned short FU = FirstUnused;
    do {
      ++FU;
    } while (FU != SlabSize && isNodeAllocated(FU));
    FirstUnused = FU;
    
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
  if (UsedEnd+Size <= getSlabSize()) {
    // Mark the returned entry used and set the start bit
    unsigned UE = UsedEnd;
    setStartBit(UE);
    for (unsigned i = UE; i != UE+Size; ++i)
      markNodeAllocated(i);
    
    // If we are allocating out the first unused field, bump its index also
    if (FirstUnused == UE)
      FirstUnused += Size;

    // Increment UsedEnd
    UsedEnd += Size;

    // Return the entry
    return UE;
  }

  // If not, check to see if this node has a declared "FirstUnused" value
  // starting which Size nodes can be allocated
  //
  unsigned Idx = FirstUnused;
  while (Idx+Size <= getSlabSize()) {
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
    while (Idx+Size <= getSlabSize() && isNodeAllocated(Idx))
      ++Idx;
  }

  return -1;
}


// containsElement - Return the element number of the specified address in
// this slab.  If the address is not in slab, return -1.
int PoolSlab::containsElement(void *Ptr, unsigned ElementSize) const {
  const void *FirstElement = getElementAddress(0, 0);
  if (FirstElement > Ptr || 
      (char*)getElementAddress(ElementSize, getSlabSize())-1 < Ptr)
    return -1;

  unsigned Index = (char*)Ptr-(char*)FirstElement;
  assert(Index % ElementSize == 0 &&
         "Freeing pointer into the middle of an element!");
  Index /= ElementSize;
  assert(Index < getSlabSize() && "Pool slab searching loop broken!");
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

  // FIXME: This should use manual strength reduction to produce decent code.
  unsigned UE = UsedEnd;
  while (ElementEndIdx != UE &&
         !isStartOfAllocation(ElementEndIdx) && 
         isNodeAllocated(ElementEndIdx)) {
    markNodeFree(ElementEndIdx);
    ++ElementEndIdx;
  }
  
  // Update the first free field if this node is below the free node line
  if (ElementIdx < FirstUnused) FirstUnused = ElementIdx;
  
  // If we are freeing the last element in a slab, shrink the UsedEnd marker
  // down to the last used node.
  if (ElementEndIdx == UE) {
    UsedEnd = lastNodeAllocated(ElementIdx);
    assert(FirstUnused <= UsedEnd &&
           "FirstUnused field was out of date!");
  }
}

unsigned PoolSlab::lastNodeAllocated(unsigned ScanIdx) {
  // Check the last few nodes in the current word of flags...
  unsigned CurWord = ScanIdx/16;
  unsigned short Flags = NodeFlagsVector[CurWord] & 0xFFFF;
  if (Flags) {
    // Mask off nodes above this one
    Flags &= (1 << (ScanIdx & 15))-1;
    if (Flags) {
      // If there is still something in the flags vector, then there is a node
      // allocated in this part.  The goto is a hack to get the uncommonly
      // executed code away from the common code path.
      goto ContainsAllocatedNode;
    }
  }

  // Ok, the top word doesn't contain anything, scan the whole flag words now.
  ScanIdx &= ~15;
  --CurWord;
  while (CurWord != ~0U) {
    if (NodeFlagsVector[CurWord] & 0xFFFF)
      goto ContainsAllocatedNode;
    CurWord--;
    ScanIdx -= 16;
  }
  return 0;

ContainsAllocatedNode:
  // Figure out exactly which node is allocated in this word now.
  // FIXME: this could be made to be more efficient!
  do {
    --ScanIdx;
  } while (ScanIdx && !isNodeAllocated(ScanIdx-1));
  return ScanIdx;
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

  // Ensure the page manager is initialized
  InitializePageManager();

  // We must alway return unique pointers, even if they asked for 0 bytes
  Pool->NodeSize = NodeSize ? NodeSize : 1;
  Pool->Ptr1 = Pool->Ptr2 = 0;
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

  // Free any partially allocated slabs
  PoolSlab *PS = (PoolSlab*)Pool->Ptr1;
  while (PS) {
    PoolSlab *Next = PS->Next;
    PS->destroy();
    PS = Next;
  }

  // Free the completely allocated slabs
  PS = (PoolSlab*)Pool->Ptr2;
  while (PS) {
    PoolSlab *Next = PS->Next;
    PS->destroy();
    PS = Next;
  }
}

void *poolalloc(PoolTy *Pool) {
  assert(Pool && "Null pool pointer passed in to poolalloc!\n");

  PoolSlab *PS = (PoolSlab*)Pool->Ptr1;

  if (__builtin_expect(PS != 0, 1)) {
    int Element = PS->allocateSingle();
    if (__builtin_expect(Element != -1, 1)) {
      // We allocated an element.  Check to see if this slab has been
      // completely filled up.  If so, move it to the Ptr2 list.
      if (__builtin_expect(PS->isFull(), false)) {
        PS->unlinkFromList();
        PS->addToList((PoolSlab**)&Pool->Ptr2);
      }
      
      return PS->getElementAddress(Element, Pool->NodeSize);
    }

    // Loop through all of the slabs looking for one with an opening
    for (PS = PS->Next; PS; PS = PS->Next) {
      int Element = PS->allocateSingle();
      if (Element != -1) {
        // We allocated an element.  Check to see if this slab has been
        // completely filled up.  If so, move it to the Ptr2 list.
        if (PS->isFull()) {
          PS->unlinkFromList();
          PS->addToList((PoolSlab**)&Pool->Ptr2);
        }
        
        return PS->getElementAddress(Element, Pool->NodeSize);
      }
    }
  }

  // Otherwise we must allocate a new slab and add it to the list
  PoolSlab *New = PoolSlab::create(Pool);
  int Idx = New->allocateSingle();
  assert(Idx == 0 && "New allocation didn't return zero'th node?");
  return New->getElementAddress(0, 0);
}

void *poolallocarray(PoolTy* Pool, unsigned Size) {
  assert(Pool && "Null pool pointer passed into poolallocarray!\n");

  // Special case size=1, because poolalloc is much faster than this function.
  if (Size == 1 || Size == 0)
    return poolalloc(Pool);
  if (Size > PoolSlab::getSlabSize(Pool))
    return PoolSlab::createSingleArray(Pool, Size);    

  PoolSlab *PS = (PoolSlab*)Pool->Ptr1;

  // Loop through all of the slabs looking for one with an opening
  for (; PS; PS = PS->Next) {
    int Element = PS->allocateMultiple(Size);
    if (Element != -1) {
      // We allocated an element.  Check to see if this slab has been completely
      // filled up.  If so, move it to the Ptr2 list.
      if (PS->isFull()) {
        PS->unlinkFromList();
        PS->addToList((PoolSlab**)&Pool->Ptr2);
      }
      return PS->getElementAddress(Element, Pool->NodeSize);
    }
  }
  
  PoolSlab *New = PoolSlab::create(Pool);
  int Idx = New->allocateMultiple(Size);
  assert(Idx == 0 && "New allocation didn't return zero'th node?");
  return New->getElementAddress(0, 0);
}

// SearchForContainingSlab - Do a brute force search through the list of
// allocated slabs for the node in question.
//
static PoolSlab *SearchForContainingSlab(PoolTy *Pool, void *Node,
                                         unsigned &TheIndex) {
  PoolSlab *PS = (PoolSlab*)Pool->Ptr1;
  unsigned NodeSize = Pool->NodeSize;

  // Search the partially allocated slab list for the slab that contains this
  // node.
  int Idx = -1;
  if (PS) {               // Pool->Ptr1 could be null if Ptr2 isn't
    for (; PS; PS = PS->Next) {
      Idx = PS->containsElement(Node, NodeSize);
      if (Idx != -1) break;
    }
  }

  // If the partially allocated slab list doesn't contain it, maybe the
  // completely allocated list does.
  if (PS == 0) {
    PS = (PoolSlab*)Pool->Ptr2;
    assert(Idx == -1 && "Found node but don't have PS?");
    
    while (1) {
      assert(PS && "poolfree: node being free'd not found in allocation "
             " pool specified!\n");
      Idx = PS->containsElement(Node, NodeSize);
      if (Idx != -1) break;
      PS = PS->Next;
    }
  }
  TheIndex = Idx;
  return PS;
}

void poolfree(PoolTy *Pool, void *Node) {
  assert(Pool && "Null pool pointer passed in to poolfree!\n");

  PoolSlab *PS;
  unsigned Idx;
  if (0) {                  // THIS SHOULD BE SET FOR SAFECODE!
    unsigned TheIndex;
    PS = SearchForContainingSlab(Pool, Node, TheIndex);
    Idx = TheIndex;
  } else {
    // Since it is undefined behavior to free a node which has not been
    // allocated, we know that the pointer coming in has to be a valid node
    // pointer in the pool.  Mask off some bits of the address to find the base
    // of the pool.
    assert((PageSize & PageSize-1) == 0 && "Page size is not a power of 2??");
    PS = (PoolSlab*)((long)Node & ~(PageSize-1));
    Idx = PS->containsElement(Node, Pool->NodeSize);
    assert((int)Idx != -1 && "Node not contained in slab??");
  }

  // If PS was full, it must have been in list #2.  Unlink it and move it to
  // list #1.
  if (PS->isFull()) {
    // Now that we found the node, we are about to free an element from it.
    // This will make the slab no longer completely full, so we must move it to
    // the other list!
    PS->unlinkFromList(); // Remove it from the Ptr2 list.

    PoolSlab **InsertPosPtr = (PoolSlab**)&Pool->Ptr1;

    // If the partially full list has an empty node sitting at the front of the
    // list, insert right after it.
    if ((*InsertPosPtr)->isEmpty())
      InsertPosPtr = &(*InsertPosPtr)->Next;

    PS->addToList(InsertPosPtr);     // Insert it now in the Ptr1 list.
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
        PS->unlinkFromList();
        PS->destroy();
        return;
      } else {
        // If this is a non-freeable pool, we might as well use the memory
        // allocated for normal node allocations.
        PS->isSingleArray = 0;
      }

    // No more singlearray objects exist at this point.
    assert(!PS->isSingleArray);

    PS->unlinkFromList();   // Unlink from the list of slabs...
    
    // If we can free this pool, check to see if there are any empty slabs at
    // the start of this list.  If so, delete the FirstSlab!
    PoolSlab *FirstSlab = (PoolSlab*)Pool->Ptr1;
    if (Pool->FreeablePool && FirstSlab->isEmpty()) {
      // Here we choose to delete FirstSlab instead of the pool we just freed
      // from because the pool we just freed from is more likely to be in the
      // processor cache.
      FirstSlab->unlinkFromList();
      FirstSlab->destroy();
    }

    // Link our slab onto the head of the list so that allocations will find it
    // efficiently.    
    PS->addToList((PoolSlab**)&Pool->Ptr1);
  }
}
