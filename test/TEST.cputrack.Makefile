##===- poolalloc/TEST.cputrack.Makefile --------------------*- Makefile -*-===##
#
# Makefile for measuring cache miss rates with the Solaris cputrack utility
#
##===----------------------------------------------------------------------===##

TESTNAME = $*
CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_OBJ_ROOT)/projects/llvm-test; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

CPUTRACK := cputrack -T 500 -c pic0=EC_rd_miss,pic1=DC_rd_miss

#
# Once the results are generated, create files containing each individiual
# piece of performance information.
#

#
# Generate events for Pool Allocated CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).pa.%): \
Output/test.$(TEST).pa.%: Output/%.poolalloc.cbe Output/test.$(TEST).%
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(CPUTRACK) -o $@ $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(CPUTRACK) -o $@ $< $(RUN_OPTIONS)
endif

#
# Generate events for CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).%): \
Output/test.$(TEST).%: Output/%.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(CPUTRACK) -o $@ $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(CPUTRACK) -o $@ $< $(RUN_OPTIONS)
endif

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/test.$(TEST).pa.% Output/test.$(TEST).%
	touch $@

$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

