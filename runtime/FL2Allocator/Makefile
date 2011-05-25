LEVEL = ../..
LIBRARYNAME=poolalloc_rt
BYTECODE_LIBRARY=1

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

include $(LEVEL)/Makefile.common

