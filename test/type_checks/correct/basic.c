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
#include <stdarg.h>
#include <stdio.h>
//This is a basic use of vararg pointer use

static int get( int unused, ... )
{
  va_list ap;
  va_start( ap, unused );
  int *val = va_arg( ap, int* );
  va_end( ap );

  return *val;
}

int main()
{
  int stack_val = 5;

  int ret = get( 0, &stack_val );

  return ret - 5;
}
