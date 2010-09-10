#!/bin/sh

# Location of the LLVM source and object trees
LLVMDIR=$HOME/cronjobs/llvm27

# Location of test suite object tree
TESTSUITE=$LLVMDIR/projects/test-suite

# Location of file containing log of the test build and run
LOGFILE=$LLVMDIR/projects/poolalloc/test/results

# List of directories to clean before test
TESTDIRS="MultiSource/Benchmarks/Olden External/SPEC/CINT2000"

#
# Switch to the LLVM source tree.
#
cd $LLVMDIR

#
# Make sure LLVM is up-to-date.
#
echo "Updating LLVM"
svn up
echo "Building LLVM"
make -s -j3 tools-only

#
# Update and build Automatic Pool Allocation
#
cd $LLVMDIR/projects/poolalloc
echo "Updating Poolalloc"
svn up
echo "Building Poolalloc"
make -s -j3

#
# Clean out the old test files.
#
echo "Cleaning out old test files..."
for dir in $TESTDIRS
do
  cd $TESTSUITE/$dir
  make clean
done

#
# Run the automatic pool allocation tests.
#
echo "Testing Poolalloc..."
cd $LLVMDIR/projects/poolalloc/test
mv $LOGFILE $LOGFILE.old
make NO_STABLE_NUMBERS=1 -j3 progtest 2>&1 > $LOGFILE

#
# Print out the results.
#
for dir in $TESTDIRS
do
  cat $TESTSUITE/$dir/report.poolalloc.txt
done

