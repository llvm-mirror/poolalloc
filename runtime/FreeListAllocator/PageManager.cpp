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

unsigned PageSize = 4096;

// Explicitly use the malloc allocator here, to avoid depending on the C++
// runtime library.
typedef std::vector<void*, llvm::MallocAllocator<void*> > FreePagesListType;
static FreePagesListType *FreePages = 0;

//
// Function: InitializePageManager ()
//
// Description:
//  This function initializes the Page Manager code.  It must be called before
//  any other Page Manager functions are called.
//
unsigned int InitializePageManager() {
  if (!PageSize) {
    PageSize = sysconf(_SC_PAGESIZE);
    FreePages = 0;
  }
  return PageSize;
}

///
/// Function: AllocatePage ()
///
/// Description:
/// This function returns a chunk of memory with size and alignment specified
/// by PageSize.
void *AllocatePage() {
#if USE_MEMALIGN
  void *Addr;
  posix_memalign(&Addr, PageSize, PageSize);
  return Addr;
#else
  //
  // Try to allocate a page that has already been created.
  //
  if (FreePages && !FreePages->empty()) {
    void *Result = FreePages->back();
    FreePages->pop_back();
    return Result;
  }

  // Allocate several pages, and put the extras on the freelist...
  unsigned NumToAllocate = 8;
  char *Ptr = (char*)AllocateSpaceWithMMAP(PageSize*NumToAllocate);

  if (!FreePages) {
    // Avoid using operator new!
    FreePages = (FreePagesListType*)malloc(sizeof(FreePagesListType));
    // Use placement new now.
    new (FreePages) std::vector<void*, llvm::MallocAllocator<void*> >();
  }
  for (unsigned i = 1; i != NumToAllocate; ++i)
    FreePages->push_back(Ptr+i*PageSize);
  return Ptr;
#endif
}


/// FreePage - This function returns the specified page to the pagemanager for
/// future allocation.
void FreePage(void *Page) {
#if USE_MEMALIGN
  free(Page);
#else
  assert(FreePages && "No pages allocated!");
  FreePages->push_back(Page);
#endif
}
