#include "PoolAllocator.h"
#include <iostream>

PoolAllocator<MallocSlabManager<> > a(10, 16);

RangeSplayMap<unsigned> x;

int main() {
  void* x = a.alloc();
  std::cerr << a.isAllocated(x) << " " << a.isAllocated((char*)x + 5) << " " << a.isAllocated((char*)x + 10) << "\n";
  a.dealloc(x);
  return 0;
}

  
