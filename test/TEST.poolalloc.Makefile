##===- poolalloc/test/TEST.poolalloc.Makefile --------------*- Makefile -*-===##
#
# This test runs the pool allocator on all of the Programs, producing some
# performance numbers and statistics.
#
##===----------------------------------------------------------------------===##

CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LEVEL)/test/Programs; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

# Pool allocator shared object and runtime library...
PA_SO  := $(PROJECT_DIR)/lib/Debug/libpoolalloc.so
PA_RT  := $(PROJECT_DIR)/lib/Bytecode/libpoolalloc.bc

# Command to run opt with the pool allocator pass loaded
OPT_PA := $(LOPT) -load $(PA_SO)

# OPT_PA_STATS - Run opt with the -stats and -time-passes options, capturing the
# output to a file.
OPT_PA_STATS = $(OPT_PA) -info-output-file=$(CURDIR)/$@.info -stats -time-passes

# This rule runs the pool allocator on the .llvm.bc file to produce a new .bc
# file
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).transformed.bc): \
Output/%.$(TEST).transformed.bc: Output/%.llvm.bc $(PA_SO)
	-$(OPT_PA_STATS) -q -poolalloc $< -o $@ -f 2>&1 > $@.out

# This rule compiles the new .bc file into a .s file using LLC
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.llc.s): \
Output/%.poolalloc.llc.s: Output/%.llvm.bc $(LLC)
	-$(LLC) $(LLCFLAGS) -f $< -o $@

# This rule compiles the .s file into an executable using $CC
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.llc): \
Output/%.poolalloc.llc: Output/%.poolalloc.llc.s
	-$(CC) $(CFLAGS) $< $(LLCLIBS) $(LDFLAGS) -o $@

# This rule runs the generated executable, generating timing information
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-llc): \
Output/%.poolalloc.out-llc: Output/%.poolalloc.llc
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

# This rule diffs the post-poolallocated version to make sure we didn't break
# the program!
$(PROGRAMS_TO_TEST:%=Output/%.poolalloc.diff-llc): \
Output/%.poolalloc.diff-llc: Output/%.out-nat Output/%.poolalloc.out-llc
	@cp Output/$*.out-nat Output/$*.poolalloc.out-nat
	-$(DIFFPROG) llc $*.poolalloc $(HIDEDIFF)



# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.$(TEST).transformed.bc Output/%.exe-llc \
			     Output/%.poolalloc.diff-llc
	@echo > $@
	@-if test -f Output/$*.poolalloc.diff-llc; then \
	  printf "LLC-RUN-TIME-NORMAL: " >> $@;\
	  grep "^real" Output/$*.out-llc.time >> $@;\
	  printf "LLC-RUN-TIME-POOLALLOC: " >> $@;\
	  grep "^real" Output/$*.poolalloc.out-llc.time >> $@;\
	fi

	@cat Output/$*.$(TEST).transformed.bc.info >> $@
	@#cat Output/$*.$(TEST).transformed.bc.out  >> $@


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<
