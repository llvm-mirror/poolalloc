##===- poolalloc/test/TEST.optzn.Makefile ------------------*- Makefile -*-===##
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
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/llvm-test; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

# Pool allocator pass shared object
PA_SO    := $(PROJECT_DIR)/$(CONFIGURATION)/lib/poolalloc$(SHLIBEXT)

# Pool allocator runtime library
#PA_RT    := $(PROJECT_DIR)/lib/Bytecode/libpoolalloc_fl_rt.bc
#PA_RT_O  := $(PROJECT_DIR)/lib/$(CONFIGURATION)/poolalloc_rt.o
PA_RT_O  := $(PROJECT_DIR)/Release/lib/poolalloc_rt.o
#PA_RT_O  := $(PROJECT_DIR)/lib/Release/poolalloc_fl_rt.o

# Command to run opt with the pool allocator pass loaded
OPT_PA := $(LOPT) -load $(PA_SO)

# OPT_PA_STATS - Run opt with the -stats and -time-passes options, capturing the
# output to a file.
OPT_PA_STATS = $(OPT_PA) -info-output-file=$(CURDIR)/$@.info -stats -time-passes

OPTZN_PASSES := -globaldce -ipsccp -deadargelim -adce -instcombine -simplifycfg


# This rule runs the pool allocator on the .llvm.bc file to produce a new .bc
# file
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).basepa.bc): \
Output/%.$(TEST).basepa.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-disable-alignopt  -poolalloc-heuristic=AllNodes $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).selectivepa.bc): \
Output/%.$(TEST).selectivepa.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-heuristic=AllNodes $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) -pooloptimize $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).bumpptr.bc): \
Output/%.$(TEST).bumpptr.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).align.bc): \
Output/%.$(TEST).align.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-disable-alignopt $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) -pooloptimize $< -o $@ -f 2>&1 > $@.out


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).freeelim.bc): \
Output/%.$(TEST).freeelim.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-force-all-poolfrees $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) -pooloptimize $< -o $@ -f 2>&1 > $@.out


# This rule compiles the new .bc file into a .c file using CBE
$(PROGRAMS_TO_TEST:%=Output/%.basepa.cbe.c): \
Output/%.basepa.cbe.c: Output/%.$(TEST).basepa.bc $(LLC)
	-$(LLC) -march=c $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.selectivepa.cbe.c): \
Output/%.selectivepa.cbe.c: Output/%.$(TEST).selectivepa.bc $(LLC)
	-$(LLC) -march=c $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.bumpptr.cbe.c): \
Output/%.bumpptr.cbe.c: Output/%.$(TEST).bumpptr.bc $(LLC)
	-$(LLC) -march=c $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.align.cbe.c): \
Output/%.align.cbe.c: Output/%.$(TEST).align.bc $(LLC)
	-$(LLC) -march=c $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.freeelim.cbe.c): \
Output/%.freeelim.cbe.c: Output/%.$(TEST).freeelim.bc $(LLC)
	-$(LLC) -march=c $< -o $@



$(PROGRAMS_TO_TEST:%=Output/%.basepa.cbe): \
Output/%.basepa.cbe: Output/%.basepa.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.selectivepa.cbe): \
Output/%.selectivepa.cbe: Output/%.selectivepa.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.bumpptr.cbe): \
Output/%.bumpptr.cbe: Output/%.bumpptr.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.align.cbe): \
Output/%.align.cbe: Output/%.align.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.freeelim.cbe): \
Output/%.freeelim.cbe: Output/%.freeelim.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@



ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

# This rule runs the generated executable, generating timing information, for
# normal test programs
$(PROGRAMS_TO_TEST:%=Output/%.basepa.out-cbe): \
Output/%.basepa.out-cbe: Output/%.basepa.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.selectivepa.out-cbe): \
Output/%.selectivepa.out-cbe: Output/%.selectivepa.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.bumpptr.out-cbe): \
Output/%.bumpptr.out-cbe: Output/%.bumpptr.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.align.out-cbe): \
Output/%.align.out-cbe: Output/%.align.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.freeelim.out-cbe): \
Output/%.freeelim.out-cbe: Output/%.freeelim.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

else

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/%.basepa.out-cbe): \
Output/%.basepa.out-cbe: Output/%.basepa.cbe
	-$(SPEC_SANDBOX) basepacbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/basepacbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/basepacbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.selectivepa.out-cbe): \
Output/%.selectivepa.out-cbe: Output/%.selectivepa.cbe
	-$(SPEC_SANDBOX) selectivepacbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/selectivepacbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/selectivepacbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.bumpptr.out-cbe): \
Output/%.bumpptr.out-cbe: Output/%.bumpptr.cbe
	-$(SPEC_SANDBOX) bumpptrcbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/bumpptrcbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/bumpptrcbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.align.out-cbe): \
Output/%.align.out-cbe: Output/%.align.cbe
	-$(SPEC_SANDBOX) aligncbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/aligncbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/aligncbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.freeelim.out-cbe): \
Output/%.freeelim.out-cbe: Output/%.freeelim.cbe
	-$(SPEC_SANDBOX) freeelimcbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/freeelimcbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/freeelimcbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

endif


# This rule diffs the post-poolallocated version to make sure we didn't break
# the program!
$(PROGRAMS_TO_TEST:%=Output/%.basepa.diff-cbe): \
Output/%.basepa.diff-cbe: Output/%.out-nat Output/%.basepa.out-cbe
	@cp Output/$*.out-nat Output/$*.basepa.out-nat
	-$(DIFFPROG) cbe $*.basepa $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.selectivepa.diff-cbe): \
Output/%.selectivepa.diff-cbe: Output/%.out-nat Output/%.selectivepa.out-cbe
	@cp Output/$*.out-nat Output/$*.selectivepa.out-nat
	-$(DIFFPROG) cbe $*.selectivepa $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.bumpptr.diff-cbe): \
Output/%.bumpptr.diff-cbe: Output/%.out-nat Output/%.bumpptr.out-cbe
	@cp Output/$*.out-nat Output/$*.bumpptr.out-nat
	-$(DIFFPROG) cbe $*.bumpptr $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.align.diff-cbe): \
Output/%.align.diff-cbe: Output/%.out-nat Output/%.align.out-cbe
	@cp Output/$*.out-nat Output/$*.align.out-nat
	-$(DIFFPROG) cbe $*.align $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.freeelim.diff-cbe): \
Output/%.freeelim.diff-cbe: Output/%.out-nat Output/%.freeelim.out-cbe
	@cp Output/$*.out-nat Output/$*.freeelim.out-nat
	-$(DIFFPROG) cbe $*.freeelim $(HIDEDIFF)



# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
			     Output/%.basepa.diff-cbe     \
			     Output/%.selectivepa.diff-cbe     \
			     Output/%.bumpptr.diff-cbe      \
			     Output/%.align.diff-cbe    \
			     Output/%.freeelim.diff-cbe  \
                             Output/%.LOC.txt
	@echo > $@
	@-if test -f Output/$*.basepa.diff-cbe; then \
	  printf "CBE-RUN-TIME-BASEPA: " >> $@;\
	  grep "^program" Output/$*.basepa.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.align.diff-cbe; then \
	  printf "CBE-RUN-TIME-ALIGN: " >> $@;\
	  grep "^program" Output/$*.align.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.freeelim.diff-cbe; then \
	  printf "CBE-RUN-TIME-FREEELIM: " >> $@;\
	  grep "^program" Output/$*.freeelim.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.bumpptr.diff-cbe; then \
	  printf "CBE-RUN-TIME-BUMPPTR: " >> $@;\
	  grep "^program" Output/$*.bumpptr.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.selectivepa.diff-cbe; then \
	  printf "CBE-RUN-TIME-SELECTIVEPA: " >> $@;\
	  grep "^program" Output/$*.selectivepa.out-cbe.time >> $@;\
	fi


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
