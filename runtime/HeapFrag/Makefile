LEVEL = ../..
LIBRARYNAME=heapfrag

#BYTECODE_LIBRARY=1

#
# Build shared libraries on all platforms except Cygwin and MingW (which do
# not support them).
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
#SHARED_LIBRARY=1
endif
endif

include $(LEVEL)/Makefile.common

# Always build optimized and debug versions
all:: $(LIBNAME_OBJO) $(LIBNAME_OBJG)
