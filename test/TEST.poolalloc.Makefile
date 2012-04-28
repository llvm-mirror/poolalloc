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
#PADIR   := /home/andrewl/Research/llvm/projects/poolalloc
PADIR   := $(LLVM_OBJ_ROOT)/projects/poolalloc

# Watchdog utility
WATCHDOG := $(LLVM_OBJ_ROOT)/projects/poolalloc/$(CONFIGURATION)/bin/watchdog

# Bits of runtime to improve analysis
PA_PRE_RT := $(PADIR)/$(CONFIGURATION)/lib/libpa_pre_rt.bca

# Pool allocator pass shared object
PA_SO    := $(PADIR)/$(CONFIGURATION)/lib/poolalloc$(SHLIBEXT)
DSA_SO   := $(PADIR)/$(CONFIGURATION)/lib/LLVMDataStructure$(SHLIBEXT)
ASSIST_SO := $(PADIR)/$(CONFIGURATION)/lib/AssistDS$(SHLIBEXT)

# Pool allocator runtime library
#PA_RT    := $(PADIR)/$(CONFIGURATION)/lib/poolalloc_fl_rt.bc
#PA_RT_O  := $(PROJECT_DIR)/lib/$(CONFIGURATION)/poolalloc_rt.o
PA_RT_O  := $(PADIR)/$(CONFIGURATION)/lib/libpoolalloc_rt.a
#PA_RT_O  := $(PROJECT_DIR)/lib/$(CONFIGURATION)/poolalloc_fl_rt.o

# Command to run opt with the pool allocator pass loaded
OPT_PA := $(RUNTOOLSAFELY) $(WATCHDOG) $(LOPT) -load $(DSA_SO) -load $(PA_SO)

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
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.bc): \
Output/%.poolalloc.bc: Output/%.base.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -paheur-AllButUnreachableFromMemory -poolalloc $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) -pooloptimize $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.basepa.bc): \
Output/%.basepa.bc: Output/%.base.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -paheur-AllNodes -poolalloc -poolalloc-disable-alignopt -poolalloc-force-all-poolfrees $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out


$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.bc): \
Output/%.mallocrepl.bc: Output/%.base.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -paheur-AllInOneGlobalPool -poolalloc $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.bc): \
Output/%.onlyoverhead.bc: Output/%.base.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -paheur-OnlyOverhead -poolalloc $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.bc): \
Output/%.nonpa.bc: Output/%.base.bc $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(LOPT) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

# This rule compiles the new .bc file into a .s file
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.s): \
Output/%.poolalloc.s: Output/%.poolalloc.bc $(LLC)
	-$(LLC) $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.basepa.s): \
Output/%.basepa.s: Output/%.basepa.bc $(LLC)
	-$(LLC) $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.s): \
Output/%.mallocrepl.s: Output/%.mallocrepl.bc $(LLC)
	-$(LLC) $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.s): \
Output/%.onlyoverhead.s: Output/%.onlyoverhead.bc $(LLC)
	-$(LLC) $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.s): \
Output/%.nonpa.s: Output/%.nonpa.bc $(LLC)
	-$(LLC) $< -o $@



$(PROGRAMS_TO_TEST:%=Output/%.poolalloc): \
Output/%.poolalloc: Output/%.poolalloc.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.basepa): \
Output/%.basepa: Output/%.basepa.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl): \
Output/%.mallocrepl: Output/%.mallocrepl.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead): \
Output/%.onlyoverhead: Output/%.onlyoverhead.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.nonpa): \
Output/%.nonpa: Output/%.nonpa.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@



ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

# This rule runs the generated executable, generating timing information, for
# normal test programs
$(PROGRAMS_TO_TEST:%=Output/%.out-poolalloc): \
Output/%.out-poolalloc: Output/%.poolalloc
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.out-basepa): \
Output/%.out-basepa: Output/%.basepa
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.out-mallocrepl): \
Output/%.out-mallocrepl: Output/%.mallocrepl
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.out-onlyoverhead): \
Output/%.out-onlyoverhead.out: Output/%.onlyoverhead
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.out-nonpa): \
Output/%.out-nonpa: Output/%.nonpa
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
else

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/%.out-poolalloc): \
Output/%.out-poolalloc: Output/%.poolalloc
	-$(SPEC_SANDBOX) poolalloc-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/poolalloc-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/poolalloc-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.out-basepa): \
Output/%.out-basepa: Output/%.basepa
	-$(SPEC_SANDBOX) basepa-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/basepa-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/basepa-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.out-mallocrepl): \
Output/%.out-mallocrepl: Output/%.mallocrepl
	-$(SPEC_SANDBOX) mallocrepl-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/mallocrepl-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/mallocrepl-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.out-onlyoverhead): \
Output/%.out-onlyoverhead: Output/%.onlyoverhead
	-$(SPEC_SANDBOX) onlyoverhead-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/onlyoverhead-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/onlyoverhead-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.out-nonpa): \
Output/%.out-nonpa: Output/%.nonpa
	-$(SPEC_SANDBOX) nonpa-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/nonpa-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/nonpa-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

endif


# This rule diffs the post-poolallocated version to make sure we didn't break
# the program!
$(PROGRAMS_TO_TEST:%=Output/%.diff-poolalloc): \
Output/%.diff-poolalloc: Output/%.out-nat Output/%.out-poolalloc
	-$(DIFFPROG) poolalloc $* $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.diff-basepa): \
Output/%.diff-basepa: Output/%.out-nat Output/%.out-basepa
	-$(DIFFPROG) basepa $* $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.diff-mallocrepl): \
Output/%.diff-mallocrepl: Output/%.out-nat Output/%.out-mallocrepl
	-$(DIFFPROG) mallocrepl $* $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.diff-onlyoverhead): \
Output/%.diff-onlyoverhead: Output/%.out-nat Output/%.out-onlyoverhead
	-$(DIFFPROG) onlyoverhead $* $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.diff-nonpa): \
Output/%.diff-nonpa: Output/%.out-nat Output/%.out-nonpa
	-$(DIFFPROG) nonpa $* $(HIDEDIFF)


# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
                             Output/%.diff-nonpa         \
			     Output/%.diff-poolalloc  \
			     Output/%.diff-basepa      \
			     Output/%.diff-mallocrepl    \
			     Output/%.diff-onlyoverhead  \
                             Output/%.LOC.txt
	@echo > $@
	@echo "---------------------------------------------------------------" >> $@
	@echo ">>> ========= '$(RELDIR)/$*' Program" >> $@
	@echo "---------------------------------------------------------------" >> $@
	@echo >> $@
	@-if test -f Output/$*.diff-nonpa; then \
	  printf "GCC-RUN-TIME: " >> $@;\
	  grep "^program" Output/$*.out-nat.time >> $@;\
        fi
	@-if test -f Output/$*.diff-nonpa; then \
	  printf "RUN-TIME-NORMAL: " >> $@;\
	  grep "^program" Output/$*.out-nonpa.time >> $@;\
        fi
	@-if test -f Output/$*.diff-mallocrepl; then \
	  printf "RUN-TIME-MALLOCREPL: " >> $@;\
	  grep "^program" Output/$*.out-mallocrepl.time >> $@;\
        fi
	@-if test -f Output/$*.diff-onlyoverhead; then \
	  printf "RUN-TIME-ONLYOVERHEAD: " >> $@;\
	  grep "^program" Output/$*.out-onlyoverhead.time >> $@;\
        fi
	@-if test -f Output/$*.diff-basepa; then \
	  printf "RUN-TIME-BASEPA: " >> $@;\
	  grep "^program" Output/$*.out-basepa.time >> $@;\
        fi
	@-if test -f Output/$*.diff-poolalloc; then \
	  printf "RUN-TIME-POOLALLOC: " >> $@;\
	  grep "^program" Output/$*.out-poolalloc.time >> $@;\
	fi
	@-if test -f Output/$*.poolalloc.bc.info; then \
	  printf "PATIME: " >> $@;\
	  grep '  Pool allocate disjoint' Output/$*.poolalloc.bc.info >> $@;\
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
