/*
 * Build into bitcode
 * RUN: llvm-gcc -O0 %s --emit-llvm -c -o %t.bc
 * RUN: adsaopt -internalize -mem2reg -typechecks %t.bc -o %t.tc.bc
 * RUN: tc-link %t.tc.bc -o %t.tc1.bc
 * RUN: llc %t.tc1.bc -o %t.tc1.s
 * RUN: llvm-gcc %t.tc1.s -o %t.tc2
 * Execute
 * RUN: %t.tc2 >& %t.tc.out
 * ;XFAIL:*
 */

#include<stdio.h>


typedef unsigned char byte;    //!< byte type definition

int testEndian()
{
  short s;
  byte *p;

  p=(byte*)&s;

  s=1;

  return (*p==0);
}

int main() {
  testEndian();
  return 0;
}

