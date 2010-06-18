#include <stdarg.h>
#include <stdio.h>
//This is a basic use of vararg pointer use

//--build the code into a .bc
//RUN: llvm-gcc -O0 %s -S --emit-llvm -o - | llvm-as > %t.bc
//--check if ds-aa breaks, breaks opts, or results in miscompiled code
//RUN: lli %t.bc > %t.refout
//RUN: dsaopt %t.bc -ds-aa -O3 -o - | lli > %t.out
//RUN: diff %t.refout %t.out
//--check properties of this particular test
//N/A

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
