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

/// InitializePageManager - This function must be called before any other page
/// manager accesses are performed.  It may be called multiple times.
/// 
void InitializePageManager();

/// PageSize - Contains the size of the unit of memory allocated by
/// AllocatePage.  This is a value that is typically several kilobytes in size,
/// and is guaranteed to be a power of two.
///
extern unsigned PageSize;

/// AllocatePage - This function returns a chunk of memory with size and
/// alignment specified by getPageSize().
void *AllocatePage();

/// AllocateNPages - 
void *AllocateNPages(unsigned Num);

/// FreePage - This function returns the specified page to the pagemanager for
/// future allocation.
void FreePage(void *Page);

#endif
