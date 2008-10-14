#include "PoolAllocator.h"
#include <iostream>

PoolAllocator<mmapPageManager, MallocSlabManager> a(10, 16);

int main() {
  void* x = a.alloc();
  std::cerr << a.isAllocated(x) << " " << a.isAllocated((char*)x + 5) << " " << a.isAllocated((char*)x + 10) << "\n";
  a.dealloc(x);
  return 0;
}

  
