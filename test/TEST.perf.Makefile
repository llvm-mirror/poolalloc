##===- test/Programs/TEST.vtl.Makefile ---------------------*- Makefile -*-===##
#
# Makefile for getting performance metrics using Intel's VTune.
#
##===----------------------------------------------------------------------===##

TESTNAME = $*
CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LEVEL)/test/Programs; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

PERFEX := /home/vadve/criswell/local/Linux/bin/perfex

#
# Events for the AMD processors
#   Data cache refills from system.
#   Data cache refills from L2.
#   Data cache misses.
#   Data cache accesses.
#
K7_REFILL_SYSTEM  := 0x00411f43
K7_REFILL_L2      := 0x00411f42
K7_CACHE_MISSES   := 0x00410041
K7_CACHE_ACCESSES := 0x00410040

K7_EVENTS := -e $(K7_REFILL_SYSTEM) -e $(K7_REFILL_L2) -e $(K7_CACHE_MISSES) -e $(K7_CACHE_ACCESSES)

EVENTS := $(K7_EVENTS)

#
# Once the results are generated, create files containing each individiual
# piece of performance information.
#

# AMD K7 (Athlon) Events
ifeq ($(EVENTS),$(K7_EVENTS))
$(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.%): \
Output/$(TEST).cacheaccesses.%: Output/test.$(TEST).%
	grep $(K7_CACHE_ACCESSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.pa.%): \
Output/$(TEST).cacheaccesses.pa.%: Output/test.$(TEST).pa.%
	grep $(K7_CACHE_ACCESSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.%): \
Output/$(TEST).cachemisses.%: Output/test.$(TEST).%
	grep $(K7_CACHE_MISSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.pa.%): \
Output/$(TEST).cachemisses.pa.%: Output/test.$(TEST).pa.%
	grep $(K7_CACHE_MISSES) $< | awk '{print $$(NF)}' > $@
endif

#
# Generate events for Pool Allocated CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).pa.%): \
Output/test.$(TEST).pa.%: Output/%.poolalloc.cbe Output/test.$(TEST).%
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $< $(RUN_OPTIONS)
endif

#
# Generate events for CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).%): \
Output/test.$(TEST).%: Output/%.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $< $(RUN_OPTIONS)
endif


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: $(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.%)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.pa.%)
	@echo > $@
	@printf "CBE-PA-Cache-Accesses: %11lld\n" `cat Output/$(TEST).cacheaccesses.pa.$*`
	@printf "CBE-Cache-Accesses   : %11lld\n" `cat Output/$(TEST).cacheaccesses.$*`
	@printf "CBE-PA-Cache-Misses  : %11lld\n" `cat Output/$(TEST).cachemisses.pa.$*`
	@printf "CBE-Cache-Misses     : %11lld\n" `cat Output/$(TEST).cachemisses.$*`


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

