LEVEL = ../..
LIBRARYNAME=count

#
# Build shared libraries on all platforms except Cygwin and MingW (which do
# not support them).
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY=1
endif
endif

ifeq ($(OS),Linux)
CXX.Flags += -march=native
else
CXX.Flags += -march=nocona
endif

CXX.Flags += -fno-threadsafe-statics

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
