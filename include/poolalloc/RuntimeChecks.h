//===- RuntimeChecks.h - Runtime check information -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// Provides a table holding information about runtime checks that take pool
// handles.
//
//===---------------------------------------------------------------------===//

#ifndef _RUNTIME_CHECKS_H
#define _RUNTIME_CHECKS_H

namespace {

typedef enum { SAFECodeCheck, CStdLibCheck, CIFCheck } CheckKindTy;

const struct RuntimeCheckEntry {
  const char *Function;
  unsigned PoolArgc;
  CheckKindTy CheckKind;
} RuntimeCheckEntries[] = {
  //
  // SAFECode runtime checks
  //
  { "pool_register",          1, SAFECodeCheck },
  { "pool_register_debug",    1, SAFECodeCheck },
  { "pool_register_stack",    1, SAFECodeCheck },
  { "pool_register_stack_debug", 1, SAFECodeCheck   },
  { "pool_register_global",   1, SAFECodeCheck },
  { "pool_register_global_debug", 1, SAFECodeCheck  },
  { "pool_reregister",        1, SAFECodeCheck },
  { "pool_reregister_debug",  1, SAFECodeCheck },
  { "pool_unregister",        1, SAFECodeCheck },
  { "pool_unregister_debug",  1, SAFECodeCheck },
  { "pool_unregister_stack",  1, SAFECodeCheck },
  { "pool_unregister_stack_debug", 1, SAFECodeCheck },
  { "poolcheck",              1, SAFECodeCheck },
  { "poolcheck_debug",        1, SAFECodeCheck },
  { "poolcheckui",            1, SAFECodeCheck },
  { "poolcheckui_debug",      1, SAFECodeCheck },
  { "poolcheckalign",         1, SAFECodeCheck },
  { "poolcheckalign_debug",   1, SAFECodeCheck },
  { "poolcheckstr",           1, SAFECodeCheck },
  { "poolcheckstr_debug",     1, SAFECodeCheck },
  { "poolcheckstrui",         1, SAFECodeCheck },
  { "poolcheckstrui_debug",   1, SAFECodeCheck },
  { "boundscheck",            1, SAFECodeCheck },
  { "boundscheck_debug",      1, SAFECodeCheck },
  { "boundscheckui",          1, SAFECodeCheck },
  { "boundscheckui_debug",    1, SAFECodeCheck },
  { "poolcheck_free",         1, SAFECodeCheck },
  { "poolcheck_free_debug",   1, SAFECodeCheck },
  { "poolcheck_freeui",       1, SAFECodeCheck },
  { "poolcheck_freeui_debug", 1, SAFECodeCheck },
  { "pchk_getActualValue",    1, SAFECodeCheck },
  { "__sc_fsparameter",       1, SAFECodeCheck },

  //
  // CStdLib runtime checks
  //
  { "pool_vprintf",           1, CStdLibCheck  },
  { "pool_vfprint",           2, CStdLibCheck  },
  { "pool_vsprintf",          2, CStdLibCheck  },
  { "pool_vsnprintf",         2, CStdLibCheck  },
  { "pool_vscanf",            1, CStdLibCheck  },
  { "pool_vsscanf",           2, CStdLibCheck  },
  { "pool_vfscanf",           2, CStdLibCheck  },
  { "pool_vsyslog",           1, CStdLibCheck  },
  { "pool_memccpy",           2, CStdLibCheck  },
  { "pool_memchr",            1, CStdLibCheck  },
  { "pool_memcmp",            2, CStdLibCheck  },
  { "pool_memcpy",            2, CStdLibCheck  },
  { "pool_memmove",           2, CStdLibCheck  },
  { "pool_memset",            1, CStdLibCheck  },
  { "pool_strcat",            2, CStdLibCheck  },
  { "pool_strchr",            1, CStdLibCheck  },
  { "pool_strcmp",            2, CStdLibCheck  },
  { "pool_strcoll",           2, CStdLibCheck  },
  { "pool_strcpy",            2, CStdLibCheck  },
  { "pool_strcspn",           2, CStdLibCheck  },
  { "pool_strlen",            1, CStdLibCheck  },
  { "pool_strncat",           2, CStdLibCheck  },
  { "pool_strncmp",           2, CStdLibCheck  },
  { "pool_strncpy",           2, CStdLibCheck  },
  { "pool_strpbrk",           2, CStdLibCheck  },
  { "pool_strrchr",           1, CStdLibCheck  },
  { "pool_strspn",            2, CStdLibCheck  },
  { "pool_strstr",            2, CStdLibCheck  },
  { "pool_strxfrm",           2, CStdLibCheck  },
  { "pool_mempcpy",           2, CStdLibCheck  },
  { "pool_strcasestr",        2, CStdLibCheck  },
  { "pool_stpcpy",            2, CStdLibCheck  },
  { "pool_strnlen",           1, CStdLibCheck  },
  { "pool_bcmp",              2, CStdLibCheck  },
  { "pool_bcopy",             2, CStdLibCheck  },
  { "pool_bzero",             1, CStdLibCheck  },
  { "pool_index",             1, CStdLibCheck  },
  { "pool_rindex",            1, CStdLibCheck  },
  { "pool_strcasecmp",        2, CStdLibCheck  },
  { "pool_strncasecmp",       2, CStdLibCheck  },
  { "pool_fgets",             1, CStdLibCheck  },
  { "pool_fputs",             1, CStdLibCheck  },
  { "pool_gets",              1, CStdLibCheck  },
  { "pool_puts",              1, CStdLibCheck  },
  { "pool_fread",             1, CStdLibCheck  },
  { "pool_fwrite",            1, CStdLibCheck  },
  { "pool_tmpnam",            1, CStdLibCheck  },
  { "pool_readdir_r",         2, CStdLibCheck  },
  { "pool_read",              1, CStdLibCheck  },
  { "pool_recv",              1, CStdLibCheck  },
  { "pool_write",             1, CStdLibCheck  },
  { "pool_send",              1, CStdLibCheck  },
  { "pool_readlink",          2, CStdLibCheck  },
  { "pool_realpath",          2, CStdLibCheck  },
  { "pool_getcwd",            1, CStdLibCheck  },

  //
  // CIFCheck intrinsics
  //
  { "__if_pool_get_label",    1, CIFCheck      },
  { "__if_pool_set_label",    1, CIFCheck      }
};

}

#endif
