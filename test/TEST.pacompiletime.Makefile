##===- poolalloc/test/TEST.pacompiletime.Makefile ----------*- Makefile -*-===##
#
# This test figures out how much time we spend in DSA and the pool allocator
# compiling a program.
#
##===----------------------------------------------------------------------===##

CFLAGS = -O2 -fno-strict-aliasing

EXTRA_PA_FLAGS := 

CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/test-suite/; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

# Pool allocator pass shared object
PA_SO    := $(PROJECT_DIR)/$(CONFIGURATION)/lib/poolalloc$(SHLIBEXT)

# Command to run opt with the pool allocator pass loaded
OPT_PA := $(RUNTOOLSAFELY) $(WATCHDOG) $(LOPT) -load $(DSA_SO) -load $(PA_SO)

# OPT_PA_STATS - Run opt with the -stats and -time-passes options, capturing the
# output to a file.
OPT_PA_STATS = $(OPT_PA) -info-output-file=$(CURDIR)/$@.info -time-passes


# This rule runs the pool allocator on the .llvm.bc file to produce a new .bc
# file
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).poolalloc.bc): \
Output/%.$(TEST).poolalloc.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) -poolalloc $(EXTRA_PA_FLAGS) $< -o $@ -f 2>&1 > $@.out


# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.$(TEST).poolalloc.bc Output/%.LOC.txt
	@echo > $@
	printf "LOC: " >> $@
	cat Output/$*.LOC.txt >> $@
	@echo >> $@
	@printf "LOCTIME: " >> $@
	@-grep "Local Data Structure" Output/$*.$(TEST).poolalloc.bc.info >>$@
	@printf "BUTIME: " >> $@
	@-grep "  Bottom-up Data Struc" Output/$*.$(TEST).poolalloc.bc.info >>$@
	@printf "TDTIME: " >> $@
	@-grep "Top-down Data Structur" Output/$*.$(TEST).poolalloc.bc.info >>$@
	@printf "COMTIME: " >> $@
	@-grep "'Complete' Bottom-up D" Output/$*.$(TEST).poolalloc.bc.info >>$@
	@printf "EQTIME: " >> $@
	@-grep "Equivalence-class Bott" Output/$*.$(TEST).poolalloc.bc.info >>$@
	@printf "PATIME: " >> $@
	@-grep "Pool allocate disjoint" Output/$*.$(TEST).poolalloc.bc.info >>$@



$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
