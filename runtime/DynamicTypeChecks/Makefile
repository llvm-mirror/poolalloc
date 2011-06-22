LEVEL = ../..
LIBRARYNAME = typechecks_rt
BYTECODE_LIBRARY=1

ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY=1
endif
endif

ifdef ENABLE_OPTIMIZED
CXXFLAGS += -DNDEBUG=1
endif

include $(LEVEL)/Makefile.common

# Always build optimized and debug versions
all:: $(LIBNAME_OBJO) $(LIBNAME_OBJG)
