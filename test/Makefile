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
	(cd $(LLVM_SRC_ROOT)/test/Programs/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) LARGE_PROBLEM_SIZE=1 TEST=poolalloc \
                   report report.html )
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"
