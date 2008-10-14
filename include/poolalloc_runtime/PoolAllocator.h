#include "poolalloc_runtime/Support/SplayTree.h"
#include <cstdlib>

template<class PageManager, template <typename T> class SlabManager >
class PoolAllocator : SlabManager<PageManager> {
  using SlabManager<PageManager>::slab_alloc;
  using SlabManager<PageManager>::slab_free;
  using SlabManager<PageManager>::slab_valid;
  using SlabManager<PageManager>::slab_getbounds;
 public:
  PoolAllocator(unsigned objsize, unsigned Alignment)
    : SlabManager<PageManager>(objsize, Alignment)
    {}
    
    
    //In-place new operator
    static void* operator new( std::size_t s, void* p ) throw() {
      return p;
    }
    
    //Allocate an object of size objsize
    void* alloc() {
      return slab_alloc(1);
    }
    
    //Allocate an array with num objects of size objsize
    void* alloc_array(unsigned num) {
      return slab_alloc(num);
    }
    
    //Free allocated object
  void dealloc(void* obj) {
    slab_free(obj);
  }

  //Tests if obj is in an allocated object
  bool isAllocated(void* obj) {
    return slab_valid(obj);
  }

  // Returns the start and end of an object, return value is true if found
  bool getBounds(void* obj, void*& start, void*& end) {
    return slab_getbounds(obj, start, end);
  }

};

class mmapPageManager {
  enum { pageSize = 4096 };
};

template<class PageManager>
class MallocSlabManager {
  SplayRangeSet objs;
  unsigned objsize;
 protected:
  MallocSlabManager(unsigned Osize, unsigned Alignment) : objsize(Osize) {}

  void* slab_alloc(unsigned num) {
    void* x = malloc(num*objsize);
    objs.insert(x, (char*)x + num*objsize - 1);
    return x;
  }
  void slab_free(void* obj) {
    objs.remove(obj);
    free(obj);
  }
  bool slab_valid(void* obj) {
    return objs.find(obj);
  }
  void* slab_getbounds(void* obj, void*& start, void*& end) {
    return objs.find(obj, start, end);
  } 

};
