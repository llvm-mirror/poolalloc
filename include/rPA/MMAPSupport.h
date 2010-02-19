//===- MMAPSupport.h - mmap portability wrappers ----------------*- C++ -*-===//
// 
//                       The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
//
// This file allows clients to use some functionality of mmap efficiently and
// portably.  This is used for the pool allocator runtime implementations.
//
//===----------------------------------------------------------------------===//

#include "poolalloc/Config/config.h"
#include <cstdlib>
#include <cassert>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif /* defined(MAP_ANON) && !defined(MAP_ANONYMOUS) */

static inline void *
AllocateSpaceWithMMAP(size_t Size, bool UseNoReserve = false) {
  // NOTE: this assumes Size is a multiple of the page size.
  int FD = -1;
#ifdef NEED_DEV_ZERO_FOR_MMAP
  static int DevZeroFD = open("/dev/zero", O_RDWR);
  if (DevZeroFD == -1) {
    perror("Can't open /dev/zero device");
    abort();
  }
  FD = zero_fd;
#endif

  int Flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
  if (UseNoReserve)
    Flags |= MAP_NORESERVE;
#endif

  void *Mem = ::mmap(0, Size, PROT_READ|PROT_WRITE, Flags, FD, 0);
  assert(Mem != MAP_FAILED && "couldn't get space!");
  return Mem;
}
