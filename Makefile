#
# This is a sample Makefile for a project that uses LLVM.
#

#
# Indicates our relative path to the top of the project's root directory.
#
LEVEL = .

#
# Directories that needs to be built.
#
DIRS = lib bytecode

#
# This is needed since the tags generation code expects a tools directory
# to exist.
#
all::
	mkdir -p tools

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common

distclean:: clean
	${RM} -f Makefile.common Makefile.config

