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
#include <unistd.h>
#include "poolalloc/MMAPSupport.h"
#include "poolalloc/Support/MallocAllocator.h"
#include <vector>
#include <iostream>

// Define this if we want to use memalign instead of mmap to get pages.
// Empirically, this slows down the pool allocator a LOT.
#define USE_MEMALIGN 0

unsigned PageSize = 0;

void InitializePageManager() {
  if (!PageSize) PageSize = sysconf(_SC_PAGESIZE);
}

// Explicitly use the malloc allocator here, to avoid depending on the C++
// runtime library.
typedef std::vector<void*, llvm::MallocAllocator<void*> > FreePagesListType;

static FreePagesListType &getFreePageList() {
  static FreePagesListType *FreePages = 0;

  if (!FreePages) {
    // Avoid using operator new!
    FreePages = (FreePagesListType*)malloc(sizeof(FreePagesListType));
    // Use placement new now.
    new (FreePages) std::vector<void*, llvm::MallocAllocator<void*> >();
  }
  return *FreePages;
}

/// AllocatePage - This function returns a chunk of memory with size and
/// alignment specified by PageSize.
void *AllocatePage() {
#if USE_MEMALIGN
  void *Addr;
  posix_memalign(&Addr, PageSize, PageSize);
  return Addr;
#else

  FreePagesListType &FPL = getFreePageList();

  if (!FPL.empty()) {
    void *Result = FPL.back();
    FPL.pop_back();
    return Result;
  }

  // Allocate several pages, and put the extras on the freelist...
  unsigned NumToAllocate = 8;
  char *Ptr = (char*)AllocateSpaceWithMMAP(NumToAllocate*PageSize);
  for (unsigned i = 1; i != NumToAllocate; ++i)
    FPL.push_back(Ptr+i*PageSize);
  return Ptr;
#endif
}

void *AllocateNPages(unsigned Num) {
  if (Num <= 1) return AllocatePage();
  return AllocateSpaceWithMMAP(Num*PageSize);
}

/// FreePage - This function returns the specified page to the pagemanager for
/// future allocation.
void FreePage(void *Page) {
#if USE_MEMALIGN
  free(Page);
#else
  FreePagesListType &FPL = getFreePageList();
  FPL.push_back(Page);
#endif
}
