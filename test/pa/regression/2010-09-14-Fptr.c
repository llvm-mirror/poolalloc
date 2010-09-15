/* 
 * Build this file into bitcode and run poolalloc on it
 * RUN: llvm-gcc -O0 %s --emit-llvm -c -o %t.bc
 * RUN: paopt %t.bc -poolalloc -o %t.pa.bc 2>&1
 * RUN: llc %t.pa.bc -o %t.pa.s
 * RUN: llvm-gcc %t.pa.s -o %t.pa
 *
 * Build the program without poolalloc:
 * RUN: llvm-gcc -o %t.native %s
 *
 * Execute the program to verify it's correct:
 * RUN: ./%t.pa >& %t.pa.out
 * RUN: /%t.native >& %t.native.out
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

int main(int argc, char ** argv)
{
  int val = MAGIC;
  getFP()(&val);
}
