//===- PageManager.h - Allocates memory on page boundaries ------*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file defines the interface used by the pool allocator to allocate memory
// on large alignment boundaries.
//
//===----------------------------------------------------------------------===//

#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

/// getPageSize - Return the size of the unit of memory allocated by
/// AllocatePage.  This is a value that is typically several kilobytes in size.
unsigned getPageSize();

/// AllocatePage - This function returns a chunk of memory with size and
/// alignment specified by getPageSize().
void *AllocatePage();

/// FreePage - This function returns the specified page to the pagemanager for
/// future allocation.
void FreePage(void *Page);

#endif
