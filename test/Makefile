##===- poolalloc/test/Makefile -----------------------------*- Makefile -*-===##
#
# Define the default test rule, which allows people to just type 'make test' in
# this directory.
#
##===----------------------------------------------------------------------===##

# The default target in this directory is the test:: target
test::

LEVEL = ..
include $(LEVEL)/Makefile.common

# test target - Descend into test/Programs and run the TEST.poolalloc.Makefile
# tests...
test::
	(cd $(LLVM_SRC_ROOT)/test/Programs; PROJECT_DIR=$(BUILD_OBJ_ROOT) \
                $(MAKE) TEST=poolalloc)
