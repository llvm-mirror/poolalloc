##===- lib/DSA/Makefile ------------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file was developed by the LLVM research group and is distributed under
# the University of Illinois Open Source License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##

LEVEL = ../..
LIBRARYNAME = LLVMDataStructure
BUILD_ARCHIVE := 1
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
#SHARED_LIBRARY := 1
#LOADABLE_MODULE := 1
endif
endif

#Bah. Confirmed on llvm-dev, there's no good way for us to do this.
#Essentially we're tasked with linking in the pieces that our plugin
#makes use of--but /not/ the tool we're using does /not/ make use of.
#It's dirty, but apparently what we're stuck with since we're a plugin
#LDFLAGS = $(LLVM_OBJ_ROOT)/lib/Support/$(BuildMode)/Triple.o

include $(LEVEL)/Makefile.common

CFlags += -Wno-deprecated

