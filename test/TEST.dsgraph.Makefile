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
DSA_SO   := $(PADIR)/$(CONFIGURATION)/lib/libLLVMDataStructure$(SHLIBEXT)

# Command for running the opt program
RUNOPT := $(RUNTOOLSAFELY) $(LLVM_OBJ_ROOT)/projects/poolalloc/$(CONFIGURATION)/bin/watchdog $(LOPT) -load $(DSA_SO)

# PASS - The dsgraph pass to run: ds, bu, td
PASS := td

ANALYZE_OPTS := -stats -time-passes -disable-output -dsstats
#ANALYZE_OPTS := -stats -time-passes -dsstats 
ANALYZE_OPTS +=  -instcount -disable-verify
MEM := -track-memory -time-passes -disable-output

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.llvm.bc Output/%.LOC.txt $(LOPT)
	@# Gather data
	-($(RUNOPT) -dsa-$(PASS)  $(ANALYZE_OPTS) $<)> $@.time.1 2>&1
	-($(RUNOPT) -dsa-$(PASS) -dsa-stdlib-no-fold  $(ANALYZE_OPTS) $<)> $@.time.2 2>&1
	-($(RUNOPT)  $(MEM) -dsa-$(PASS) -disable-verify -debug-pass=Details $<)> $@.mem.1 2>&1
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
	-@grep 'Number of folded nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "TOTALNODES: " >> $@
	-@grep 'Graphs contain.*nodes total' $@.time.1 >> $@
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
	@/bin/echo -n "ACCESSES TYPED: " >> $@
	-@grep 'Number of loads/stores which are fully typed' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES UNTYPED: " >> $@
	-@grep 'Number of loads/stores which are untyped' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES TYPED1: " >> $@
	-@grep 'Number of loads/stores which are access a DSNode with 1 type' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES TYPED2: " >> $@
	-@grep 'Number of loads/stores which are access a DSNode with 2 type' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES TYPED3: " >> $@
	-@grep 'Number of loads/stores which are access a DSNode with 3 type' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES TYPED4: " >> $@
	-@grep 'Number of loads/stores which are access a DSNode with >3 type' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES I: " >> $@
	-@grep 'Number of loads/stores which are on incomplete nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES E: " >> $@
	-@grep 'Number of loads/stores which are on external nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES U: " >> $@
	-@grep 'Number of loads/stores which are on unknown nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "STD_LIB_FOLD: " >> $@
	-@grep 'Number of nodes folded in std lib' $@.time.1 >> $@
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

