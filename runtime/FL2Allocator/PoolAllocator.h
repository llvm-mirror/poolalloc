//===- PoolAllocator.h - Pool allocator runtime interface file --*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines the interface which is implemented by the LLVM pool
// allocator runtime library.
//
// Note: Most of this runtime library is templated based on a PoolTraits
// instance.  This allows the normal pool allocator to use standard pointers and
// long's to represent things, but allows the pointer compression runtime
// library use pool indexes which are smaller.  Using smaller indexes reduces
// the minimum object size on a 64-bit system from 16 to 8 bytes, and reduces
// the object header size to 4 bytes (from 8).
//
//===----------------------------------------------------------------------===//

#ifndef POOLALLOCATOR_RUNTIME_H
#define POOLALLOCATOR_RUNTIME_H

#include <assert.h>
#include <pthread.h>

template<typename PoolTraits>
struct PoolSlab;
template<typename PoolTraits>
struct FreedNodeHeader;

// NormalPoolTraits - This describes normal pool allocation pools, which can
// address the entire heap, and are made out of multiple chunks of memory.  The
// object header is a full machine word, and pointers into the heap are native
// pointers.
struct NormalPoolTraits {
  typedef unsigned long NodeHeaderType;
  enum {
    UseLargeArrayObjects = 1,
    CanGrowPool = 1
  };

  // Pointers are just pointers.
  typedef FreedNodeHeader<NormalPoolTraits>* FreeNodeHeaderPtrTy;

  static const char *getSuffix() { return ""; }

  /// DerefFNHPtr - Given an index into the pool, return a pointer to the
  /// FreeNodeHeader object.
  static FreedNodeHeader<NormalPoolTraits>*
  IndexToFNHPtr(FreeNodeHeaderPtrTy P, void *PoolBase) {
    return P;
  }

  static FreeNodeHeaderPtrTy
  FNHPtrToIndex(FreedNodeHeader<NormalPoolTraits>* FNHP, void *PoolBase) {
    return FNHP;
  }
};


// CompressedPoolTraits - This describes a statically pointer compressed pool,
// which is known to be <= 2^32 bytes in size (even on a 64-bit machine), and is
// made out of a single contiguous block.  The meta-data to represent the pool
// uses 32-bit indexes from the start of the pool instead of full pointers to
// decrease the minimum object size.
struct CompressedPoolTraits {
  typedef unsigned NodeHeaderType;

  enum {
    UseLargeArrayObjects = 0,
    CanGrowPool = 0
  };

  // Represent pointers with indexes from the pool base.
  typedef unsigned FreeNodeHeaderPtrTy;

  static const char *getSuffix() { return "_pc"; }

  /// DerefFNHPtr - Given an index into the pool, return a pointer to the
  /// FreeNodeHeader object.
  static FreedNodeHeader<CompressedPoolTraits>*
  IndexToFNHPtr(FreeNodeHeaderPtrTy P, void *PoolBase) {
    return (FreedNodeHeader<CompressedPoolTraits>*)((char*)PoolBase + P);
  }

  static FreeNodeHeaderPtrTy
  FNHPtrToIndex(FreedNodeHeader<CompressedPoolTraits>* FNHP, void *PoolBase) {
    assert(FNHP && PoolBase && "Can't handle null FHNP!");
    return (char*)FNHP - (char*)PoolBase;
  }
};


// NodeHeader - Each block of memory is preceeded in the the pool by one of
// these headers.
template<typename PoolTraits>
struct NodeHeader {
  typename PoolTraits::NodeHeaderType Size;
};


// When objects are on the free list, we pretend they have this header.  
template<typename PoolTraits>
struct FreedNodeHeader {
  // NormalHeader - This is the normal node header that is on allocated or free
  // blocks.
  NodeHeader<PoolTraits> Header;

  // Next - The next object in the free list.
  typename PoolTraits::FreeNodeHeaderPtrTy Next;

  // Prev - The node that points to this node on the free list.  This is null
  // if it is the first node in one of the two free lists.
  typename PoolTraits::FreeNodeHeaderPtrTy Prev;
};


// Large Arrays are passed on to directly malloc, and are not necessarily page
// aligned.  These arrays are marked by setting the object size preheader to ~1.
// LargeArrays are on their own list to allow for efficient deletion.
struct LargeArrayHeader {
  LargeArrayHeader **Prev, *Next;

  // Size - This contains the size of the object.
  unsigned long Size;
  
  // Marker: this is the ObjectSize marker which MUST BE THE LAST ELEMENT of
  // this header!
  unsigned long Marker;

  void UnlinkFromList() {
    *Prev = Next;
    if (Next)
      Next->Prev = Prev;
  }

  void LinkIntoList(LargeArrayHeader **List) {
    Next = *List;
    if (Next)
      Next->Prev = &Next;
    *List = this;
    Prev = List;
  }
};


template<typename PoolTraits>
struct PoolTy {
  // Slabs - the list of slabs in this pool.  NOTE: This must remain the first
  // memory of this structure for the pointer compression pass.
  PoolSlab<PoolTraits> *Slabs;

  // The free node lists for objects of various sizes.  
  typename PoolTraits::FreeNodeHeaderPtrTy ObjFreeList;
  typename PoolTraits::FreeNodeHeaderPtrTy OtherFreeList;

  // Alignment - The required alignment of allocations the pool in bytes.
  unsigned Alignment;

  // The declared size of the pool, just kept for the record.
  unsigned DeclaredSize;

  // LargeArrays - A doubly linked list of large array chunks, dynamically
  // allocated with malloc.
  LargeArrayHeader *LargeArrays;

  // The size to allocate for the next slab.
  unsigned AllocSize;

  // NumObjects - the number of poolallocs for this pool.
  unsigned NumObjects;

  // BytesAllocated - The total number of bytes ever allocated from this pool.
  // Together with NumObjects, allows us to calculate average object size.
  unsigned BytesAllocated;

  // Lock for the pool
  pthread_mutex_t pool_lock;

  // Thread reference count for the pool
  int thread_refcount;
};

extern "C" {
  void poolinit(PoolTy<NormalPoolTraits> *Pool,
                unsigned DeclaredSize, unsigned ObjAlignment);
  void poolmakeunfreeable(PoolTy<NormalPoolTraits> *Pool);
  void pooldestroy(PoolTy<NormalPoolTraits> *Pool);
  void *poolalloc(PoolTy<NormalPoolTraits> *Pool, unsigned NumBytes);
  void *poolcalloc(PoolTy<NormalPoolTraits> *Pool, unsigned NumBytes, unsigned);
  void *poolrealloc(PoolTy<NormalPoolTraits> *Pool,
                    void *Node, unsigned NumBytes);
  void *poolmemalign(PoolTy<NormalPoolTraits> *Pool,
                     unsigned Alignment, unsigned NumBytes);
  void poolfree(PoolTy<NormalPoolTraits> *Pool, void *Node);

  /// poolobjsize - Return the size of the object at the specified address, in
  /// the specified pool.  Note that this cannot be used in normal cases, as it
  /// is completely broken if things land in the system heap.  Perhaps in the
  /// future.  :(
  ///
  unsigned poolobjsize(PoolTy<NormalPoolTraits> *Pool, void *Node);

  // Bump pointer pool library.  This is a pool implementation that does not
  // support frees or reallocs to the pool.  As such, it can be much more
  // efficient and simpler than a general pool implementation.
  void poolinit_bp(PoolTy<NormalPoolTraits> *Pool, unsigned ObjAlignment);
  void *poolalloc_bp(PoolTy<NormalPoolTraits> *Pool, unsigned NumBytes);
  void pooldestroy_bp(PoolTy<NormalPoolTraits> *Pool);


  // Pointer Compression runtime library.  Most of these are just wrappers
  // around the normal pool routines.
  void *poolinit_pc(PoolTy<CompressedPoolTraits> *Pool, unsigned NodeSize,
                    unsigned ObjAlignment);
  void pooldestroy_pc(PoolTy<CompressedPoolTraits> *Pool);
  unsigned long long poolalloc_pc(PoolTy<CompressedPoolTraits> *Pool,
                                  unsigned NumBytes);
  void poolfree_pc(PoolTy<CompressedPoolTraits> *Pool, unsigned long long Node);
  //void *poolmemalign_pc(PoolTy *Pool, unsigned Alignment, unsigned NumBytes);
  unsigned long long poolrealloc_pc(PoolTy<CompressedPoolTraits> *Pool,
                                  unsigned long long Node, unsigned NumBytes);

  // Alternate Pointer Compression runtime library.  Most of these are just 
  // wrappers around the normal pool routines.
  void *poolinit_pca(PoolTy<CompressedPoolTraits> *Pool, unsigned NodeSize,
                    unsigned ObjAlignment);
  void pooldestroy_pca(PoolTy<CompressedPoolTraits> *Pool);
  void* poolalloc_pca(PoolTy<CompressedPoolTraits> *Pool,
                                  unsigned NumBytes);
  void poolfree_pca(PoolTy<CompressedPoolTraits> *Pool, void* Node);
  void* poolrealloc_pca(PoolTy<CompressedPoolTraits> *Pool,
                                  void* Node, unsigned NumBytes);

  // Access tracing runtime library support.
  void poolaccesstraceinit(void);
  void poolaccesstrace(void *Ptr, void *PD);

  // Auxiliary functions for thread support
#ifdef USE_DYNCALL
  int poolalloc_pthread_create(pthread_t* thread,
							   const pthread_attr_t* attr,
							   void *(*start_routine)(void*), int num_pools, ...);
#endif
}

#endif

