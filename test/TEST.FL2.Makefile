##===- poolalloc/test/TEST.FL2.Makefile --------------------*- Makefile -*-===##
#
# This test uses simple pool allocation to test the FL2 allocator.
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
Output/%.base.bc: Output/%.temp.bc $(LOPT) $(ASSIST_SO)
	-$(LOPT) -load $(ASSIST_SO) -instnamer -internalize -indclone -funcspec -ipsccp -deadargelim -instcombine -globaldce -stats $< -f -o $@ 

# This rule runs the pool allocator on the .base.bc file to produce a new .bc
# file
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.bc): \
Output/%.poolalloc.bc: Output/%.base.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-heuristic=AllInOneGlobalPool $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.bc): \
Output/%.nonpa.bc: Output/%.base.bc $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(LOPT) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

# This rule compiles the new .bc file into a .s file
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.s): \
Output/%.poolalloc.s: Output/%.poolalloc.bc $(LLC)
	-$(LLC) $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.s): \
Output/%.nonpa.s: Output/%.nonpa.bc $(LLC)
	-$(LLC) $< -o $@

# Compile the .s file into an executable
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc): \
Output/%.poolalloc: Output/%.poolalloc.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.nonpa): \
Output/%.nonpa: Output/%.nonpa.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@


ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

# This rule runs the generated executable, generating timing information, for
# normal test programs
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out): \
Output/%.poolalloc.out: Output/%.poolalloc
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.out): \
Output/%.nonpa.out: Output/%.nonpa
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
else

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out): \
Output/%.poolalloc.out: Output/%.poolalloc
	-$(SPEC_SANDBOX) poolalloc-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/poolalloc-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/poolalloc-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.out): \
Output/%.nonpa.out: Output/%.nonpa
	-$(SPEC_SANDBOX) nonpa-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/nonpa-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/nonpa-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

endif


# This rule diffs the post-poolallocated version to make sure we didn't break
# the program!
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.diff-nat): \
Output/%.poolalloc.diff-nat: Output/%.out-nat Output/%.poolalloc.out
	@cp Output/$*.out-nat Output/$*.poolalloc.out-nat
	-$(DIFFPROG) nat $*.poolalloc $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.diff-nat): \
Output/%.nonpa.diff-nat: Output/%.out-nat Output/%.nonpa.out
	@cp Output/$*.out-nat Output/$*.nonpa.out-nat
	-$(DIFFPROG) nat $*.nonpa $(HIDEDIFF)


# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
                             Output/%.nonpa.diff-nat         \
                             Output/%.poolalloc.diff-nat     \
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
	@-if test -f Output/$*.nonpa.diff-nat; then \
	  printf "RUN-TIME-NORMAL: " >> $@;\
	  grep "^program" Output/$*.nonpa.out.time >> $@;\
        fi
	@-if test -f Output/$*.poolalloc.diff-nat; then \
	  printf "RUN-TIME-POOLALLOC: " >> $@;\
	  grep "^program" Output/$*.poolalloc.out.time >> $@;\
	fi
	-printf "LOC: " >> $@
	-cat Output/$*.LOC.txt >> $@
	@-cat Output/$*.$(TEST).bc.info >> $@

$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@-cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
