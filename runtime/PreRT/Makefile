LEVEL = ../..
LIBRARYNAME=pa_pre_rt

#
# Do not build bitcode library on Mac OS X; XCode will pre-install llvm-gcc,
# and that can cause the build to fail if it doesn't match the current version
# of LLVM.
#
ifneq ($(OS),Darwin)
#BYTECODE_LIBRARY=1
endif

include $(LEVEL)/Makefile.common

