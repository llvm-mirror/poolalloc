##===- poolalloc/TEST.vtl.Makefile -------------------------*- Makefile -*-===##
#
# Makefile for getting performance metrics using Intel's VTune.
#
##===----------------------------------------------------------------------===##

TESTNAME = $*
CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/llvm-test; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))

VTL := /opt/intel/vtune/bin/vtl

#
# Events: These will need to be modified for every different CPU that is used
# (i.e. the Pentium 3 on Cypher has a different set of available events than
# the Pentium 4 on Zion).
#
P4_EVENTS := -ec
P4_EVENTS += en='2nd Level Cache Read Misses':sa=5000
P4_EVENTS += -ec
P4_EVENTS += en='2nd-Level Cache Read References'
#P4_EVENTS += -ec
#P4_EVENTS += en='3rd-Level Cache Read Misses':sa=5000
#P4_EVENTS += en='1st Level Cache Load Misses Retired'
P3_EVENTS := -ec en='L2 Cache Request Misses (highly correlated)'

EVENTS := $(P4_EVENTS)

#
# Once the results are generated, create files containing each individiual
# piece of performance information.
#

# Pentium 4 Events
ifeq ($(EVENTS),$(P4_EVENTS))
$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.%): \
Output/$(TEST).L2cachemisses.%: Output/test.$(TEST).%
	grep "Output/$*.cbe" $< | grep "Cache Read Misses" | grep "2nd" | awk '{print $$(NF-1)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.pa.%): \
Output/$(TEST).L2cachemisses.pa.%: Output/test.$(TEST).pa.%
	grep "Output/$*.poolalloc.cbe" $< | grep "Cache Read Misses" | grep "2nd" | awk '{print $$(NF-1)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.%): \
Output/$(TEST).L2cacherefs.%: Output/test.$(TEST).%
	grep "Output/$*.cbe" $< | grep "Cache Read References" | grep "2nd" | awk '{print $$(NF-1)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.pa.%): \
Output/$(TEST).L2cacherefs.pa.%: Output/test.$(TEST).pa.%
	grep "Output/$*.poolalloc.cbe" $< | grep "Cache Read References" | grep "2nd" | awk '{print $$(NF-1)}' > $@
endif

# Pentium 3 Events
ifeq ($(EVENTS),$(P3_EVENTS))
$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.%): \
Output/$(TEST).L2cachemisses.%: Output/test.$(TEST).%
	grep "Output/$*.cbe" $< | grep " L2 Cache Request Misses" | awk '{print $$(NF-1)}' > $@

$(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.pa.%): \
Output/$(TEST).L2cachemisses.pa.%: Output/test.$(TEST).pa.%
	grep "Output/$*.poolalloc.cbe" $< | grep " L2 Cache Request Misses" | awk '{print $$(NF-1)}' > $@
endif

#
# Generate events for Pool Allocated CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).pa.%): \
Output/test.$(TEST).pa.%: Output/%.poolalloc.cbe Output/test.$(TEST).%
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(VTL) activity $* -d 50 -c sampling -o "$(EVENTS)" -app $< -moi $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(VTL) activity $* -d 50 -c sampling -o "$(EVENTS)" -app $<,"$(RUN_OPTIONS)" -moi $<
endif
	$(VERB) $(VTL) run $*
	$(VERB) $(VTL) view > $@
	$(VERB)  $(VTL) delete $* -f

#
# Generate events for CBE
#
$(PROGRAMS_TO_TEST:%=Output/test.$(TEST).%): \
Output/test.$(TEST).%: Output/%.cbe
	@echo "========================================="
	@echo "Running '$(TEST)' test on '$(TESTNAME)' program"
ifeq ($(RUN_OPTIONS),)
	$(VERB) cat $(STDIN_FILENAME) | $(VTL) activity $* -d 50 -c sampling -o "$(EVENTS)" -app $< -moi $<
else
	$(VERB) cat $(STDIN_FILENAME) | $(VTL) activity $* -d 50 -c sampling -o "$(EVENTS)" -app $<,"$(RUN_OPTIONS)" -moi $<
endif
	$(VERB) $(VTL) run $*
	$(VERB) $(VTL) view > $@
	$(VERB)  $(VTL) delete $* -f


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.%)     \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cachemisses.pa.%)  \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.%)       \
                     $(PROGRAMS_TO_TEST:%=Output/$(TEST).L2cacherefs.pa.%)
	@echo > $@
	@echo "CBE-PA-L2-Misses:" `cat Output/$(TEST).L2cachemisses.pa.$*` >> $@
	@echo "CBE-PA-L2-Refs:" `cat Output/$(TEST).L2cacherefs.pa.$*` >> $@
	@echo "CBE-L2-Misses:" `cat Output/$(TEST).L2cachemisses.$*` >> $@
	@echo "CBE-L2-Refs:" `cat Output/$(TEST).L2cacherefs.$*` >> $@


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

