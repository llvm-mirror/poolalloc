#include "PoolAllocator.h"
#include <iostream>

PoolAllocator<MallocSlabManager<> > a(10, 16);
PoolAllocator<BitMaskSlabManager<LinuxMmap> > b(8, 8);

PoolAllocator<CompoundSlabManager<BitMaskSlabManager<LinuxMmap>, MallocSlabManager<> > > c(8, 8);

RangeSplayMap<unsigned> ma;

int main() {
  void* x = a.alloc();
  std::cerr << a.isAllocated(x) << " " << a.isAllocated((char*)x + 5) << " " << a.isAllocated((char*)x + 10) << "\n";
  a.dealloc(x);

  x = b.alloc();
  std::cerr << b.isAllocated(x) << " " << b.isAllocated((char*)x + 5) << " " << b.isAllocated((char*)x + 10) << "\n";
  b.dealloc(x);

  x = c.alloc();
  std::cerr << c.isAllocated(x) << " " << c.isAllocated((char*)x + 5) << " " << c.isAllocated((char*)x + 10) << "\n";
  void* y = c.alloc(4);
  std::cerr << c.isAllocated(y) << " " << c.isAllocated((char*)y + 5) << " " << c.isAllocated((char*)y + 10) << " " << c.isAllocated((char*)y + 50) << "\n";
  c.dealloc(x);
  c.dealloc(y);
  
  unsigned asdf = 2;
  ma.insert((void*)0, (void*)1, asdf);

  return 0;
}

  
