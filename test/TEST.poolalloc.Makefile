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
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/llvm-test; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

# Pool allocator pass shared object
PA_SO    := $(PROJECT_DIR)/lib/Debug/libpoolalloc$(SHLIBEXT)

# Pool allocator runtime library
#PA_RT    := $(PROJECT_DIR)/lib/Bytecode/libpoolalloc_fl_rt.bc
#PA_RT_O  := $(PROJECT_DIR)/lib/$(CONFIGURATION)/poolalloc_rt.o
PA_RT_O  := $(PROJECT_DIR)/lib/Release/poolalloc_rt.o
#PA_RT_O  := $(PROJECT_DIR)/lib/Release/poolalloc_fl_rt.o

# Command to run opt with the pool allocator pass loaded
OPT_PA := $(LOPT) -load $(PA_SO)

# OPT_PA_STATS - Run opt with the -stats and -time-passes options, capturing the
# output to a file.
OPT_PA_STATS = $(OPT_PA) -info-output-file=$(CURDIR)/$@.info -stats -time-passes

OPTZN_PASSES := -globaldce -ipconstprop -deadargelim -adce -instcombine -simplifycfg


# This rule runs the pool allocator on the .llvm.bc file to produce a new .bc
# file
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).poolalloc.bc): \
Output/%.$(TEST).poolalloc.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) -pooloptimize $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).allnodes.bc): \
Output/%.$(TEST).allnodes.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-heuristic=AllNodes -pooloptimize  $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).mallocrepl.bc): \
Output/%.$(TEST).mallocrepl.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-heuristic=AllInOneGlobalPool $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).onlyoverhead.bc): \
Output/%.$(TEST).onlyoverhead.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc -poolalloc-heuristic=OnlyOverhead $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.bc): \
Output/%.nonpa.bc: Output/%.llvm.bc $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(LOPT) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

# This rule compiles the new .bc file into a .c file using CBE
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.cbe.c): \
Output/%.poolalloc.cbe.c: Output/%.$(TEST).poolalloc.bc $(LLC)
	-$(LLC) -march=c -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.allnodes.cbe.c): \
Output/%.allnodes.cbe.c: Output/%.$(TEST).allnodes.bc $(LLC)
	-$(LLC) -march=c -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.cbe.c): \
Output/%.mallocrepl.cbe.c: Output/%.$(TEST).mallocrepl.bc $(LLC)
	-$(LLC) -march=c -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.cbe.c): \
Output/%.onlyoverhead.cbe.c: Output/%.$(TEST).onlyoverhead.bc $(LLC)
	-$(LLC) -march=c -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.cbe.c): \
Output/%.nonpa.cbe.c: Output/%.nonpa.bc $(LLC)
	-$(LLC) -march=c -f $< -o $@



$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.cbe): \
Output/%.poolalloc.cbe: Output/%.poolalloc.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.allnodes.cbe): \
Output/%.allnodes.cbe: Output/%.allnodes.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.cbe): \
Output/%.mallocrepl.cbe: Output/%.mallocrepl.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.cbe): \
Output/%.onlyoverhead.cbe: Output/%.onlyoverhead.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.cbe): \
Output/%.nonpa.cbe: Output/%.nonpa.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@



ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

# This rule runs the generated executable, generating timing information, for
# normal test programs
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-cbe): \
Output/%.poolalloc.out-cbe: Output/%.poolalloc.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.allnodes.out-cbe): \
Output/%.allnodes.out-cbe: Output/%.allnodes.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.out-cbe): \
Output/%.mallocrepl.out-cbe: Output/%.mallocrepl.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.out-cbe): \
Output/%.onlyoverhead.out-cbe: Output/%.onlyoverhead.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.out-cbe): \
Output/%.nonpa.out-cbe: Output/%.nonpa.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
else

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-cbe): \
Output/%.poolalloc.out-cbe: Output/%.poolalloc.cbe
	-$(SPEC_SANDBOX) poolalloccbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/poolalloccbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/poolalloccbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.allnodes.out-cbe): \
Output/%.allnodes.out-cbe: Output/%.allnodes.cbe
	-$(SPEC_SANDBOX) allnodescbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/allnodescbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/allnodescbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.out-cbe): \
Output/%.mallocrepl.out-cbe: Output/%.mallocrepl.cbe
	-$(SPEC_SANDBOX) mallocreplcbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/mallocreplcbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/mallocreplcbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.out-cbe): \
Output/%.onlyoverhead.out-cbe: Output/%.onlyoverhead.cbe
	-$(SPEC_SANDBOX) onlyoverheadcbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/onlyoverheadcbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/onlyoverheadcbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.out-cbe): \
Output/%.nonpa.out-cbe: Output/%.nonpa.cbe
	-$(SPEC_SANDBOX) nonpacbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/nonpacbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/nonpacbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

endif


# This rule diffs the post-poolallocated version to make sure we didn't break
# the program!
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.diff-cbe): \
Output/%.poolalloc.diff-cbe: Output/%.out-nat Output/%.poolalloc.out-cbe
	@cp Output/$*.out-nat Output/$*.poolalloc.out-nat
	-$(DIFFPROG) cbe $*.poolalloc $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.allnodes.diff-cbe): \
Output/%.allnodes.diff-cbe: Output/%.out-nat Output/%.allnodes.out-cbe
	@cp Output/$*.out-nat Output/$*.allnodes.out-nat
	-$(DIFFPROG) cbe $*.allnodes $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.mallocrepl.diff-cbe): \
Output/%.mallocrepl.diff-cbe: Output/%.out-nat Output/%.mallocrepl.out-cbe
	@cp Output/$*.out-nat Output/$*.mallocrepl.out-nat
	-$(DIFFPROG) cbe $*.mallocrepl $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.onlyoverhead.diff-cbe): \
Output/%.onlyoverhead.diff-cbe: Output/%.out-nat Output/%.onlyoverhead.out-cbe
	@cp Output/$*.out-nat Output/$*.onlyoverhead.out-nat
	-$(DIFFPROG) cbe $*.onlyoverhead $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.nonpa.diff-cbe): \
Output/%.nonpa.diff-cbe: Output/%.out-nat Output/%.nonpa.out-cbe
	@cp Output/$*.out-nat Output/$*.nonpa.out-nat
	-$(DIFFPROG) cbe $*.nonpa $(HIDEDIFF)


# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.nonpa.diff-cbe         \
			     Output/%.poolalloc.diff-cbe     \
			     Output/%.allnodes.diff-cbe      \
			     Output/%.mallocrepl.diff-cbe    \
			     Output/%.onlyoverhead.diff-cbe  \
                             Output/%.LOC.txt
	@echo > $@
	@-if test -f Output/$*.nonpa.diff-cbe; then \
	  printf "CBE-RUN-TIME-NORMAL: " >> $@;\
	  grep "^program" Output/$*.nonpa.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.mallocrepl.diff-cbe; then \
	  printf "CBE-RUN-TIME-MALLOCREPL: " >> $@;\
	  grep "^program" Output/$*.mallocrepl.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.onlyoverhead.diff-cbe; then \
	  printf "CBE-RUN-TIME-ONLYOVERHEAD: " >> $@;\
	  grep "^program" Output/$*.onlyoverhead.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.allnodes.diff-cbe; then \
	  printf "CBE-RUN-TIME-ALLNODES: " >> $@;\
	  grep "^program" Output/$*.allnodes.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.poolalloc.diff-cbe; then \
	  printf "CBE-RUN-TIME-POOLALLOC: " >> $@;\
	  grep "^program" Output/$*.poolalloc.out-cbe.time >> $@;\
	fi
	printf "LOC: " >> $@
	cat Output/$*.LOC.txt >> $@
	@cat Output/$*.$(TEST).allnodes.bc.info >> $@
	@#cat Output/$*.$(TEST).allnodes.bc.out  >> $@


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
