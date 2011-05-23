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

# Bits of runtime to improve analysis
PA_PRE_RT := $(PADIR)/$(CONFIGURATION)/lib/libpa_pre_rt.bca

# Pathame to the DSA pass dynamic library
DSA_SO   := $(PADIR)/$(CONFIGURATION)/lib/libLLVMDataStructure$(SHLIBEXT)
ASSIST_SO := $(PADIR)/$(CONFIGURATION)/lib/libAssistDS$(SHLIBEXT)

# Command for running the opt program
RUNOPT := $(RUNTOOLSAFELY) $(LLVM_OBJ_ROOT)/projects/poolalloc/$(CONFIGURATION)/bin/watchdog $(LOPT) -load $(DSA_SO)

# PASS - The dsgraph pass to run: ds, bu, td
PASS := td

TYPE_RT_O := $(PADIR)/$(CONFIGURATION)/lib/libtypechecks_rt.a
TYPE_RT_BC := $(PADIR)/$(CONFIGURATION)/lib/libtypechecks_rt.bca
DYNCOUNT_RT_O := $(PADIR)/$(CONFIGURATION)/lib/libcount.a
ANALYZE_OPTS := -stats -time-passes -disable-output -dsstats
#ANALYZE_OPTS := -stats -time-passes -dsstats 
ANALYZE_OPTS +=  -instcount -disable-verify 
MEM := -track-memory -time-passes -disable-output

#SAFE_OPTS := -internalize -scalarrepl -deadargelim -globaldce -basiccg -inline 
SAFE_OPTS := -internalize  -deadargelim -globaldce -basiccg -inline  
#SAFE_OPTS := -internalize   -deadargelim -globaldce 

$(PROGRAMS_TO_TEST:%=Output/%.linked1.bc): \
Output/%.linked1.bc: Output/%.linked.rbc $(LOPT)
	-$(RUNOPT) -disable-opt $(SAFE_OPTS) -mem2reg -dce -info-output-file=$(CURDIR)/$@.info -stats -time-passes $< -f -o $@ 

$(PROGRAMS_TO_TEST:%=Output/%.llvm1.bc): \
Output/%.llvm1.bc: Output/%.linked1.bc $(LLVM_LDDPROG)
	-$(RUNTOOLSAFELY) $(LLVMLD) -disable-opt $(SAFE_OPTS) -info-output-file=$(CURDIR)/$@.info -stats -time-passes  $(LLVMLD_FLAGS) $< -lc $(LIBS) -o Output/$*.llvm1

$(PROGRAMS_TO_TEST:%=Output/%.temp1.bc): \
Output/%.temp1.bc: Output/%.llvm1.bc 
	-$(RUNTOOLSAFELY) $(LLVMLD) -disable-opt $(SAFE_OPTS) -link-as-library $< $(PA_PRE_RT) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.opt.bc): \
Output/%.opt.bc: Output/%.llvm1.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO) -disable-opt -info-output-file=$(CURDIR)/$@.info -instnamer -internalize -mem2reg -dce -basiccg -inline -dce -arg-cast -indclone -funcspec -ipsccp -deadargelim -simplify-gep -die -mergearrgep -die -globaldce -simplifycfg -deadargelim -arg-simplify -die -arg-cast -die -simplifycfg -globaldce -indclone -funcspec -deadargelim -globaldce -die -simplifycfg -gep-expr-arg -deadargelim -die -mergefunc -die -mergearrgep -die -globaldce -int2ptrcmp -die -dce -inline -mem2reg -dce -arg-cast -dce -sretpromotion -struct-ret -deadargelim -simplify-ev -simplify-iv -dce -ld-args -gep-expr-arg -deadargelim -mergefunc -dce -stats -time-passes $< -f -o $@ 

$(PROGRAMS_TO_TEST:%=Output/%.count.bc): \
Output/%.count.bc: Output/%.opt.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO) -enable-type-inference-opts -dsa-stdlib-no-fold -dyncount -disable-opt -info-output-file=$(CURDIR)/$@.info $< -f -o $@ 

$(PROGRAMS_TO_TEST:%=Output/%.count1.bc): \
Output/%.count1.bc: Output/%.opt.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO) -dyncount -disable-opt -info-output-file=$(CURDIR)/$@.info $< -f -o $@ 

$(PROGRAMS_TO_TEST:%=Output/%.tc.bc): \
Output/%.tc.bc: Output/%.opt.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO)  -typechecks -dce -ipsccp -dce -stats -info-output-file=$(CURDIR)/$@.info $< -f -o $@.temp
	-$(LLVMLD) -disable-opt -o $@.ld $@.temp $(TYPE_RT_BC)
	-$(LOPT) $(SAFE_OPTS) $@.ld.bc -o $@ -f


$(PROGRAMS_TO_TEST:%=Output/%.tco.bc): \
Output/%.tco.bc: Output/%.opt.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO) -typechecks -enable-type-safe-opt -dce -ipsccp -dce -stats -info-output-file=$(CURDIR)/$@.info $< -f -o $@.temp 
	-$(LLVMLD) -disable-opt -o $@.ld $@.temp $(TYPE_RT_BC)
	-$(LOPT) $(SAFE_OPTS) $@.ld.bc -o $@ -f

$(PROGRAMS_TO_TEST:%=Output/%.tcoo.bc): \
Output/%.tcoo.bc: Output/%.opt.bc $(LOPT) $(ASSIST_SO)
	-$(RUNOPT) -load $(ASSIST_SO) -typechecks -enable-type-safe-opt -enable-type-inference-opts -dsa-stdlib-no-fold -dce -ipsccp -dce -stats -info-output-file=$(CURDIR)/$@.info $< -f -o $@.temp 
	-$(LLVMLD) -disable-opt -o $@.ld $@.temp $(TYPE_RT_BC)
	-$(LOPT) $(SAFE_OPTS) $@.ld.bc -o $@ -f

$(PROGRAMS_TO_TEST:%=Output/%.count.s): \
Output/%.count.s: Output/%.count.bc $(LLC)
	-$(LLC)  $< -o $@
$(PROGRAMS_TO_TEST:%=Output/%.count1.s): \
Output/%.count1.s: Output/%.count1.bc $(LLC)
	-$(LLC)  $< -o $@
$(PROGRAMS_TO_TEST:%=Output/%.opt.s): \
Output/%.opt.s: Output/%.opt.bc $(LLC)
	-$(LLC)  $< -o $@
$(PROGRAMS_TO_TEST:%=Output/%.llvm1.s): \
Output/%.llvm1.s: Output/%.llvm1.bc $(LLC)
	-$(LLC)  $< -o $@
$(PROGRAMS_TO_TEST:%=Output/%.tc.s): \
Output/%.tc.s: Output/%.tc.bc $(LLC)
	-$(LLC)  $< -o $@
$(PROGRAMS_TO_TEST:%=Output/%.tco.s): \
Output/%.tco.s: Output/%.tco.bc $(LLC)
	-$(LLC)  $< -o $@
$(PROGRAMS_TO_TEST:%=Output/%.tcoo.s): \
Output/%.tcoo.s: Output/%.tcoo.bc $(LLC)
	-$(LLC)  $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.opt): \
Output/%.opt: Output/%.opt.s
	-$(CC) $(CFLAGS) $<  $(LLCLIBS) $(LDFLAGS) -o $@
$(PROGRAMS_TO_TEST:%=Output/%.tc): \
Output/%.tc: Output/%.tc.s $(TYPE_RT_O)
	-$(CC) $(CFLAGS) $<  $(LLCLIBS) $(TYPE_RT_O) $(LDFLAGS) -o $@
$(PROGRAMS_TO_TEST:%=Output/%.tco): \
Output/%.tco: Output/%.tco.s $(TYPE_RT_O)
	-$(CC) $(CFLAGS) $<  $(LLCLIBS) $(TYPE_RT_O) $(LDFLAGS) -o $@
$(PROGRAMS_TO_TEST:%=Output/%.tcoo): \
Output/%.tcoo: Output/%.tcoo.s $(TYPE_RT_O)
	-$(CC) $(CFLAGS) $<  $(LLCLIBS) $(TYPE_RT_O) $(LDFLAGS) -o $@
$(PROGRAMS_TO_TEST:%=Output/%.llvm1): \
Output/%.llvm1: Output/%.llvm1.s 
	-$(CC) $(CFLAGS) $<  $(LLCLIBS) $(LDFLAGS) -o $@
$(PROGRAMS_TO_TEST:%=Output/%.count): \
Output/%.count: Output/%.count.s 
	-$(CC) $(CFLAGS) $<  $(LLCLIBS) $(DYNCOUNT_RT_O) $(LDFLAGS) -o $@
$(PROGRAMS_TO_TEST:%=Output/%.count1): \
Output/%.count1: Output/%.count1.s 
	-$(CC) $(CFLAGS) $<  $(LLCLIBS) $(DYNCOUNT_RT_O) $(LDFLAGS) -o $@

ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

$(PROGRAMS_TO_TEST:%=Output/%.opt.out): \
Output/%.opt.out: Output/%.opt
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
$(PROGRAMS_TO_TEST:%=Output/%.llvm1.out): \
Output/%.llvm1.out: Output/%.llvm1
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
$(PROGRAMS_TO_TEST:%=Output/%.count1.out): \
Output/%.count1.out: Output/%.count1
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
	-cp lsstats lsstats1
$(PROGRAMS_TO_TEST:%=Output/%.count.out): \
Output/%.count.out: Output/%.count
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
	-cp lsstats lsstats2
$(PROGRAMS_TO_TEST:%=Output/%.tc.out): \
Output/%.tc.out: Output/%.tc
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
$(PROGRAMS_TO_TEST:%=Output/%.tco.out): \
Output/%.tco.out: Output/%.tco
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
$(PROGRAMS_TO_TEST:%=Output/%.tcoo.out): \
Output/%.tcoo.out: Output/%.tcoo
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

else
$(PROGRAMS_TO_TEST:%=Output/%.opt.out): \
Output/%.opt.out: Output/%.opt
	-$(SPEC_SANDBOX) opt-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/opt-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/opt-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time
$(PROGRAMS_TO_TEST:%=Output/%.llvm1.out): \
Output/%.llvm1.out: Output/%.llvm1
	-$(SPEC_SANDBOX) llvm1-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/llvm1-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/llvm1-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time
$(PROGRAMS_TO_TEST:%=Output/%.tc.out): \
Output/%.tc.out: Output/%.tc
	-$(SPEC_SANDBOX) tc-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/tc-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/tc-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time
$(PROGRAMS_TO_TEST:%=Output/%.tco.out): \
Output/%.tco.out: Output/%.tco
	-$(SPEC_SANDBOX) tco-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/tco-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/tco-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time
$(PROGRAMS_TO_TEST:%=Output/%.tcoo.out): \
Output/%.tcoo.out: Output/%.tcoo
	-$(SPEC_SANDBOX) tcoo-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/tcoo-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/tcoo-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time
$(PROGRAMS_TO_TEST:%=Output/%.count.out): \
Output/%.count.out: Output/%.count
	-$(SPEC_SANDBOX) count-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/count-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/count-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time
	-cp Output/count-$(RUN_TYPE)/lsstats lsstats2
$(PROGRAMS_TO_TEST:%=Output/%.count1.out): \
Output/%.count1.out: Output/%.count1
	-$(SPEC_SANDBOX) count1-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/count1-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/count1-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time
	-cp Output/count1-$(RUN_TYPE)/lsstats lsstats1

endif

$(PROGRAMS_TO_TEST:%=Output/%.opt.diff-nat): \
Output/%.opt.diff-nat: Output/%.out-nat Output/%.opt.out
	@cp Output/$*.out-nat Output/$*.opt.out-nat
	@cp Output/$*.opt.out Output/$*.tco.out-opt
	-$(DIFFPROG) opt $*.opt $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.tc.diff-nat): \
Output/%.tc.diff-nat: Output/%.out-nat Output/%.tc.out
	@cp Output/$*.out-nat Output/$*.tc.out-nat
	@cp Output/$*.tc.out Output/$*.tco.out-tc
	-$(DIFFPROG) tc $*.tc $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.tco.diff-nat): \
Output/%.tco.diff-nat: Output/%.out-nat Output/%.tco.out
	@cp Output/$*.out-nat Output/$*.tco.out-nat
	@cp Output/$*.tco.out Output/$*.tco.out-tco
	-$(DIFFPROG) tco $*.tco $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.tcoo.diff-nat): \
Output/%.tcoo.diff-nat: Output/%.out-nat Output/%.tcoo.out
	@cp Output/$*.out-nat Output/$*.tcoo.out-nat
	@cp Output/$*.tcoo.out Output/$*.tco.out-tcoo
	-$(DIFFPROG) tcoo $*.tcoo $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.llvm1.diff-nat): \
Output/%.llvm1.diff-nat: Output/%.out-nat Output/%.llvm1.out
	@cp Output/$*.out-nat Output/$*.llvm1.out-nat
	@cp Output/$*.llvm1.out Output/$*.llvm1.out-llvm1
	-$(DIFFPROG) llvm1 $*.llvm1 $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.count.diff-nat): \
Output/%.count.diff-nat: Output/%.out-nat Output/%.count.out
	@cp Output/$*.out-nat Output/$*.count.out-nat
	@cp Output/$*.count.out Output/$*.llvm1.out-count
	-$(DIFFPROG) count $*.count $(HIDEDIFF)
$(PROGRAMS_TO_TEST:%=Output/%.count1.diff-nat): \
Output/%.count1.diff-nat: Output/%.out-nat Output/%.count1.out
	@cp Output/$*.out-nat Output/$*.count1.out-nat
	@cp Output/$*.count1.out Output/$*.llvm1.out-count1
	-$(DIFFPROG) count1 $*.count1 $(HIDEDIFF)


$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.opt.bc Output/%.LOC.txt $(LOPT) Output/%.out-nat Output/%.llvm1.diff-nat Output/%.opt.diff-nat Output/%.tc.diff-nat Output/%.tco.diff-nat Output/%.tcoo.diff-nat
	@# Gather data
	-($(RUNOPT)  -dsa-$(PASS) -enable-type-inference-opts -dsa-stdlib-no-fold $(ANALYZE_OPTS) $<)> $@.time.1 2>&1
	-($(RUNOPT)  -dsa-$(PASS)  $(ANALYZE_OPTS) $<)> $@.time.2 2>&1
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
	@/bin/echo -n "ACCESSES TYPED0: " >> $@
	-@grep 'Number of loads/stores which are access a DSNode with 0 type' $@.time.1 >> $@
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
	@/bin/echo -n "IGN: " >> $@
	-@grep 'Number of instructions ignored' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "GEPI: " >> $@
	-@grep 'Number of gep instructions ignored' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES I: " >> $@
	-@grep 'Number of loads/stores which are on incomplete nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES E: " >> $@
	-@grep 'Number of loads/stores which are on external nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES F: " >> $@
	-@grep 'Number of loads/stores which are on folded nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES U: " >> $@
	-@grep 'Number of loads/stores which are on unknown nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "ACCESSES I2P: " >> $@
	-@grep 'Number of loads/stores which are on inttoptr nodes' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "I2PB: " >> $@
	-@grep 'Number of inttoptr used only in cmp' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "I2PS: " >> $@
	-@grep 'Number of inttoptr from ptrtoint' $@.time.1 >> $@
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
	@# Emit runtime data.
	@-if test -f Output/$*.opt.diff-nat; then \
	  printf "OPT-RUN_TIME: " >> $@;\
	  grep 'program' Output/$*.opt.out.time >> $@;\
	fi
	@-if test -f Output/$*.tc.diff-nat; then \
	  printf "TC-RUN_TIME: " >> $@;\
	  grep 'program' Output/$*.tc.out.time >> $@;\
	fi
	@-if test -f Output/$*.tco.diff-nat; then \
	  printf "TCO-RUN_TIME: " >> $@;\
	  grep 'program' Output/$*.tco.out.time >> $@;\
	fi
	@-if test -f Output/$*.tcoo.diff-nat; then \
	  printf "TCOO-RUN_TIME: " >> $@;\
	  grep 'program' Output/$*.tcoo.out.time >> $@;\
	fi
	@# Emit AssistDS stats
	@/bin/echo -n "CLONED_FUNCSPEC: " >> $@
	-@grep 'Number of Functions Cloned in FuncSpec' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "CLONED_INDCLONE: " >> $@
	-@grep 'Number of Functions Cloned in IndClone' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "GEP_CALLS: " >> $@
	-@grep 'Number of Calls Modified' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "ARG_SMPL: " >> $@
	-@grep 'Number of Args changeable' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "EV: " >> $@
	-@grep 'Number of Instructions Deleted' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "ALLOC: " >> $@
	-@grep 'Number of malloc-like allocators' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "DEALLOC: " >> $@
	-@grep 'Number of free-like deallocators' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "CAST: " >> $@
	-@grep 'Number of Args bitcasted' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "INDCALLS: " >> $@
	-@grep 'Number of unresolved IndCalls' $@.time.1 >> $@
	@echo >> $@
	@/bin/echo -n "DTOTALO: " >> $@
	-@grep 'Total' lsstats2 >> $@
	@echo >> $@
	@/bin/echo -n "DSAFEO: " >> $@
	-@grep 'Safe' lsstats2 >> $@
	@echo >> $@
	@/bin/echo -n "DTOTAL: " >> $@
	-@grep 'Total' lsstats1 >> $@
	@echo >> $@
	@/bin/echo -n "DSAFE: " >> $@
	-@grep 'Safe' lsstats1 >> $@
	@echo >> $@
	@/bin/echo -n "LCHK: " >> $@
	-@grep 'Number of Load Insts that need type checks' $<.info >> $@
	@echo >> $@
	@/bin/echo -n "SCHK: " >> $@
	-@grep 'Number of Store Insts that need type checks' $<.info >> $@
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
REPORT_DEPENDENCIES := $(DUMMYLIB) $(LOPT) $(ASSIST_SO)

