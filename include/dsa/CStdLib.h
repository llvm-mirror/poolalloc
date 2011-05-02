//===- CStdLib.h - CStdLib Function Information ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Tables to hold information about transformed CStdLib functions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CSTDLIB_H
#define LLVM_CSTDLIB_H

namespace llvm {

  const struct CStdLibPoolArgCountEntry {
    const char *function;
    unsigned pool_argc;
  } \
  CStdLibPoolArgCounts[] = {
    { "pool_strcpy",      2 },
    { "pool_stpcpy",      2 },
    { "pool_strncpy",     2 },
    { "pool_strlen",      1 },
    { "pool_strchr",      1 },
    { "pool_strrchr",     1 },
    { "pool_strcat",      2 },
    { "pool_strncat",     2 },
    { "pool_strstr",      2 },
    { "pool_strpbrk",     2 },
    { "pool_strcmp",      2 },
    { "pool_strcmp",      2 },
    { "pool_memcmp",      2 },
    { "pool_strcasecmp",  2 },
    { "pool_strncasecmp", 2 },
    { "pool_strspn",      2 },
    { "pool_strcspn",     2 },
    { "pool_memccpy",     2 },
    { "pool_memchr",      1 },
    { "pool_bcmp",        2 },
    { "pool_bcopy",       2 },
    { "pool_index",       1 },
    { "pool_rindex",      1 },
    {  0,                 0 }
  };

} // End llvm namespace

#endif
