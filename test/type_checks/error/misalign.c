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

#include<stdio.h>


typedef unsigned char byte;    //!< byte type definition

int testEndian()
{
  short s;
  byte *p;

  p=(byte*)&s;

  s=1;

  return (*(p+1)==0);
}

int main() {
  testEndian();
  return 0;
}

