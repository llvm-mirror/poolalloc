#include <stdarg.h>
#include <stdio.h>
//This is a test of context-sensitive var-arg handling

//--build the code into a .bc
//RUN: llvm-gcc -O0 %s -S --emit-llvm -o - | llvm-as > %t.bc
//--check if ds-aa breaks, breaks opts, or results in miscompiled code
//RUN: lli %t.bc > %t.refout
//RUN: dsaopt %t.bc -ds-aa -O3 -o - | lli > %t.out
//RUN: dsaopt %t.bc -ds-aa -gvn -o - | lli > %t.out2
//RUN: diff %t.refout %t.out
//RUN: diff %t.refout %t.out2


static int * get( int unused, ... )
{
  va_list ap;
  va_start( ap, unused );

  int * ret = va_arg( ap, int * );

  va_end( ap );

  return ret;
}

int main()
{
  int val1 = 1, val2 = 2;
  int *p1 = &val1, *p2 = &val2;
  int *ret1, *ret2;

  ret1 = get( 0, p1 );
  ret2 = get( 0, p2 );

  if ( *ret1 + 1 == *ret2 )
  {
    return 0;
  }

  return -1;
}
