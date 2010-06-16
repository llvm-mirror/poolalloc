##===- poolalloc/test/Makefile -----------------------------*- Makefile -*-===##
#
# Define the default test rule, which allows people to just type 'make test' in
# this directory.
#
##===----------------------------------------------------------------------===##

# The default target in this directory is the test:: target
test::
#Eventually we'll want to automatically run the dejagnu tests too
#test:: unit

LEVEL = ..
include $(LEVEL)/Makefile.common

ifndef NO_STABLE_NUMBERS
STABLERUN := GET_STABLE_NUMBERS=1
else
STABLERUN :=
endif

ifndef NO_LARGE_SIZE
LARGESIZE := LARGE_PROBLEM_SIZE=1
else
LARGESIZE :=
endif


LARGE_PROBLEM_SIZE_DIRS := \
   #MultiSource/Benchmarks/llubenchmark \
   #MultiSource/Benchmarks/FreeBench \
   #MultiSource/Benchmarks/Ptrdist \
   #MultiSource/Benchmarks/MallocBench/cfrac \
   #SingleSource/Benchmarks/McGill


LARGE_PROBLEM_SIZE_DIRS := \
   $(addprefix $(LLVM_OBJ_ROOT)/projects/test-suite/,$(LARGE_PROBLEM_SIZE_DIRS))


NORMAL_PROBLEM_SIZE_DIRS := \
   MultiSource/Benchmarks/Olden/bh \
   #MultiSource/Benchmarks/Olden \
   #External/SPEC/CINT2000/175.vpr \
   #External/SPEC/CINT2000 \
   #External/SPEC/CINT2000/181.mcf \
   #External/SPEC/CINT2000/186.crafty \
   #External/FPGrowth \
   #External/Namd \
   #External/Povray \
   #MultiSource/Benchmarks/SciMark2-C/scimark2 \
   #External/SPEC/CINT2000/164.gzip \
   #External/SPEC/CINT2000/175.vpr \
   #External/SPEC/CINT2000/176.gcc \
   #External/SPEC/CINT2000/252.eon \
   #External/SPEC/CINT2000/253.perlbmk \
   #External/SPEC/CINT2000/254.gap \
   #External/SPEC/CINT2000/300.twolf


NORMAL_PROBLEM_SIZE_DIRS := \
   $(addprefix $(LLVM_OBJ_ROOT)/projects/test-suite/,$(NORMAL_PROBLEM_SIZE_DIRS))

#
# Problems for Pointer Compression
#
PCLARGE_PROBLEM_SIZE_DIRS := \
   MultiSource/Benchmarks/Olden/bh \
   MultiSource/Benchmarks/Olden/bisort \
   MultiSource/Benchmarks/Olden/em3d \
   MultiSource/Benchmarks/Olden/power \
   MultiSource/Benchmarks/llubenchmark \
   MultiSource/Benchmarks/Ptrdist \
   MultiSource/Benchmarks/MallocBench/cfrac

PCLARGE_PROBLEM_SIZE_DIRS := \
   $(addprefix $(LLVM_OBJ_ROOT)/projects/test-suite/,$(PCLARGE_PROBLEM_SIZE_DIRS))

PCNORMAL_PROBLEM_SIZE_DIRS := \
   MultiSource/Benchmarks/Olden/perimeter \
   MultiSource/Benchmarks/Olden/treeadd \
   MultiSource/Benchmarks/Olden/tsp \
   MultiSource/Benchmarks/Olden/voronoi

PCNORMAL_PROBLEM_SIZE_DIRS := \
   $(addprefix $(LLVM_OBJ_ROOT)/projects/test-suite/,$(PCNORMAL_PROBLEM_SIZE_DIRS))

##############################################################################
# Targets for gathering statistics for programs for papers
##############################################################################
# Program tests for DSA
progdsa::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=dsgraph \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=dsgraph \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=dsgraph \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=dsgraph \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

# Program tests for the allocator
progalloc::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=FL2 \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=FL2 \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=FL2 \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=FL2 \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

# Program tests for Pool Allocation
progtest::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=poolalloc \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=poolalloc \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

# Program tests for Pointer Compression
progpc::
	for dir in $(PCLARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=ptrcomp \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(PCNORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=ptrcomp \
                   report.html) \
        done
	@for dir in $(PCLARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=ptrcomp \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(PCNORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=ptrcomp \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

progperf::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   LARGE_PROBLEM_SIZE=1 report.html report.csv) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   report.html report.csv) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=perf \
                   LARGE_PROBLEM_SIZE=1 report report.csv) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=perf \
                   report report.csv) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

progp4perf::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=p4perf \
                   LARGE_PROBLEM_SIZE=1 report.html report.csv) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=p4perf \
                   report.html report.csv) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=p4perf \
                   LARGE_PROBLEM_SIZE=1 report report.csv) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=p4perf \
                   report report.csv) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

progcputrack::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=cputrack \
                   LARGE_PROBLEM_SIZE=1 report.html report.csv) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=cputrack \
                   report.html report.csv) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=cputrack \
                   LARGE_PROBLEM_SIZE=1 report report.csv) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=cputrack \
                   report report.csv) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"


##############################################################################
# Targets for running tests and gathering statistics for arbitrary tests
##############################################################################

# test target - Descend into projects/test-suite and run the TEST.poolalloc.Makefile
# tests...
test::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=poolalloc \
                   $(LARGESIZE) $(STABLERUN) report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

pacompiletime::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=pacompiletime \
                   report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

ptrcomp::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=ptrcomp \
                   $(LARGESIZE) $(STABLERUN) report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

ptrcomptest::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=ptrcomp \
                   $(LARGESIZE) report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

vtl::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=pavtl \
                   test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

perf::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

optzn::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=optzn \
                   $(LARGESIZE) $(STABLERUN) test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

p4perf::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=p4perf \
                   test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

strace::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=strace \
                   test)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

cputrack::
	(cd $(LLVM_OBJ_ROOT)/projects/test-suite/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=cputrack \
                   report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

##===----------------------------------------------------------------------===##
# Unit tests
##===----------------------------------------------------------------------===##

#DISCLAIMER
#I would prefer being able to do something like above, by setting
#some "PROJECT_DIR" variable, but alas I don't see that for the dejagnu
#test framework.  So for now we hijack the existing framework as much as
#possible--feel free to replace this if you know a cleaner way.

.PHONY: check_runtest unit

#Path to dejagnu's runtest binary.
RUNTEST=runtest

#Figure out what our configuration is
#Better way?
ifeq ($(ENABLE_OPTIMIZED),1)
CONFIGURATION := "Release"
else
CONFIGURATION := "Debug"
endif

# Pathname to poolalloc object tree
PADIR   := $(LLVM_OBJ_ROOT)/projects/poolalloc
# Pathame to the DSA pass dynamic library
DSA_SO   := $(PADIR)/$(CONFIGURATION)/lib/libLLVMDataStructure$(SHLIBEXT)

DSAOPT := $(PROJ_OBJ_ROOT)/test/tools/dsaopt

TOOLS= $(DSAOPT)

#Check that our RUNTEST variable points to a runtest we can use
check_runtest:
	@which runtest; \
	if [ $$? -ne 0 ]; then \
		echo \"$(RUNTEST)\" is not valid. Please put runtest on your PATH; \
		echo or set the RUNTEST variable correctly.; \
		exit 1; \
	fi

#Grab site.exp per normal llvm testing, but modify it to run in this project
site.exp:
	@echo "Generating site.exp..."
	@make -C $(LLVM_OBJ_ROOT)/test site.exp
	@cp $(LLVM_OBJ_ROOT)/test/site.exp     site.tmp
	@echo "set srcdir $(PROJ_SRC_DIR)"  >> site.tmp
	@echo "set srcroot $(PROJ_SRC_DIR)" >> site.tmp
	@echo "set objdir $(PROJ_OBJ_DIR)"  >> site.tmp
	@echo "set objroot $(PROJ_OBJ_DIR)" >> site.tmp
	@cp site.tmp $@

#wrapper script for 'opt' so we can avoid manually loading the dsa lib
$(DSAOPT):
	@mkdir -p `dirname $@`
	@echo "#!/bin/sh" >> $@.tmp
	@echo 'opt -load $(DSA_SO) $$@' >> $@.tmp
	@chmod +x $@.tmp
	@mv $@.tmp $@

#We need this locally, might be able to avoid this by modifying site.exp
llvm.exp:
	cp $(LLVM_SRC_ROOT)/test/lib/llvm.exp $@

#Build up the runtest command by making sure useful tools are on its path
RUNTEST_CMD = \
	PATH=$(LLVMToolDir):$(LLVM_SRC_ROOT)/test/Scripts:$(LLVMGCCDIR)/bin:$(PATH):$(PROJ_OBJ_ROOT)/test/tools \
	$(RUNTEST)

#Run the dejagnu tests for this project
unit: check_runtest site.exp llvm.exp $(TOOLS)
	$(RUNTEST_CMD)

clean::
	-$(RM) -rf  `find $(PROJ_OBJ_DIR) -name Output -type d -print`
	-$(RM) $(PROJ_OBJ_ROOT)/test/site.exp $(PROJ_OBJ_ROOT)/test/llvm.exp
	-$(RM) -rf tools
