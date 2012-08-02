// This is the same as 2010-09-Fptr.c, but does it with a helper
// to verify/illustrate that the issue is /not/ "main"-specific.
// (and to catch fixes that attempt to fix it as such)
/* 
 * Build this file into bitcode and run poolalloc on it
 * RUN: clang -O0 %s -emit-llvm -c -o %t.bc
 * RUN: paopt %t.bc -paheur-AllButUnreachableFromMemory -poolalloc -o %t.pa.bc 2>&1
 * RUN: clang %t.pa.bc -o %t.pa -L%llvmshlibdir -lpoolalloc_rt -Wl,-rpath %llvmshlibdir
 *
 * Build the program without poolalloc:
 * RUN: clang -o %t.native %s
 *
 * Execute the program to verify it's correct:
 * RUN: %t.pa >& %t.pa.out
 * RUN: %t.native >& %t.native.out
 *
 * Diff the two executions
 * RUN: diff %t.pa.out %t.native.out
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAGIC 0xBEEF

typedef void (*FP)(int *);

void callee(int * v)
{
  printf("*v: %d\n", *v);
  assert(*v == MAGIC);
}

FP getFP()
{
  return callee;
}

void helper(int *v)
{
  getFP()(v);
}

int main(int argc, char ** argv)
{
  int val = MAGIC;
  helper(&val);
  return 0;
}
