#include "poolalloc_runtime/Support/SplayTree.h"
#include "llvm/ADT/hash_map.h"
#include <cstdlib>

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
    
  // Allocate an object of size objsize
  void* alloc() {
    return slab_alloc(1);
  }
    
  // Allocate an array with num objects of size objsize
  void* alloc_array(unsigned num) {
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

 protected:
  MallocSlabManager(unsigned Osize, unsigned Alignment) : objsize(Osize) {}

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
  bool slab_contains(void* obj) {
    return objs.find(obj);
  }
  bool slab_managed(void* obj) {
    return objs.find(obj);
  }
  bool slab_getbounds(void* obj, void*& start, void*& end) {
    return objs.find(obj, start, end);
  }
};

template<class PageManager, unsigned PageShiftAmount>
class BitMaskSlabManager {
  hash_map<void*, std::pair<unsigned, unsigned*> >slabmetadata;
  
  unsigned objsize;

};
