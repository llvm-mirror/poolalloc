##===- poolalloc/TEST.vtl.Makefile -------------------------*- Makefile -*-===##
#
# Makefile for getting performance metrics using Intel's VTune.
#
##===----------------------------------------------------------------------===##

TESTNAME = $*
CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/llvm-test; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

PERFEX := /home/vadve/criswell/local/Linux/bin/perfex

PERFOUT := /home/vadve/criswell/perf.out
PERFSCRIPT := $(BUILD_SRC_DIR)/perf.awk

#
# Events for the AMD K7 (Athlon) processors
#
K7_REFILL_SYSTEM   := 0x00411F43
K7_REFILL_L2       := 0x00411F42
#K7_CACHE_MISSES   := 0x00410041
#K7_CACHE_ACCESSES := 0x00410040
K7_TLB_MISSES      := 0x00410046
K7_MISALIGNED_DATA := 0x00410047

K7_EVENTS := -e $(K7_REFILL_SYSTEM) -e $(K7_REFILL_L2) \
             -e $(K7_MISALIGNED_DATA) -e $(K7_TLB_MISSES)

#
# Events for the Pentium 4/Xeon processors
#
P4_L1_READ_MISS := -e 0x0003B000/0x12000204@0x8000000C --p4pe=0x01000001 --p4pmv=0x1
P4_L2_READ_MISS := -e 0x0003B000/0x12000204@0x8000000D --p4pe=0x01000002 --p4pmv=0x1

P4_EVENTS := $(P4_L1_READ_MISS) $(P4_L2_READ_MISS)

EVENTS := $(K7_EVENTS)

############################################################################
# Once the results are generated, create files containing each individiual
# piece of performance information.
############################################################################

# AMD K7 (Athlon) Events
ifeq ($(EVENTS),$(K7_EVENTS))
$(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.%): \
Output/$(TEST).cacheaccesses.%: Output/test.$(TEST).%
	$(VERB) grep $(K7_CACHE_ACCESSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cacheaccesses.pa.%): \
Output/$(TEST).cacheaccesses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_CACHE_ACCESSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).tlbmisses.%): \
Output/$(TEST).tlbmisses.%: Output/test.$(TEST).%
	$(VERB) grep $(K7_TLB_MISSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).tlbmisses.pa.%): \
Output/$(TEST).tlbmisses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_TLB_MISSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.%): \
Output/$(TEST).cachemisses.%: Output/test.$(TEST).%
	$(VERB) grep $(K7_CACHE_MISSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).cachemisses.pa.%): \
Output/$(TEST).cachemisses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_CACHE_MISSES) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).misaligned.%): \
Output/$(TEST).misaligned.%: Output/test.$(TEST).%
	$(VERB) grep $(K7_MISALIGNED_DATA) $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).misaligned.pa.%): \
Output/$(TEST).misaligned.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep $(K7_MISALIGNED_DATA) $< | awk '{print $$(NF)}' > $@

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

# Pentium 4/Xeon Events
ifeq ($(EVENTS),$(P4_EVENTS))
$(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.%): \
Output/$(TEST).L1Misses.%: Output/test.$(TEST).%
	$(VERB) grep "$(P4_L1_READ_MISSES)" $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.pa.%): \
Output/$(TEST).L1Misses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep "$(P4_L1_READ_MISSES)" $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.%): \
Output/$(TEST).L2Misses.%: Output/test.$(TEST).%
	$(VERB) grep "$(P4_L2_READ_MISSES)" $< | awk '{print $$(NF)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.pa.%): \
Output/$(TEST).L2Misses.pa.%: Output/test.$(TEST).pa.%
	$(VERB) grep "$(P4_L2_READ_MISSES)" $< | awk '{print $$(NF)}' > $@
endif

############################################################################
# Rules for running the tests
############################################################################

ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

#
# Generate events for Pool Allocated CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).pa.%): \
Output/test.$(TEST).pa.%: Output/%.poolalloc.cbe Output/test.$(TEST).%
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	-$(PERFEX) -o $@ $(EVENTS) $< $(RUN_OPTIONS) > /dev/null 2>&1 < $(STDIN_FILENAME)

#
# Generate events for CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).%): \
Output/test.$(TEST).%: Output/%.nonpa.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	-$(PERFEX) -o $@ $(EVENTS) $< $(RUN_OPTIONS) > /dev/null 2>&1 < $(STDIN_FILENAME)

else

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).pa.%): \
Output/test.$(TEST).pa.%: Output/%.poolalloc.cbe  Output/test.$(TEST).%
	-$(SPEC_SANDBOX) poolalloccbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(PERFEX) -o $(BUILD_OBJ_DIR)/$@ $(EVENTS) $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).%): \
Output/test.$(TEST).%: Output/%.nonpa.cbe
	-$(SPEC_SANDBOX) nonpacbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(PERFEX) -o $(BUILD_OBJ_DIR)/$@ $(EVENTS) $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)
endif

############################################################################
# Report Targets
############################################################################
ifeq ($(EVENTS),$(K7_EVENTS))
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: $(PROGRAMS_TO_TEST:%=Output/$(TEST).tlbmisses.%)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).tlbmisses.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).misaligned.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).misaligned.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.pa.%) \
	             $(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-cbe.time) \
                     $(PROGRAMS_TO_TEST:%=Output/%.nonpa.out-cbe.time)
	@echo "Program:" $* > $@
	@echo "-------------------------------------------------------------" >> $@
	@printf "CBE-PA-TLB-Misses: %lld\n" `cat Output/$(TEST).tlbmisses.pa.$*` >> $@
	@printf "CBE-TLB-Misses: %lld\n" `cat Output/$(TEST).tlbmisses.$*` >> $@
	@printf "CBE-PA-Misaligned: %lld\n" `cat Output/$(TEST).misaligned.pa.$*` >> $@
	@printf "CBE-Misaligned: %lld\n" `cat Output/$(TEST).misaligned.$*` >> $@
	@printf "CBE-PA-L1-Cache-Misses: %lld\n" `cat Output/$(TEST).L1Misses.pa.$*` >> $@
	@printf "CBE-L1-Cache-Misses: %lld\n" `cat Output/$(TEST).L1Misses.$*` >> $@
	@printf "CBE-PA-L2-Cache-Misses: %lld\n" `cat Output/$(TEST).L2Misses.pa.$*` >> $@
	@printf "CBE-L2-Cache-Misses: %lld\n" `cat Output/$(TEST).L2Misses.$*` >> $@
	@printf "CBE-RUN-TIME: " >> $@
	@grep "^program" Output/$*.nonpa.out-cbe.time >> $@
	@printf "CBE-PA-RUN-TIME: " >> $@
	@grep "^program" Output/$*.poolalloc.out-cbe.time >> $@
endif

ifeq ($(EVENTS),$(P4_EVENTS))
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L1Misses.pa.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.%) \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2Misses.pa.%)
	@echo "Program:" $* > $@
	@echo "-------------------------------------------------------------" >> $@
	@printf "CBE-PA-L1-Cache-Misses: %lld\n" `cat Output/$(TEST).L1Misses.pa.$*` >> $@
	@printf "CBE-L1-Cache-Misses: %lld\n" `cat Output/$(TEST).L1Misses.$*` >> $@
	@printf "CBE-PA-L2-Cache-Misses: %lld\n" `cat Output/$(TEST).L2Misses.pa.$*` >> $@
	@printf "CBE-L2-Cache-Misses: %lld\n" `cat Output/$(TEST).L2Misses.$*` >> $@

endif

$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-cbe.time) \
                       $(PROGRAMS_TO_TEST:%=Output/%.nonpa.out-cbe.time)
