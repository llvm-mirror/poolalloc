/*
 * Build into bitcode
 * RUN: clang -O0 %s -emit-llvm -c -o %t.bc
 * RUN: adsaopt -internalize -mem2reg -typechecks %t.bc -o %t.tc.bc
 * RUN: tc-link %t.tc.bc -o %t.tc1.bc
 * RUN: llc %t.tc1.bc -o %t.tc1.s
 * RUN: clang++ %t.tc1.s -o %t.tc2
 * Execute
 * RUN: %t.tc2 >& %t.tc.out
 * RUN: not grep "Type.*mismatch" %t.tc.out
 */
/* vprintf example */
#include <stdio.h>
#include <stdarg.h>

void WriteFormatted (char * format, ...)
{
  va_list args;
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
}

int main ()
{
  WriteFormatted ("Call with %d variable argument.\n",1);
  WriteFormatted ("Call with %d variable %s.\n",2,"arguments");

  return 0;
}
