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

PERFOUT := /home/vadve/criswell/perf.out
PERFSCRIPT := $(BUILD_SRC_DIR)/perf.awk

#
# Events for the AMD processors
#   Data cache refills from system.
#   Data cache refills from L2.
#   Data cache misses.
#   Data cache accesses.
#
K7_REFILL_SYSTEM  := 0x00411F43
K7_REFILL_L2      := 0x00411F42
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
	$(VERB) grep $(K7_CACHE_ACCESSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.pa.%): \
Output/$(TEST).cacheaccesses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_CACHE_ACCESSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.%): \
Output/$(TEST).cachemisses.%: Output/test.$(TEST).%
	$(VERB) grep $(K7_CACHE_MISSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.pa.%): \
Output/$(TEST).cachemisses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_CACHE_MISSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.%): \
Output/$(TEST).L1Misses.%: Output/test.$(TEST).%
	$(VERB) grep $(K7_REFILL_SYSTEM) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.pa.%): \
Output/$(TEST).L1Misses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_REFILL_SYSTEM) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.%): \
Output/$(TEST).L2Misses.%: Output/test.$(TEST).%
	$(VERB) grep $(K7_REFILL_L2) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.pa.%): \
Output/$(TEST).L2Misses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_REFILL_L2) $< | awk '{print $$(NF)}' > $@
endif

#
# Generate events for Pool Allocated CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).pa.%): \
Output/test.$(TEST).pa.%: Output/%.poolalloc.cbe Output/test.$(TEST).%
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $< > /dev/null
else
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $< $(RUN_OPTIONS) > /dev/null
endif

#
# Generate events for CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).%): \
Output/test.$(TEST).%: Output/%.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $< > /dev/null
else
	$(VERB) cat $(STDIN_FILENAME) | $(PERFEX) -o $@ $(EVENTS) $< $(RUN_OPTIONS) > /dev/null
endif

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: $(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.%)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.pa.%)
	@echo "Program:" $* > $@
	@echo "-------------------------------------------------------------" >> $@
	@printf "CBE-PA-Cache-Accesses: %lld\n" `cat Output/$(TEST).cacheaccesses.pa.$*` >> $@
	@printf "CBE-Cache-Accesses: %lld\n" `cat Output/$(TEST).cacheaccesses.$*` >> $@
	@printf "CBE-PA-Cache-Misses: %lld\n" `cat Output/$(TEST).cachemisses.pa.$*` >> $@
	@printf "CBE-Cache-Misses: %lld\n" `cat Output/$(TEST).cachemisses.$*` >> $@
	@printf "CBE-PA-L1-Cache-Misses: %lld\n" `cat Output/$(TEST).L1Misses.pa.$*` >> $@
	@printf "CBE-L1-Cache-Misses: %lld\n" `cat Output/$(TEST).L1Misses.$*` >> $@
	@printf "CBE-PA-L2-Cache-Misses: %lld\n" `cat Output/$(TEST).L2Misses.pa.$*` >> $@
	@printf "CBE-L2-Cache-Misses: %lld\n" `cat Output/$(TEST).L2Misses.$*` >> $@

$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

