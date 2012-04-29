##===- TEST.dsgraph.Makefile -------------------------------*- Makefile -*-===##
#
# This recursively traverses the programs, computing DSGraphs for each of the
# programs in the testsuite.
#
##===----------------------------------------------------------------------===##

RELDIR  := $(subst $(PROJ_OBJ_ROOT),,$(PROJ_OBJ_DIR))

# We require the programs to be linked with libdummy
#include $(LEVEL)/Makefile.dummylib

# Pathname to poolalloc object tree
PADIR   := $(LLVM_OBJ_ROOT)/projects/poolalloc

# Pathame to the DSA pass dynamic library
DSA_SO   := $(PADIR)/$(CONFIGURATION)/lib/LLVMDataStructure$(SHLIBEXT)
ASSIST_SO := $(PADIR)/$(CONFIGURATION)/lib/AssistDS$(SHLIBEXT)

# Command for running the opt program
RUNOPT := $(RUNTOOLSAFELY) $(LLVM_OBJ_ROOT)/projects/poolalloc/$(CONFIGURATION)/bin/watchdog $(LOPT) -load $(DSA_SO)

# PASS - The dsgraph pass to run: ds, bu, td
PASS := td

ANALYZE_OPTS := -stats -time-passes -dsstats
ANALYZE_OPTS +=  -instcount -disable-verify -analyze -dont-print-ds
MEM := -track-memory -time-passes -disable-output

#TYPE_INFERENCE_OPT := 1

#ifdef TYPE_INFERENCE_OPT
ANALYZE_OPTS += -enable-type-inference-opts -dsa-stdlib-no-fold
#endif 

$(PROGRAMS_TO_TEST:%=Output/%.base.bc): \
Output/%.base.bc: Output/%.llvm.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO) -info-output-file=$(CURDIR)/$@.info -instnamer -internalize -indclone -funcspec -ipsccp -deadargelim -instcombine -globaldce -stats -time-passes $< -f -o $@ 

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.base.bc Output/%.LOC.txt $(LOPT)
	@# Gather data
	-($(RUNOPT) -dsa-$(PASS) $(ANALYZE_OPTS) $<)> $@.time.1 2>&1
	-($(RUNOPT)  $(MEM) -dsa-$(PASS) -disable-verify  $<)> $@.mem.1 2>&1
	@# Emit data.
	@echo "---------------------------------------------------------------" > $@
	@echo ">>> ========= '$(RELDIR)/$*' Program" >> $@
	@echo "---------------------------------------------------------------" >> $@
	@/bin/echo -n "LOC: " >> $@
	@cat Output/$*.LOC.txt >> $@
	@echo >> $@
	@/bin/echo -n "MEMINSTS: " >> $@
	-@grep 'Number of memory instructions' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "FOLDEDNODES: " >> $@
	-@grep 'Number of nodes completely folded' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "TOTALNODES: " >> $@
	-@grep 'Number of nodes allocated' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "MAXGRAPHSIZE: " >> $@
	-@grep 'Maximum graph size' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "GLOBALSGRAPH: " >> $@
	-@grep 'td.GlobalsGraph.dot' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "SCCSIZE: " >> $@
	-@grep 'Maximum SCC Size in Call Graph' $@.time.1 >> $@
	@echo >> $@
	@# Emit timing data.
	@/bin/echo -n "TIME: " >> $@
	-@grep '  Local Data Structure' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "TIME: " >> $@
	-@grep '  Bottom-up Data Structure' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "TIME: " >> $@
	-@grep '  Top-down Data Structure' $@.time.1 >> $@
	@echo >> $@
	@# Emit space data.
	@/bin/echo -n "MEM: " >> $@
	-@grep '  Local Data Structure' $@.mem.1 >> $@
	@echo >> $@
	@/bin/echo -n "MEM: " >> $@
	-@grep '  Bottom-up Data Structure' $@.mem.1 >> $@
	@echo >> $@
	@/bin/echo -n "MEM: " >> $@
	-@grep '  Top-down Data Structure' $@.mem.1 >> $@
	@# Emit AssistDS stats
	@/bin/echo -n "CLONED_FUNCSPEC: " >> $@
	-@grep 'Number of Functions Cloned in FuncSpec' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "CLONED_INDCLONE: " >> $@
	-@grep 'Number of Functions Cloned in IndClone' $<.info >> $@
	@echo >> $@



$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

# Define REPORT_DEPENDENCIES so that the report is regenerated if analyze or
# dummylib is updated.
#
REPORT_DEPENDENCIES := $(DUMMYLIB) $(LOPT)

