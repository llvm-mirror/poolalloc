//===- PageManager.cpp - Implementation of the page allocator -------------===//
// 
//                       The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file implements the PageManager.h interface.
//
//===----------------------------------------------------------------------===//

#include "PageManager.h"
#ifndef _POSIX_MAPPED_FILES
#define _POSIX_MAPPED_FILES
#endif
#include "Support/MallocAllocator.h"
#include "Config/unistd.h"
#include "Config/sys/mman.h"
#include <cassert>
#include <vector>

unsigned PageSize = 0;

// Explicitly use the malloc allocator here, to avoid depending on the C++
// runtime library.
static std::vector<void*, MallocAllocator<void*> > *FreePages = 0;

void InitializePageManager() {
  if (!PageSize) PageSize = sysconf(_SC_PAGESIZE);
}

static void *GetPages(unsigned NumPages) {
#if defined(i386) || defined(__i386__) || defined(__x86__)
  /* Linux and *BSD tend to have these flags named differently. */
#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
# define MAP_ANONYMOUS MAP_ANON
#endif /* defined(MAP_ANON) && !defined(MAP_ANONYMOUS) */
#elif defined(sparc) || defined(__sparc__) || defined(__sparcv9)
  /* nothing */
#else
  std::cerr << "This architecture is not supported by the pool allocator!\n";
  abort();
#endif

#if defined(__linux__)
#define fd 0
#else
#define fd -1
#endif

  void *pa = mmap(0, NumPages*PageSize, PROT_READ|PROT_WRITE|PROT_EXEC,
                  MAP_PRIVATE|MAP_ANONYMOUS, fd, 0);
  assert(pa != MAP_FAILED && "MMAP FAILED!");
  return pa;
}


/// AllocatePage - This function returns a chunk of memory with size and
/// alignment specified by PageSize.
void *AllocatePage() {
  if (FreePages && !FreePages->empty()) {
    void *Result = FreePages->back();
    FreePages->pop_back();
    return Result;
  }

  // Allocate several pages, and put the extras on the freelist...
  unsigned NumToAllocate = 10;
  char *Ptr = (char*)GetPages(NumToAllocate);

  if (!FreePages) FreePages = new std::vector<void*, MallocAllocator<void*> >();
  for (unsigned i = 1; i != NumToAllocate; ++i)
    FreePages->push_back(Ptr+i*PageSize);
  return Ptr;
}


/// FreePage - This function returns the specified page to the pagemanager for
/// future allocation.
void FreePage(void *Page) {
  assert(FreePages && "No pages allocated!");
  FreePages->push_back(Page);
  //munmap(Page, 1);
}
