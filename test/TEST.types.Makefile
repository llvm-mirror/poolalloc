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
ASSIST_SO := $(PADIR)/$(CONFIGURATION)/lib/libAssistDS$(SHLIBEXT)

# Command for running the opt program
RUNOPT := $(RUNTOOLSAFELY) $(LLVM_OBJ_ROOT)/projects/poolalloc/$(CONFIGURATION)/bin/watchdog $(LOPT) -load $(DSA_SO)

# PASS - The dsgraph pass to run: ds, bu, td
PASS := td

ANALYZE_OPTS := -stats -time-passes -disable-output -dsstats
#ANALYZE_OPTS := -stats -time-passes -dsstats 
ANALYZE_OPTS +=  -instcount -disable-verify 
MEM := -track-memory -time-passes -disable-output

SAFE_OPTS := -internalize -scalarrepl -deadargelim -globaldce -basiccg -inline 

$(PROGRAMS_TO_TEST:%=Output/%.linked1.bc): \
Output/%.linked1.bc: Output/%.linked.rbc $(LOPT)
	-$(RUNOPT) -disable-opt $(SAFE_OPTS) -info-output-file=$(CURDIR)/$@.info -stats -time-passes $< -f -o $@ 

$(PROGRAMS_TO_TEST:%=Output/%.llvm1.bc): \
Output/%.llvm1.bc: Output/%.linked1.bc $(LLVM_LDDPROG)
	-$(RUNTOOLSAFELY) $(LLVMLD) -disable-opt $(SAFE_OPTS) -info-output-file=$(CURDIR)/$@.info -stats -time-passes  $(LLVMLD_FLAGS) $< -lc $(LIBS) -o Output/$*.llvm1

$(PROGRAMS_TO_TEST:%=Output/%.opt.bc): \
Output/%.opt.bc: Output/%.llvm1.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO) -disable-opt -info-output-file=$(CURDIR)/$@.info -instnamer -internalize  -varargsfunc -indclone -funcspec -ipsccp -deadargelim  -mergegep -die -globaldce -stats -time-passes $< -f -o $@ 

$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.opt.bc Output/%.LOC.txt $(LOPT)
	@# Gather data
	-($(RUNOPT) -dsa-$(PASS) -enable-type-inference-opts -dsa-stdlib-no-fold $(ANALYZE_OPTS) $<)> $@.time.1 2>&1
	-($(RUNOPT) -dsa-$(PASS) $(ANALYZE_OPTS) $<)> $@.time.2 2>&1
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
	@/bin/echo -n "ACCESSES TYPED_O: " >> $@
	-@grep 'Number of loads/stores which are fully typed' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES UNTYPED_O: " >> $@
	-@grep 'Number of loads/stores which are untyped' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES TYPED: " >> $@
	-@grep 'Number of loads/stores which are fully typed' $@.time.2 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES UNTYPED: " >> $@
	-@grep 'Number of loads/stores which are untyped' $@.time.2 >> $@
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
	@# Emit AssistDS stats
	@/bin/echo -n "CLONED_FUNCSPEC: " >> $@
	-@grep 'Number of Functions Cloned in FuncSpec' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "CLONED_INDCLONE: " >> $@
	-@grep 'Number of Functions Cloned in IndClone' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "VARARGS_CALLS: " >> $@
	-@grep 'Number of Calls Simplified' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "CALLS1: " >> $@
	-@grep 'Number of calls that could not be resolved' $@.time.1 >> $@
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

