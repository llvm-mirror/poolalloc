##===- test/Programs/TEST.vtl.Makefile ---------------------*- Makefile -*-===##
#
# Makefile for getting performance metrics using Intel's VTune.
#
##===----------------------------------------------------------------------===##

TESTNAME = $*
CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LEVEL)/test/Programs; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

STRACE := strace -c -f

#
# Once the results are generated, create files containing each individiual
# piece of performance information.
#

# AMD Events
$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.%): \
Output/$(TEST).L2cachemisses.%: Output/test.$(TEST).%
	grep "Output/$*.cbe" $< | grep "Cache Read Misses" | awk '{print $$(NF-1)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.pa.%): \
Output/$(TEST).L2cachemisses.pa.%: Output/test.$(TEST).pa.%
	grep "Output/$*.poolalloc.cbe" $< | grep "Cache Read Misses" | awk '{print $$(NF-1)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.%): \
Output/$(TEST).L2cacherefs.%: Output/test.$(TEST).%
	grep "Output/$*.cbe" $< | grep "Cache Read References" | awk '{print $$(NF-1)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.pa.%): \
Output/$(TEST).L2cacherefs.pa.%: Output/test.$(TEST).pa.%
	grep "Output/$*.poolalloc.cbe" $< | grep "Cache Read References" | awk '{print $$(NF-1)}' > $@

#
# Generate events for Pool Allocated CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).pa.%): \
Output/test.$(TEST).pa.%: Output/%.poolalloc.cbe Output/test.$(TEST).%
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(STRACE) -o $@ $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(STRACE) -o $@ $< $(RUN_OPTIONS)
endif

#
# Generate events for CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).%): \
Output/test.$(TEST).%: Output/%.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(STRACE) -o $@ $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(STRACE) -o $@ $< $(RUN_OPTIONS)
endif


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.%)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.pa.%)  \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.%)       \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.pa.%)
	@echo > $@
	@echo CBE-PA-L2-Misses `cat Output/$(TEST).L2cachemisses.pa.$*`
	@echo CBE-PA-L2-Refs   `cat Output/$(TEST).L2cacherefs.pa.$*`
	@echo CBE-L2-Misses    `cat Output/$(TEST).L2cachemisses.$*`
	@echo CBE-L2-Refs      `cat Output/$(TEST).L2cacherefs.$*`


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

