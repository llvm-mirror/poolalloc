##===- poolalloc/test/Makefile -----------------------------*- Makefile -*-===##
#
# Define the default test rule, which allows people to just type 'make test' in
# this directory.
#
##===----------------------------------------------------------------------===##

# Simply run lit tests by default
test:: lit

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
   MultiSource/Benchmarks/Olden \
   External/SPEC/CINT2000/ \
   External/SPEC/CINT2006/ \
   #External/SPEC/CINT2000/164.gzip \
   #External/SPEC/CINT2000/175.vpr \
   #External/SPEC/CINT2000/176.gcc \
   #External/SPEC/CINT2000/181.mcf \
   #External/SPEC/CINT2000/186.crafty \
   #External/SPEC/CINT2000/197.parser \
   #External/SPEC/CINT2000/252.eon \
   #External/SPEC/CINT2000/253.perlbmk \
   #External/SPEC/CINT2000/254.gap \
   #External/SPEC/CINT2000/255.vortex \
   #External/SPEC/CINT2000/256.bzip2 \
   #External/SPEC/CINT2000/300.twolf \
   #MultiSource/Applications \
   #MultiSource/Applications/Burg \
   #MultiSource/Applications/hexxagon \
   #MultiSource/Applications/kimwitu++ \
   #MultiSource/Applications/lambda-0.1.3 \
   #MultiSource/Benchmarks/Olden/bh \
   #External/FPGrowth \
   #External/Namd \
   #External/Povray \
   #MultiSource/Benchmarks/SciMark2-C/scimark2 \


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

# Program tests for DSAA
progdsaa::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=dsaa \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=dsaa \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=dsaa \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=dsaa \
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

# Program tests for DSA Call Targets
progcall::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=calltargets \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=calltargets \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=calltargets \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=calltargets \
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

# Program tests for Types
progtypes::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=types \
                   LARGE_PROBLEM_SIZE=1 report.html) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=types \
                   report.html) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=types \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s -j1 TEST=types \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"


##############################################################################
# Targets for running tests and gathering statistics for arbitrary tests
##############################################################################

# test target - Descend into projects/test-suite and run the TEST.poolalloc.Makefile
# tests...
fulltest::
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
# Lit tests
##===----------------------------------------------------------------------===##

.PHONY: lit lit-report

ifeq ($(DISABLE_ASSERTIONS),1)
ENABLE_ASSERTIONS=0
else
ENABLE_ASSERTIONS=1
endif

FORCE:

lit.site.cfg: FORCE
	@echo "Making Poolalloc/DSA 'lit.site.cfg' file..."
	@$(ECHOPATH) s=@TARGET_TRIPLE@=$(TARGET_TRIPLE)=g > lit.tmp
	@$(ECHOPATH) s=@LLVM_SOURCE_DIR@=$(LLVM_SRC_ROOT)=g >> lit.tmp
	@$(ECHOPATH) s=@LLVM_BINARY_DIR@=$(LLVM_OBJ_ROOT)=g >> lit.tmp
	@$(ECHOPATH) s=@PROJ_SRC_ROOT@=$(PROJ_SRC_ROOT)=g >> lit.tmp
	@$(ECHOPATH) s=@PROJ_OBJ_ROOT@=$(PROJ_OBJ_ROOT)=g >> lit.tmp
	@$(ECHOPATH) s=@LLVM_TOOLS_DIR@=$(LLVMToolDir)=g >> lit.tmp
	@$(ECHOPATH) s=@SHLIBDIR@=$(SharedLibDir)=g >> lit.tmp
	@$(ECHOPATH) s=@SHLIBEXT@=$(SHLIBEXT)=g >> lit.tmp
	@$(ECHOPATH) s=@PYTHON_EXECUTABLE@=python=g >> lit.tmp
	@$(ECHOPATH) s,@OCAMLOPT@,$(OCAMLOPT) -cc \\\\\"$(CXX_FOR_OCAMLOPT)\\\\\" -I $(LibDir)/ocaml,g >> lit.tmp
	@$(ECHOPATH) s=@ENABLE_SHARED@=$(ENABLE_SHARED)=g >> lit.tmp
	@$(ECHOPATH) s=@ENABLE_ASSERTIONS@=$(ENABLE_ASSERTIONS)=g >> lit.tmp
	@$(ECHOPATH) s=@TARGETS_TO_BUILD@=$(TARGETS_TO_BUILD)=g >> lit.tmp
	@$(ECHOPATH) s=@LLVM_BINDINGS@=$(BINDINGS_TO_BUILD)=g >> lit.tmp
	@$(ECHOPATH) s=@HOST_OS@=$(HOST_OS)=g >> lit.tmp
	@$(ECHOPATH) s=@HOST_ARCH@=$(HOST_ARCH)=g >> lit.tmp
	@sed -f lit.tmp $(PROJ_SRC_ROOT)/test/lit.site.cfg.in > $@
	@-rm -f lit.tmp

#Run the lit tests for this project
.PHONY: lit lit-report
lit: lit.site.cfg $(TOOLS) check-asserts
	$(LLVM_SRC_ROOT)/utils/lit/lit.py $(PROJ_OBJ_ROOT)/test --path $(ToolDir) -s -v

#Run the lit tests, but supress all error output--just report the test results.
lit-report: lit.site.cfg $(TOOLS) check-asserts
	$(LLVM_SRC_ROOT)/utils/lit/lit.py $(PROJ_OBJ_ROOT)/test --path $(ToolDir) -q

check-asserts:
	$(if $(findstring Asserts,$(BuildMode)), , \
		$(error Error: Lit tests require assertions to be enabled))

clean::
	-$(RM) -rf  `find $(PROJ_OBJ_DIR) -name Output -type d -print`
	-$(RM) lit.site.cfg
	-$(RM) -rf tools
