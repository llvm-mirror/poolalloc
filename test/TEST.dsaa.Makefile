##===- poolalloc/test/TEST.dsaa.Makefile -------------------*- Makefile -*-===##
#
# This test runs ds-aa on all of the Programs, producing some
# performance numbers and statistics.
#
##===----------------------------------------------------------------------===##


#CFLAGS += -O2 -fno-strict-aliasing
CFLAGS += -O0 -fno-strict-aliasing

CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/test-suite; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))
#PADIR   := /home/andrewl/Research/llvm/projects/poolalloc
PADIR   := $(LLVM_OBJ_ROOT)/projects/poolalloc

# Watchdog utility
WATCHDOG := $(LLVM_OBJ_ROOT)/projects/poolalloc/$(CONFIGURATION)/bin/watchdog

# Bits of runtime to improve analysis
PA_PRE_RT := $(PADIR)/$(CONFIGURATION)/lib/libpa_pre_rt.bca

# DSA pass shared object
DSA_SO   := $(PADIR)/$(CONFIGURATION)/lib/libLLVMDataStructure$(SHLIBEXT)

# Command to run opt with the ds-aa pass loaded
OPT_PA := $(WATCHDOG) $(LOPT) -load $(DSA_SO)

# OPT_PA_STATS - Run opt with the -stats and -time-passes options, capturing the
# output to a file.
OPT_PA_STATS = $(OPT_PA) -info-output-file=$(CURDIR)/$@.info -stats -time-passes

#All llvm 2.7 -O3 passes
#OPTZN_PASSES := -preverify -domtree -verify -lowersetjmp -globalopt -ipsccp -deadargelim -instcombine -simplifycfg -basiccg -prune-eh -inline -functionattrs -argpromotion -domtree -domfrontier -scalarrepl -simplify-libcalls -instcombine -jump-threading -simplifycfg -instcombine -tailcallelim -simplifycfg -reassociate -domtree -loops -loopsimplify -domfrontier -loopsimplify -lcssa -loop-rotate -licm -lcssa -loop-unswitch -instcombine -scalar-evolution -loopsimplify -lcssa -iv-users -indvars -loop-deletion -loopsimplify -lcssa -loop-unroll -instcombine -memdep -gvn -memdep -memcpyopt -sccp -instcombine -jump-threading -domtree -memdep -dse -adce -simplifycfg -strip-dead-prototypes -print-used-types -deadtypeelim -globaldce -constmerge -preverify -domtree -verify

#Subset of -O3 passes to work around bugs
OPTZN_PASSES := -preverify -domtree -verify -lowersetjmp -globalopt -ipsccp -deadargelim -instcombine -simplifycfg -basiccg -prune-eh -inline -functionattrs -argpromotion -domtree -domfrontier -scalarrepl -simplify-libcalls -instcombine -jump-threading -simplifycfg -instcombine -tailcallelim -simplifycfg -reassociate -domtree -loops -loopsimplify -domfrontier -loopsimplify -lcssa -loop-rotate -lcssa -loop-unswitch -instcombine -scalar-evolution -loopsimplify -lcssa -iv-users -indvars -loop-deletion -loopsimplify -lcssa -loop-unroll -instcombine -memdep -gvn -memdep -memcpyopt -sccp -instcombine -jump-threading -domtree -adce -simplifycfg -strip-dead-prototypes -print-used-types -deadtypeelim -globaldce -constmerge -preverify -domtree -verify

#-ds-aa -opt1 -ds-aa -opt2, etc
#this forces each pass to use -ds-aa as it's AA if it would use one
AA_OPT = $(addprefix -ds-aa ,$(OPTZN_PASSES))

## Set of passes to run before -ds-aa
#BASE_OPT_PASSES := -instnamer -internalize -indclone -funcspec -ipsccp -deadargelim -instcombine -globaldce 
BASE_OPT_PASSES := -disable-opt

$(PROGRAMS_TO_TEST:%=Output/%.temp.bc): \
Output/%.temp.bc: Output/%.llvm.bc 
	-$(LLVMLD) -link-as-library $< $(PA_PRE_RT) -o $@

$(PROGRAMS_TO_TEST:%=Output/%.base.bc): \
Output/%.base.bc: Output/%.temp.bc $(LOPT)
	-$(LOPT) $(BASE_OPT_PASSES) -stats $< -f -o $@ 

#LLVM with all opts
$(PROGRAMS_TO_TEST:%=Output/%.llvmopt.bc): \
Output/%.llvmopt.bc: Output/%.base.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) $(OPTZN_PASSES) $< -o $@ -f 2>&1 > $@.out

#DSA --run all opts, having them make use of -ds-aa
$(PROGRAMS_TO_TEST:%=Output/%.dsaopt.bc): \
Output/%.dsaopt.bc: Output/%.base.bc $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(OPT_PA_STATS) $(AA_OPT) $< -o $@ -f 2>&1 > $@.out

# This rule compiles the new .bc file into a .s file
Output/%.s: Output/%.bc $(LLC)
	-$(LLC) $< -o $@

Output/%: Output/%.s $(PA_RT_O)
	-$(CC) $(CFLAGS) $< $(PA_RT_O) $(LLCLIBS) $(LDFLAGS) -o $@


ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

# This rule runs the generated executable, generating timing information, for
# normal test programs
$(PROGRAMS_TO_TEST:%=Output/%.llvmopt.out): \
Output/%.llvmopt.out: Output/%.llvmopt
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

$(PROGRAMS_TO_TEST:%=Output/%.dsaopt.out): \
Output/%.dsaopt.out: Output/%.dsaopt
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)
else

# This rule runs the generated executable, generating timing information, for
# SPEC
$(PROGRAMS_TO_TEST:%=Output/%.llvmopt.out): \
Output/%.llvmopt.out: Output/%.llvmopt
	-$(SPEC_SANDBOX) dsa-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/dsa-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/dsa-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

$(PROGRAMS_TO_TEST:%=Output/%.dsaopt.out): \
Output/%.dsaopt.out: Output/%.dsaopt
	-$(SPEC_SANDBOX) dsaopt-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/dsaopt-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/dsaopt-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

endif


#Diff llvm's optimized vs gcc, as well as ds-aa optimized vs gcc
$(PROGRAMS_TO_TEST:%=Output/%.llvmopt.diff-nat): \
Output/%.llvmopt.diff-nat: Output/%.out-nat Output/%.llvmopt.out
	@cp Output/$*.out-nat Output/$*.llvmopt.out-nat
	-$(DIFFPROG) nat $*.llvmopt $(HIDEDIFF)

$(PROGRAMS_TO_TEST:%=Output/%.dsaopt.diff-nat): \
Output/%.dsaopt.diff-nat: Output/%.out-nat Output/%.dsaopt.out
	@cp Output/$*.out-nat Output/$*.dsaopt.out-nat
	-$(DIFFPROG) nat $*.dsaopt $(HIDEDIFF)


# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
                             Output/%.dsaopt.diff-nat         \
			     Output/%.llvmopt.diff-nat     \
                             Output/%.LOC.txt
	@-cat $<
	@echo > $@
	@echo "---------------------------------------------------------------" >> $@
	@echo ">>> ========= '$(RELDIR)/$*' Program" >> $@
	@echo "---------------------------------------------------------------" >> $@
	@echo >> $@
	@-if test -f Output/$*.dsaopt.diff-nat; then \
	  printf "GCC-RUN-TIME: " >> $@;\
	  grep "^program" Output/$*.out-nat.time >> $@;\
        fi
	@-if test -f Output/$*.llvmopt.diff-nat; then \
	  printf "RUN-TIME-LLVMOPT: " >> $@;\
	  grep "^program" Output/$*.llvmopt.out.time >> $@;\
        fi
	@-if test -f Output/$*.dsaopt.diff-nat; then \
	  printf "RUN-TIME-DSAAOPT: " >> $@;\
	  grep "^program" Output/$*.dsaopt.out.time >> $@;\
	fi
	-printf "LOC: " >> $@
	-cat Output/$*.LOC.txt >> $@
	@#-cat Output/$*.$(TEST).bc.info >> $@
	@#cat Output/$*.$(TEST).basepa.bc.out  >> $@


$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@-cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
