#===- tools/pa/Makefile ------------------------------------*- Makefile -*-===##
# 
#                     Automatic Pool Allocation Project
#
# This file was developed by the LLVM research group and is distributed under
# the University of Illinois Open Source License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##

LEVEL = ../..
TOOLNAME=pa

# Initialize the USEDLIBS so we can add to it

LINK_COMPONENTS :=  bitreader bitwriter instrumentation scalaropts ipo \
                    nativecodegen
USEDLIBS := poolalloc.a LLVMDataStructure.a

# Include this here so we can get the configuration of the targets
# that have been configured for construction. We have to do this 
# early so we can set up USEDLIBS properly before includeing Makefile.rules
include $(LEVEL)/Makefile.common


