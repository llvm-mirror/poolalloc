##===- poolalloc/test/TEST.poolalloc.Makefile --------------*- Makefile -*-===##
#
# This test runs the pool allocator on all of the Programs, producing some
# performance numbers and statistics.
#
##===----------------------------------------------------------------------===##

CFLAGS = -O2 -fno-strict-aliasing

EXTRA_PA_FLAGS := 

# HEURISTIC can be set to:
#   AllNodes
ifdef HEURISTIC
EXTRA_PA_FLAGS += -poolalloc-heuristic=$(HEURISTIC)
endif


CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/test-suite; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))
PADIR   := $(LLVM_OBJ_ROOT)/projects/poolalloc

# Watchdog utility
WATCHDOG := $(LLVM_OBJ_ROOT)/projects/poolalloc/$(CONFIGURATION)/bin/watchdog

# Bits of runtime to improve analysis
PA_PRE_RT := $(PADIR)/$(CONFIGURATION)/lib/libpa_pre_rt.bca

# Pool allocator pass shared object
PA_SO    := $(PADIR)/$(CONFIGURATION)/lib/libpoolalloc$(SHLIBEXT)
DSA_SO   := $(PADIR)/$(CONFIGURATION)/lib/libLLVMDataStructure$(SHLIBEXT)
ASSIST_SO := $(PADIR)/$(CONFIGURATION)/lib/libAssistDS$(SHLIBEXT)

# Pool allocator runtime library
#PA_RT    := $(PADIR)/$(CONFIGURATION)/lib/libpoolalloc_fl_rt.bc
#PA_RT_O  := $(PROJECT_DIR)/lib/$(CONFIGURATION)/poolalloc_rt.o
PA_RT_O  := $(PADIR)/$(CONFIGURATION)/lib/libpoolalloc_rt.a
#PA_RT_O  := $(PROJECT_DIR)/lib/$(CONFIGURATION)/poolalloc_fl_rt.o

# Command to run opt with the pool allocator pass loaded
OPT_PA := $(WATCHDOG) $(LOPT) -load $(DSA_SO) -load $(PA_SO)

# OPT_PA_STATS - Run opt with the -stats and -time-passes options, capturing the
# output to a file.
OPT_PA_STATS = $(OPT_PA) -info-output-file=$(CURDIR)/$@.info -stats -time-passes

OPTZN_PASSES := -globaldce -ipsccp -deadargelim -adce -instcombine -simplifycfg


$(PROGRAMS_TO_TEST:%=Output/%.temp.bc): \
Output/%.temp.bc: Output/%.llvm.bc 
	-$(LLVMLD) -link-as-library $< $(PA_PRE_RT) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.base.bc): \
Output/%.base.bc: Output/%.temp.bc $(LOPT) $(ASSIST_SO) $(DSA_SO)
	-$(LOPT) -load $(DSA_SO) -load $(ASSIST_SO) -instnamer -internalize -indclone -funcspec -ipsccp -deadargelim -instcombine -globaldce -stats $< -f -o $@ 

# This rule runs the pool allocator on the .base.bc file to produce a new .bc
# file
$(PROGRAMS_TO_TEST:%=Output/%.calltargets.data): \
Output/%.calltargets.data: Output/%.base.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -analyze -internalize -calltarget $< 2>&1 > $@

# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
                             Output/%.calltargets.data       \
                             Output/%.LOC.txt
	@-cat $<
	@echo > $@
	@echo "---------------------------------------------------------------" >> $@
	@echo ">>> ========= '$(RELDIR)/$*' Program" >> $@
	@echo "---------------------------------------------------------------" >> $@
	@echo >> $@
	@-if test -f Output/$*.nonpa.diff-nat; then \
	  printf "GCC-RUN-TIME: " >> $@;\
	  grep "^program" Output/$*.out-nat.time >> $@;\
        fi
	-printf "LOC: " >> $@
	-cat Output/$*.LOC.txt >> $@
	@-cat Output/$*.$(TEST).bc.info >> $@
	@#cat Output/$*.$(TEST).basepa.bc.out  >> $@


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@-cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
