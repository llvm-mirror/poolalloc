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

LARGE_PROBLEM_SIZE_DIRS := \
   MultiSource/Benchmarks/Olden \
   MultiSource/Benchmarks/FreeBench/analyzer \
   MultiSource/Benchmarks/llubenchmark

LARGE_PROBLEM_SIZE_DIRS := \
   $(addprefix $(LLVM_OBJ_ROOT)/projects/llvm-test/,$(LARGE_PROBLEM_SIZE_DIRS))

NORMAL_PROBLEM_SIZE_DIRS := \
   MultiSource/Benchmarks/Ptrdist \
   External/SPEC/CINT2000
   #MultiSource/Benchmarks/MallocBench/cfrac

NORMAL_PROBLEM_SIZE_DIRS := \
   $(addprefix $(LLVM_OBJ_ROOT)/projects/llvm-test/,$(NORMAL_PROBLEM_SIZE_DIRS))


##############################################################################
# Targets for gathering statistics for programs for papers
##############################################################################
progtest::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=poolalloc \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

progperf::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=perf \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=perf \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

progp4perf::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=p4perf \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=p4perf \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=p4perf \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=p4perf \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

progcputrack::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=cputrack \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=cputrack \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=cputrack \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -s -j1 TEST=cputrack \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"


##############################################################################
# Targets for running tests and gathering statistics for arbitrary tests
##############################################################################

# test target - Descend into projects/llvm-test and run the TEST.poolalloc.Makefile
# tests...
test::
	(cd $(LLVM_OBJ_ROOT)/projects/llvm-test/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   GET_STABLE_NUMBERS=1 report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

vtl::
	(cd $(LLVM_OBJ_ROOT)/projects/llvm-test/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=pavtl \
                   test report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

perf::
	(cd $(LLVM_OBJ_ROOT)/projects/llvm-test/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   test report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

p4perf::
	(cd $(LLVM_OBJ_ROOT)/projects/llvm-test/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=p4perf \
                   test report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

strace::
	(cd $(LLVM_OBJ_ROOT)/projects/llvm-test/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=strace \
                   test)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

cputrack::
	(cd $(LLVM_OBJ_ROOT)/projects/llvm-test/$(SUBDIR); \
               PROJECT_DIR=$(BUILD_OBJ_ROOT) $(MAKE) -j1 TEST=cputrack \
                   report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

