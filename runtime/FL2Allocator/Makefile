LEVEL = ../..
LIBRARYNAME=poolalloc_rt

#
# Build shared libraries on all platforms except Cygwin and MingW (which do
# not support them).
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY=1
endif
endif

ifdef ENABLE_OPTIMIZED
CXXFLAGS += -DNDEBUG=1
endif

CXXFLAGS += -fno-exceptions

#
# Do not build bitcode library on Mac OS X; XCode will pre-install llvm-gcc,
# and that can cause the build to fail if it doesn't match the current version
# of LLVM.
#
ifneq ($(OS),Darwin)
#BYTECODE_LIBRARY=1
endif

include $(LEVEL)/Makefile.common


