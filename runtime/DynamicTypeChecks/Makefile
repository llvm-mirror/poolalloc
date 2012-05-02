LEVEL = ../..
LIBRARYNAME = typechecks_rt

#
# Don't build shared libraries on Windows.  Note that we need to specify
# both SHARED_LIBRARY and LOADABLE_MODULE on Mac OS X.
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY=1
LOADABLE_MODULE := 1
endif
endif

ifdef ENABLE_OPTIMIZED
CXXFLAGS += -DNDEBUG=1
endif

#
# Do not build bitcode library on Mac OS X; XCode will pre-install llvm-gcc,
# and that can cause the build to fail if it doesn't match the current version
# of LLVM.
#
ifneq ($(OS),Darwin)
#BYTECODE_LIBRARY=1
endif

include $(LEVEL)/Makefile.common

# Always build optimized and debug versions
all:: $(LIBNAME_OBJO) $(LIBNAME_OBJG)
