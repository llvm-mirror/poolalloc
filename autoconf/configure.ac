dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([[[Pool Allocation]]],[[[1.0]]],[llvmbugs@cs.uiuc.edu])

dnl Identify where LLVM source tree is
LLVM_SRC_ROOT="`(cd $srcdir/../..; pwd)`"
LLVM_OBJ_ROOT="`(cd ../..; pwd)`"

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
LLVM_CONFIG_PROJECT($LLVM_SRC_ROOT,$LLVM_OBJ_ROOT)

dnl Tell autoconf that the auxilliary files are actually located in
dnl the LLVM autoconf directory, not here.
AC_CONFIG_AUX_DIR($LLVM_SRC/autoconf)

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.common.in"])

dnl Configure a common Makefile
AC_CONFIG_FILES(Makefile.common)

dnl Configure project makefiles
dnl List every Makefile that exists within your source tree
AC_CONFIG_MAKEFILE(Makefile)
AC_CONFIG_MAKEFILE(lib/Makefile)
AC_CONFIG_MAKEFILE(lib/PoolAllocate/Makefile)
AC_CONFIG_MAKEFILE(runtime/Makefile)
AC_CONFIG_MAKEFILE(runtime/PoolAllocator/Makefile)
AC_CONFIG_MAKEFILE(runtime/FreeListAllocator/Makefile)
AC_CONFIG_MAKEFILE(test/Makefile)

dnl **************************************************************************
dnl * Determine which system we are building on
dnl **************************************************************************

dnl **************************************************************************
dnl * Check for programs.
dnl **************************************************************************

dnl **************************************************************************
dnl * Check for libraries.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for header files.
dnl **************************************************************************
AC_HEADER_MMAP_ANONYMOUS
AC_HEADER_STAT
AC_CHECK_HEADERS([fcntl.h sys/mman.h])

dnl **************************************************************************
dnl * Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for library functions.
dnl **************************************************************************
AC_FUNC_MMAP
AC_FUNC_MMAP_FILE
AC_NEED_DEV_ZERO_FOR_MMAP

dnl **************************************************************************
dnl * Enable various compile-time options
dnl **************************************************************************

dnl --enable-llva-kernel: Compile DSA for pool inference
AC_ARG_ENABLE(kernel,
  AS_HELP_STRING(--enable-kernel,
                 [Use SAFECode for use with Linux Kernel (default is NO)]),
                  AC_DEFINE(LLVA_KERNEL,[[1]]),
                  )

dnl **************************************************************************
dnl * Set the location of various third-party software packages
dnl **************************************************************************

dnl **************************************************************************
dnl * Set the location of various programs
dnl **************************************************************************
AC_PATH_PROG(LATEX, [latex])
AC_PATH_PROG(BIBTEX, [bibtex])

dnl **************************************************************************
dnl * Create the output files
dnl **************************************************************************
AC_CONFIG_HEADERS(include/poolalloc/Config/config.h)

dnl This must be last
AC_OUTPUT
