/*
 * Build into bitcode
 * RUN: clang -O0 %s -emit-llvm -c -o %t.bc
 * RUN: adsaopt -internalize -mem2reg -typechecks %t.bc -o %t.tc.bc
 * RUN: tc-link %t.tc.bc -o %t.tc1.bc
 * RUN: llc %t.tc1.bc -o %t.tc1.s
 * RUN: clang++ %t.tc1.s -o %t.tc2
 * Execute
 * RUN: %t.tc2 >& %t.tc.out
 * RUN: grep "Type.*mismatch" %t.tc.out
 */
#include <stdio.h>

typedef int* (*funcptr)(double *);

static int *foo(double *d) {
  return (int*)d;
}
static int *bar(double *d) {
  return (int*)(d+1);
}
int main(int argc, char **argv)
{

 funcptr FP; 
 FP = &foo;
 if(argc > 5)
   FP = &bar;
 double d = 5.0;
 int *t = (*FP)(&d);
 int v = *t;

 return 0;
}
