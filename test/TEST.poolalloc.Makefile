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
PADIR   := /home/andrewl/Research/llvm/projects/poolalloc

# Pool allocator pass shared object
PA_SO    := $(PADIR)/Debug/lib/libpoolalloc$(SHLIBEXT)
DSA_SO   := $(PADIR)/Debug/lib/libLLVMDataStructure$(SHLIBEXT)

# Pool allocator runtime library
#PA_RT    := $(PADIR)/Debug/lib/libpoolalloc_fl_rt.bc
#PA_RT_O  := $(PROJECT_DIR)/lib/$(CONFIGURATION)/poolalloc_rt.o
PA_RT_O  := $(PADIR)/Release/lib/poolalloc_rt.o
#PA_RT_O  := $(PROJECT_DIR)/lib/Release/poolalloc_fl_rt.o

# Command to run opt with the pool allocator pass loaded
OPT_PA := $(LOPT) -load $(DSA_SO) -load $(PA_SO)

# OPT_PA_STATS - Run opt with the -stats and -time-passes options, capturing the
# output to a file.
OPT_PA_STATS = $(OPT_PA) -info-output-file=$(CURDIR)/$@.info -stats -time-passes

OPTZN_PASSES := -globaldce -ipsccp -deadargelim -adce -instcombine -simplifycfg


# This rule runs the pool allocator on the .llvm.bc file to produce a new .bc
# file
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.bc): \
Output/%.poolalloc.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) -pooloptimize $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.basepa.bc): \
Output/%.basepa.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-disable-alignopt -poolalloc-force-all-poolfrees -poolalloc-heuristic=AllNodes $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out


$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.bc): \
Output/%.mallocrepl.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-heuristic=AllInOneGlobalPool $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.bc): \
Output/%.onlyoverhead.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-heuristic=OnlyOverhead $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.bc): \
Output/%.nonpa.bc: Output/%.llvm.bc $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(LOPT) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

# This rule compiles the new .bc file into a .s file
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.s): \
Output/%.poolalloc.s: Output/%.poolalloc.bc $(LLC)
	-$(LLC) -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.basepa.s): \
Output/%.basepa.s: Output/%.basepa.bc $(LLC)
	-$(LLC) -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.s): \
Output/%.mallocrepl.s: Output/%.mallocrepl.bc $(LLC)
	-$(LLC) -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.s): \
Output/%.onlyoverhead.s: Output/%.onlyoverhead.bc $(LLC)
	-$(LLC) -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.s): \
Output/%.nonpa.s: Output/%.nonpa.bc $(LLC)
	-$(LLC) -f $< -o $@



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
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out): \
Output/%.poolalloc.out: Output/%.poolalloc
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.basepa.out): \
Output/%.basepa.out: Output/%.basepa
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.out): \
Output/%.mallocrepl.out: Output/%.mallocrepl
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.out): \
Output/%.onlyoverhead.out: Output/%.onlyoverhead
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

$(PROGRAMS_TO_TEST:%=Output/%.basepa.out): \
Output/%.basepa.out: Output/%.basepa
	-$(SPEC_SANDBOX) basepa-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/basepa-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/basepa-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.out): \
Output/%.mallocrepl.out: Output/%.mallocrepl
	-$(SPEC_SANDBOX) mallocrepl-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/mallocrepl-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/mallocrepl-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.out): \
Output/%.onlyoverhead.out: Output/%.onlyoverhead
	-$(SPEC_SANDBOX) onlyoverhead-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/onlyoverhead-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/onlyoverhead-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

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
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.diff): \
Output/%.poolalloc.diff: Output/%.out-nat Output/%.poolalloc.out
	@cp Output/$*.out-nat Output/$*.poolalloc.out-nat
	-$(DIFFPROG) nat $*.poolalloc $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.basepa.diff): \
Output/%.basepa.diff: Output/%.out-nat Output/%.basepa.out
	@cp Output/$*.out-nat Output/$*.basepa.out-nat
	-$(DIFFPROG) nat $*.basepa $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.diff): \
Output/%.mallocrepl.diff: Output/%.out-nat Output/%.mallocrepl.out
	@cp Output/$*.out-nat Output/$*.mallocrepl.out-nat
	-$(DIFFPROG) nat $*.mallocrepl $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.diff): \
Output/%.onlyoverhead.diff: Output/%.out-nat Output/%.onlyoverhead.out
	@cp Output/$*.out-nat Output/$*.onlyoverhead.out-nat
	-$(DIFFPROG) nat $*.onlyoverhead $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.diff): \
Output/%.nonpa.diff: Output/%.out-nat Output/%.nonpa.out
	@cp Output/$*.out-nat Output/$*.nonpa.out-nat
	-$(DIFFPROG) nat $*.nonpa $(HIDEDIFF)


# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
                             Output/%.nonpa.diff         \
			     Output/%.poolalloc.diff     \
			     Output/%.basepa.diff      \
			     Output/%.mallocrepl.diff    \
			     Output/%.onlyoverhead.diff  \
                             Output/%.LOC.txt
	@echo > $@
	@-if test -f Output/$*.nonpa.diff; then \
	  printf "GCC-RUN-TIME: " >> $@;\
	  grep "^program" Output/$*.out-nat.time >> $@;\
        fi
	@-if test -f Output/$*.nonpa.diff; then \
	  printf "RUN-TIME-NORMAL: " >> $@;\
	  grep "^program" Output/$*.nonpa.out.time >> $@;\
        fi
	@-if test -f Output/$*.mallocrepl.diff; then \
	  printf "RUN-TIME-MALLOCREPL: " >> $@;\
	  grep "^program" Output/$*.mallocrepl.out.time >> $@;\
        fi
	@-if test -f Output/$*.onlyoverhead.diff; then \
	  printf "RUN-TIME-ONLYOVERHEAD: " >> $@;\
	  grep "^program" Output/$*.onlyoverhead.out.time >> $@;\
        fi
	@-if test -f Output/$*.basepa.diff; then \
	  printf "RUN-TIME-BASEPA: " >> $@;\
	  grep "^program" Output/$*.basepa.out.time >> $@;\
        fi
	@-if test -f Output/$*.poolalloc.diff; then \
	  printf "RUN-TIME-POOLALLOC: " >> $@;\
	  grep "^program" Output/$*.poolalloc.out.time >> $@;\
	fi
	printf "LOC: " >> $@
	cat Output/$*.LOC.txt >> $@
	@cat Output/$*.$(TEST).bc.info >> $@
	@#cat Output/$*.$(TEST).basepa.bc.out  >> $@


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
