#include "PoolAllocator.h"
#include <iostream>

PoolAllocator<MallocSlabManager<> > a(10, 16);
PoolAllocator<BitMaskSlabManager<LinuxMmap> > b(8, 8);

RangeSplayMap<unsigned> x;

int main() {
  void* x = a.alloc();
  std::cerr << a.isAllocated(x) << " " << a.isAllocated((char*)x + 5) << " " << a.isAllocated((char*)x + 10) << "\n";
  a.dealloc(x);

  x = b.alloc();
  std::cerr << b.isAllocated(x) << " " << b.isAllocated((char*)x + 5) << " " << b.isAllocated((char*)x + 10) << "\n";
  b.dealloc(x);
  return 0;
}

  
