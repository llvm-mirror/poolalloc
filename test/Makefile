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


##############################################################################
# Targets for gathering statistics for programs for papers
##############################################################################
progtest::
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Olden; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report.html)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Ptrdist; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report.html)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/FreeBench/analyzer; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report.html)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/MallocBench/cfrac; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report.html)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/External/SPEC/CINT2000; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report.html)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Olden; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Ptrdist; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/FreeBench/analyzer; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/MallocBench/cfrac; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/External/SPEC/CINT2000; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

progperf::
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Olden; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   LARGE_PROBLEM_SIZE=1 report.html)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Ptrdist; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report.html)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/MallocBench/cfrac; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report.html)
	(cd $(LLVM_OBJ_ROOT)/test/Programs/External/SPEC/CINT2000; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report.html)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Olden; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   LARGE_PROBLEM_SIZE=1 report)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/Ptrdist; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/MultiSource/Benchmarks/MallocBench/cfrac; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report)
	@(cd $(LLVM_OBJ_ROOT)/test/Programs/External/SPEC/CINT2000; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

##############################################################################
# Targets for running tests and gathering statistics for arbitrary tests
##############################################################################

# test target - Descend into test/Programs and run the TEST.poolalloc.Makefile
# tests...
test::
	(cd $(LLVM_OBJ_ROOT)/test/Programs/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

vtl::
	(cd $(LLVM_OBJ_ROOT)/test/Programs/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=pavtl \
                   test report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

perf::
	(cd $(LLVM_OBJ_ROOT)/test/Programs/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   test report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

strace::
	(cd $(LLVM_OBJ_ROOT)/test/Programs/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=strace \
                   test)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"
