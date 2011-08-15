LEVEL = ../..
LIBRARYNAME=poolalloc_fl_rt

#
# Build shared libraries on all platforms except Cygwin and MingW (which do
# not support them).
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
#SHARED_LIBRARY=1
#LOADABLE_MODULE := 1
endif
endif

include $(LEVEL)/Makefile.common
