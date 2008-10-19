#include "poolalloc_runtime/Support/SplayTree.h"
#include "llvm/ADT/hash_map.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <sys/mman.h>

template<class SlabManager >
class PoolAllocator : SlabManager {
  using SlabManager::slab_alloc;
  using SlabManager::slab_free;
  using SlabManager::slab_valid;
  using SlabManager::slab_getbounds;
  using SlabManager::slab_managed;

 public:
  PoolAllocator(unsigned objsize, unsigned Alignment)
    : SlabManager(objsize, Alignment)
    {}
    
  // In-place new operator
  static void* operator new( std::size_t s, void* p ) throw() {
    return p;
  }
    
  // Allocate num objects of size objsize
  void* alloc(unsigned num = 1) {
    return slab_alloc(num);
  }
    
  // Free allocated object
  void dealloc(void* obj) {
    slab_free(obj);
  }

  // Tests if obj is in an allocated object
  bool isAllocated(void* obj) {
    return slab_valid(obj);
  }

  // Tests if the obj is in the managed set
  bool isManaged(void* obj) {
    return slab_managed(obj);
  }

  // Returns the start and end of an object, return value is true if found
  bool getBounds(void* obj, void*& start, void*& end) {
    return slab_getbounds(obj, start, end);
  }

};

template<class SafeAllocator = std::allocator<void>, class DataAllocator = std::allocator<void> >
class MallocSlabManager {
  typedef typename DataAllocator::template rebind<char>::other DAlloc;
  typedef typename SafeAllocator::template rebind<void>::other SAlloc;

  RangeSplaySet<SAlloc> objs;
  unsigned objsize;

  DAlloc allocator;

  struct dealloc_actor {
    MallocSlabManager* m;
    void operator()(void*& start, void*& end) {
      m->allocator.deallocate((char*) start,((char*)end-(char*)start) + 1);
    }
    dealloc_actor(MallocSlabManager* _m) : m(_m) {}
  };

  public:
  MallocSlabManager(unsigned Osize, unsigned Alignment) : objsize(Osize) {}
  ~MallocSlabManager() {
    dealloc_actor act(this);
    objs.clear(act);
  }
  void* slab_alloc(unsigned num) {
    void* x = allocator.allocate(num*objsize);
    objs.insert(x, (char*)x + num*objsize - 1);
    return x;
  }
  void slab_free(void* obj) {
    void* start; 
    void* end;
    if (objs.find(obj, start, end)) {
      objs.remove(obj);
      allocator.deallocate((char*)start, ((char*)end-(char*)start) + 1);
    }
  }
  bool slab_valid(void* obj) {
    return objs.find(obj);
  }
  bool slab_managed(void* obj) {
    return objs.find(obj);
  }
  bool slab_getbounds(void* obj, void*& start, void*& end) {
    return objs.find(obj, start, end);
  }
};

template<class PageManager, unsigned PageShiftAmount = 6, unsigned load = 80,
  class SafeAllocator = std::allocator<void> >
class BitMaskSlabManager {
  struct slab_metadata {
    void* data;
    unsigned* free_bitmask;
  };

  typedef typename SafeAllocator::template rebind<std::pair<void*, slab_metadata> >::other SAlloc;
  typedef typename SafeAllocator::template rebind<unsigned>::other SBMAlloc;
  typedef hash_map<void*, slab_metadata , hash<void*>, std::equal_to<void*>, SAlloc> MapTy;

  MapTy slabmetadata;
  hash_map<void*, void*, hash<void*>, std::equal_to<void*>, SAlloc> mappedPages;

  SBMAlloc BMAlloc;

  unsigned objsize;
  slab_metadata* CurAllocSlab;
  unsigned totalslots;
  unsigned totalallocs;

  unsigned numObjsPerSlab() const {
    return (PageManager::pageSize << PageShiftAmount) / objsize;
  }

  unsigned numIntsPerSlabMeta() const {
    return (numObjsPerSlab() + (sizeof(unsigned) * 8 - 1)) / (sizeof(unsigned) * 8);
  }

  slab_metadata* getSlabForObj(void* obj) {
    intptr_t ptr = (intptr_t)obj;
    ptr &= ~(PageManager::pageSize - 1);
    void* page = (void*)ptr;
    page = mappedPages[page];
    typename MapTy::iterator ii = slabmetadata.find(page);
    if (ii == slabmetadata.end()) return 0;
    return &ii->second;
  }

  unsigned getObjLoc(slab_metadata* slab, void* obj) {
    return ((char*)obj - (char*)(slab->data)) / objsize;
  }

  unsigned findFree(slab_metadata* slab) const {
    // FIXME: free_bitmask is treated as a linear array.  A bit-tree representation would be faster
    
    for (unsigned y = 0; y < numIntsPerSlabMeta(); ++y) {
      unsigned zone = slab->free_bitmask[y];
      if (zone != ~0)
        for (unsigned x = 0; x < sizeof(unsigned); ++x)
          if ((zone >> (x * 8) & 0x00FF) != 0x00FF)
            for (int z = 0; z < 8; ++z)
              if (!(zone & 1 << (x * 8 + z)))
                return y * sizeof(unsigned) + x * 8 + z;
    }
    return ~0;
  }

  bool isFree(slab_metadata* slab, unsigned loc) const {
    return !(slab->free_bitmask[loc / sizeof(unsigned)] & ( 1 << (loc % sizeof(unsigned))));
  }

  void setFree(slab_metadata* slab, unsigned loc) {
    slab->free_bitmask[loc / sizeof(unsigned)] &= ~( 1 << (loc % sizeof(unsigned)));
    --totalallocs;
  }

  void setUsed(slab_metadata* slab, unsigned loc) {
    slab->free_bitmask[loc / sizeof(unsigned)] |= ( 1 << (loc % sizeof(unsigned)));
    ++totalallocs;
  }

  void createOrSetNewSlab() {
    if (!totalslots || ((totalallocs * 100) / totalslots > load)) {
      // Create new slab
      void* mem = PageManager::getPages(1 << PageShiftAmount);
      slab_metadata& slab = slabmetadata[mem];
      slab.data = mem;
      slab.free_bitmask = BMAlloc.allocate(numIntsPerSlabMeta());
      std::fill(slab.free_bitmask, slab.free_bitmask + numIntsPerSlabMeta(), 0);
      CurAllocSlab = &slab;
      totalslots += numObjsPerSlab();
      for (unsigned x = 0; x < (1 << PageShiftAmount); ++x)
        mappedPages[(void*) &(((char*)mem)[x * PageManager::pageSize])] = mem;
    } else {
      // Find a slab with some free space
      for (typename MapTy::iterator ii = slabmetadata.begin(),
             ee = slabmetadata.end(); ii != ee; ++ii)
        if (findFree(&ii->second)) {
          CurAllocSlab = &ii->second;
          break;
        }
    }
  }

  public:
  BitMaskSlabManager(unsigned Osize, unsigned Alignment) 
  :objsize(Osize), CurAllocSlab(0), totalslots(0), totalallocs(0)
  {}
  ~BitMaskSlabManager() {
    for (typename MapTy::const_iterator ii = slabmetadata.begin(),
           ee = slabmetadata.end(); ii != ee; ++ii) {
      PageManager::freePages(ii->second.data, 1 << PageShiftAmount);
      BMAlloc.deallocate(ii->second.free_bitmask, numIntsPerSlabMeta());
    }
  }
    
  void* slab_alloc(unsigned num) {
    if (num > 1) {
      assert(0 && "Only size 1 allowed");
      abort();
    }
    if (!CurAllocSlab)
      createOrSetNewSlab();
    unsigned loc = findFree(CurAllocSlab);
    if (loc == ~0) {
      CurAllocSlab = 0;
      return slab_alloc(num);
    }
    setUsed(CurAllocSlab, loc);
    return &((char*)CurAllocSlab->data)[loc * objsize];
  }
  void slab_free(void* obj) {
    slab_metadata* slab = getSlabForObj(obj);
    if (!slab) {
      assert(0 && "Freeing invalid object");
      abort();
    }
    setFree(slab, getObjLoc(slab, obj));
  }
  bool slab_valid(void* obj) {
    slab_metadata* slab = getSlabForObj(obj);
    if (!slab) return false;
    return !isFree(slab, getObjLoc(slab, obj));
  }
  bool slab_managed(void* obj) {
    slab_metadata* slab = getSlabForObj(obj);
    return slab != 0;
  }
  bool slab_getbounds(void* obj, void*& start, void*& end) {
    slab_metadata* slab = getSlabForObj(obj);
    if (!slab) return false;
    unsigned loc = getObjLoc(slab, obj);
    if (isFree(slab, loc)) return false;
    start = &(char*)(slab->data)[loc * objsize];
    end = &(char*)(slab->data)[loc * objsize] - 1;
    return true;
  }
};

template<class FixedAllocator, class VarAllocator>
class CompoundSlabManager {
  FixedAllocator FixedAlloc;
  VarAllocator   VarAlloc;
  unsigned objsize;
 public:
  CompoundSlabManager(unsigned Osize, unsigned Alignment) 
    :FixedAlloc(Osize, Alignment), VarAlloc(1, Alignment), objsize(Osize)
  {}
  void* slab_alloc(unsigned num) {
    if (num == 1)
      return FixedAlloc.slab_alloc(1);
    else
      return VarAlloc.slab_alloc(num * sizeof(objsize * num));
  }
  void slab_free(void* obj) {
    if (FixedAlloc.slab_managed(obj))
      FixedAlloc.slab_free(obj);
    else
      VarAlloc.slab_free(obj);
  }
  bool slab_valid(void* obj) {
    return FixedAlloc.slab_valid(obj) || VarAlloc.slab_valid(obj);
  }
  bool slab_managed(void* obj) {
    return FixedAlloc.slab_managed(obj) || VarAlloc.slab_managed(obj);
  }
  bool slab_getbounds(void* obj, void*& start, void*& end) {
    if (FixedAlloc.slab_getbounds(obj, start, end))
      return true;
    if (VarAlloc.slab_getbounds(obj, start, end))
      return true;
    return false;
  }
};

class LinuxMmap {
 public:
  enum d {pageSize = 4096};
  static void* getPages(unsigned num) {
    return mmap(0, pageSize * num, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }
  static void freePages(void* page, unsigned num) {
    munmap(page, num * pageSize);
  }
};

