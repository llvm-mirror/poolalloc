##===- poolalloc/test/TEST.ptrcomp.Makefile ----------------*- Makefile -*-===##
#
# This test runs the pool allocator and pointer compressor on all of the
# programs, producing some performance numbers and statistics.
#
##===----------------------------------------------------------------------===##

CFLAGS = -O2 -fno-strict-aliasing

EXTRA_PA_FLAGS := 

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
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).poolalloc.bc): \
Output/%.$(TEST).poolalloc.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc $(EXTRA_PA_FLAGS) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).ptrcomp64.bc): \
Output/%.$(TEST).ptrcomp64.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -pointercompress $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

# This rule compiles the new .bc file into a .c file using CBE
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.cbe.c): \
Output/%.poolalloc.cbe.c: Output/%.$(TEST).poolalloc.bc $(LLC)
	-$(LLC) -march=c $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.ptrcomp64.cbe.c): \
Output/%.ptrcomp64.cbe.c: Output/%.$(TEST).ptrcomp64.bc $(LLC)
	-$(LLC) -march=c $< -o $@



$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.cbe): \
Output/%.poolalloc.cbe: Output/%.poolalloc.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.ptrcomp64.cbe): \
Output/%.ptrcomp64.cbe: Output/%.ptrcomp64.cbe.c $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@


ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

# This rule runs the generated executable, generating timing information, for
# normal test programs
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-cbe): \
Output/%.poolalloc.out-cbe: Output/%.poolalloc.cbe
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.ptrcomp64.out-cbe): \
Output/%.ptrcomp64.out-cbe: Output/%.ptrcomp64.cbe
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

$(PROGRAMS_TO_TEST:%=Output/%.ptrcomp64.out-cbe): \
Output/%.ptrcomp64.out-cbe: Output/%.ptrcomp64.cbe
	-$(SPEC_SANDBOX) ptrcomp64cbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/ptrcomp64cbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/ptrcomp64cbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

endif


# This rule diffs the post-poolallocated version to make sure we didn't break
# the program!
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.diff-cbe): \
Output/%.poolalloc.diff-cbe: Output/%.out-nat Output/%.poolalloc.out-cbe
	@cp Output/$*.out-nat Output/$*.poolalloc.out-nat
	-$(DIFFPROG) cbe $*.poolalloc $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.ptrcomp64.diff-cbe): \
Output/%.ptrcomp64.diff-cbe: Output/%.out-nat Output/%.ptrcomp64.out-cbe
	@cp Output/$*.out-nat Output/$*.ptrcomp64.out-nat
	-$(DIFFPROG) cbe $*.ptrcomp64 $(HIDEDIFF)


# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
			     Output/%.poolalloc.diff-cbe     \
			     Output/%.ptrcomp64.diff-cbe      \
                             Output/%.LOC.txt
	@echo > $@
	@-if test -f Output/$*.ptrcomp64.diff-cbe; then \
	  printf "CBE-RUN-TIME-PTRCOMP64: " >> $@;\
	  grep "^program" Output/$*.ptrcomp64.out-cbe.time >> $@;\
        fi
	@-if test -f Output/$*.poolalloc.diff-cbe; then \
	  printf "CBE-RUN-TIME-POOLALLOC: " >> $@;\
	  grep "^program" Output/$*.poolalloc.out-cbe.time >> $@;\
	fi
	printf "LOC: " >> $@
	cat Output/$*.LOC.txt >> $@
	@cat Output/$*.$(TEST).ptrcomp64.bc.info >> $@
	@#cat Output/$*.$(TEST).ptrcomp64.bc.out  >> $@


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
