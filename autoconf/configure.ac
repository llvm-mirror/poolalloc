dnl Autoconf requirements
dnl AC_INIT(package, version, bug-report-address)
dnl information on the package
dnl checks for programs
dnl checks for libraries
dnl checks for header files
dnl checks for types
dnl checks for structures
dnl checks for compiler characteristics
dnl checks for library functions
dnl checks for system services
dnl AC_CONFIG_FILES([file...])
dnl AC_OUTPUT

dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([[[LLVM]]],[[[1.0]]],[llvmbugs@cs.uiuc.edu])

dnl Place all of the extra autoconf files into the config subdirectory
AC_CONFIG_AUX_DIR([autoconf])

dnl Configure a header file
dnl AC_CONFIG_HEADERS(include/Config/config.h)

dnl **************************************************************************
dnl * Determine which system we are building on
dnl **************************************************************************

dnl **************************************************************************
dnl * Check for programs.
dnl **************************************************************************

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.common.in"])

dnl **************************************************************************
dnl * Check for libraries.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for header files.
dnl *	Chances are, if the standard C or POSIX type header files are missing,
dnl *	then LLVM just isn't going to compile.  However, it is possible that
dnl *	the necessary functions/macros will be included from other
dnl *	(non-standard and non-obvious) header files.
dnl *
dnl *	So, we'll be gracious, give it a chance, and try to go on without
dnl *	them.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for library functions.
dnl **************************************************************************

dnl **************************************************************************
dnl * Enable various compile-time options
dnl **************************************************************************

dnl **************************************************************************
dnl * Set the location of various third-party software packages
dnl **************************************************************************

dnl Location of LLVM source code
AC_ARG_WITH(llvmsrc,AC_HELP_STRING([--with-llvmsrc],[Location of LLVM Source Code]),AC_SUBST(LLVM_SRC,[$withval]),AC_SUBST(LLVM_SRC,[/home/vadve/${USER}/llvm]))

dnl Location of LLVM object code
AC_ARG_WITH(llvmobj,AC_HELP_STRING([--with-llvmobj],[Location of LLVM Object Code]),AC_SUBST(LLVM_OBJ,[$withval]),AC_SUBST(LLVM_OBJ,[/home/vadve/${USER}/llvm]))

dnl **************************************************************************
dnl * Create the output files
dnl **************************************************************************
AC_OUTPUT(Makefile.common)
