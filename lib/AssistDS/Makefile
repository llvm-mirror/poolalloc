##===- lib/AssistDS/Makefile -------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file was developed by the LLVM research group and is distributed under
# the University of Illinois Open Source License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##

LEVEL = ../..
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY=1
LOADABLE_MODULE=1
endif
endif
LIBRARYNAME = AssistDS
BUILD_ARCHIVE=1

include $(LEVEL)/Makefile.common

CFlags += -Wno-deprecated
