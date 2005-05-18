##===- poolalloc/TEST.cputrack.Makefile --------------------*- Makefile -*-===##
#
# Makefile for measuring cache miss rates with the Solaris cputrack utility
#
##===----------------------------------------------------------------------===##

TESTNAME = $*
CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/llvm-test; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

#
# The Solaris cputrack command samples the performance counters every x number
# of ticks (by default, I believe one tick == 1 second).
#
# Use the cputrack command and set the interval to the maximum amount of time
# that the program is allowed to run.
#
# Care must be taken to ensure that the counters do not wrap around.  So
# far, I don't believe this has happened, as that would require over
# 4 billion events per execution.
#
CPUTRACK       := cputrack -T $(RUNTIMELIMIT)

#
# Events for the Ultrasparc IIIi processor.
#
# EVENTS_DC_RD:
#   Read accesses and misses on the L1 Data Cache.
#
# EVENTS_DC_WR:
#   Write accesses and misses on the L1 Data Cache.  This probably isn't as
#   valuable as it sounds because the cache is write through, non allocate.
#   So, as I understand it, the L2 and Write Caches are where the write action
#   is.
#
# EVENTS_EC_MISS:
#   Read misses and overall misses for the L2 Cache (aka the Embedded Cache).
#
EVENTS_DC_RD   := -c pic0=DC_rd,pic1=DC_rd_miss
EVENTS_DC_WR   := -c pic0=DC_wr,pic1=DC_wr_miss
EVENTS_EC_MISS := -c pic0=EC_rd_miss,pic1=EC_misses
EVENTS_TLB     := -c pic0=EC_rd_miss,pic1=DTLB_miss
EVENTS_WC      := -c pic0=Cycle_cnt,pic1=WC_miss

############################################################################
# Rules for running the tests
############################################################################

ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

#
# Generate events for Pool Allocated CBE
#
$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.dcrd): \
Output/$(TEST).pa.%.dcrd: Output/%.poolalloc.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_DC_RD)   -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1


$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.ec): \
Output/$(TEST).pa.%.ec: Output/%.poolalloc.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_EC_MISS) -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1

$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.wc): \
Output/$(TEST).pa.%.wc: Output/%.poolalloc.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_WC)      -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1

$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.tlb): \
Output/$(TEST).pa.%.tlb: Output/%.poolalloc.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_TLB)      -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1

#
# Generate events for CBE
#
$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.dcrd): \
Output/$(TEST).%.dcrd: Output/%.nonpa.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_DC_RD)   -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1

$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.ec): \
Output/$(TEST).%.ec: Output/%.nonpa.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_EC_MISS) -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1

$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.wc): \
Output/$(TEST).%.wc: Output/%.nonpa.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_WC)      -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1

$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.tlb): \
Output/$(TEST).%.tlb: Output/%.nonpa.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
	$(VERB) $(CPUTRACK) $(EVENTS_TLB)      -o $@ $< $(RUN_OPTIONS) < $(STDIN_FILENAME) > /dev/null 2>&1

else

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.dcrd): \
Output/$(TEST).pa.%.dcrd: Output/%.poolalloc.cbe
	-$(SPEC_SANDBOX) poolalloccbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_DC_RD)   -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.ec): \
Output/$(TEST).pa.%.ec: Output/%.poolalloc.cbe
	-$(SPEC_SANDBOX) poolalloccbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_EC_MISS) -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.wc): \
Output/$(TEST).pa.%.wc: Output/%.poolalloc.cbe
	-$(SPEC_SANDBOX) poolalloccbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_WC)      -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.tlb): \
Output/$(TEST).pa.%.tlb: Output/%.poolalloc.cbe
	-$(SPEC_SANDBOX) poolalloccbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_TLB)      -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.dcrd): \
Output/$(TEST).%.dcrd: Output/%.nonpa.cbe
	-$(SPEC_SANDBOX) nonpacbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_DC_RD)   -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.ec): \
Output/$(TEST).%.ec: Output/%.nonpa.cbe
	-$(SPEC_SANDBOX) nonpacbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_EC_MISS) -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.wc): \
Output/$(TEST).%.wc: Output/%.nonpa.cbe
	-$(SPEC_SANDBOX) nonpacbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_WC)      -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/$(TEST).%.tlb): \
Output/$(TEST).%.tlb: Output/%.nonpa.cbe
	-$(SPEC_SANDBOX) nonpacbe-$(RUN_TYPE) $@.out $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  $(CPUTRACK) $(EVENTS_TLB)      -o $(BUILD_OBJ_DIR)/$@ $(BUILD_OBJ_DIR)/$< $(RUN_OPTIONS)
endif

############################################################################
# Report Targets
############################################################################
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).%.dcrd)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.dcrd)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).%.wc)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.wc)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).%.ec)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.ec)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).%.tlb)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).pa.%.tlb)     \
	                   $(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-cbe.time) \
                     $(PROGRAMS_TO_TEST:%=Output/%.nonpa.out-cbe.time)
	@echo "Program:" $* > $@
	@echo "-------------------------------------------------------------" >> $@
	@printf "CBE-PA-L1-Data-Reads: " >> $@
	@cat  Output/$(TEST).pa.$*.dcrd | tail -1 | awk '{print $$4}' >> $@
	@printf "CBE-PA-L1-Data-Misses: " >> $@
	@cat  Output/$(TEST).pa.$*.dcrd | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-L1-Data-Reads: " >> $@
	@cat  Output/$(TEST).$*.dcrd | tail -1 | awk '{print $$4}' >> $@
	@printf "CBE-L1-Data-Misses: " >> $@
	@cat  Output/$(TEST).$*.dcrd | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-PA-WCache-Misses: " >> $@
	@cat  Output/$(TEST).pa.$*.wc | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-WCache-Misses: " >> $@
	@cat  Output/$(TEST).$*.wc | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-PA-L2-Data-Read-Misses: " >> $@
	@cat  Output/$(TEST).pa.$*.ec | tail -1 | awk '{print $$4}' >> $@
	@printf "CBE-PA-L2-Data-Misses: " >> $@
	@cat  Output/$(TEST).pa.$*.ec | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-L2-Data-Read-Misses: " >> $@
	@cat  Output/$(TEST).$*.ec | tail -1 | awk '{print $$4}' >> $@
	@printf "CBE-L2-Data-Misses: " >> $@
	@cat  Output/$(TEST).$*.ec | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-PA-DTLB-Misses: " >> $@
	@cat  Output/$(TEST).pa.$*.tlb | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-DTLB-Misses: " >> $@
	@cat  Output/$(TEST).$*.tlb | tail -1 | awk '{print $$5}' >> $@
	@printf "CBE-RUN-TIME: " >> $@
	@grep "^program" Output/$*.nonpa.out-cbe.time >> $@
	@printf "CBE-PA-RUN-TIME: " >> $@
	@grep "^program" Output/$*.poolalloc.out-cbe.time >> $@

$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PROGRAMS_TO_TEST:%=Output/%.poolalloc.out-cbe.time) \
                       $(PROGRAMS_TO_TEST:%=Output/%.nonpa.out-cbe.time)
