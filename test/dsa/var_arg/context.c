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
//--check properties of this particular test
//RUN: dsaopt %t.bc -ds-aa -aa-eval -o /dev/null \
//  RUN: -print-all-alias-modref-info >& %t.aa

//FIXME: Find a better way to get at this information...
//--get the registers loaded from ret1 and ret2
//RUN: llvm-dis %t.bc -f -o %t.ll
//RUN: cat %t.ll | grep load | grep "ret1" | sed -e {s/ =.*$//} -e {s/^\[ \]*//} > %t.ret1
//RUN: cat %t.ll | grep load | grep "ret2" | sed -e {s/ =.*$//} -e {s/^\[ \]*//} > %t.ret2


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

  //ret1 and ret2 should explicitly /not/ alias
  //RUN: cat %t.aa | grep -f %t.ret1 | grep -f %t.ret2 | grep NoAlias
  ret1 = get( 0, p1 );
  ret2 = get( 0, p2 );

  if ( *ret1 + 1 == *ret2 )
  {
    return 0;
  }

  return -1;
}
