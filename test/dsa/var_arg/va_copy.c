#include <stdarg.h>
#include <stdio.h>
//This tests va_copy, which should just merge it's arguments...

//--build the code into a .bc
//RUN: llvm-gcc -O0 %s -S --emit-llvm -o - | llvm-as > %t.bc
//--check if ds-aa breaks, breaks opts, or results in miscompiled code
//RUN: lli %t.bc > %t.refout
//RUN: dsaopt %t.bc -ds-aa -O3 -o - 2>/dev/null | lli > %t.out
//RUN: diff %t.refout %t.out
//--check properties of this particular test
//RUN: dsaopt %t.bc -ds-aa -aa-eval -o /dev/null \
//  RUN: -print-all-alias-modref-info >& %t.aa
//FIXME: Find a better way to get at this information...
//--get the registers loaded from val1 and val2
//RUN: llvm-dis %t.bc -f -o %t.ll
//RUN: cat %t.ll | grep load | grep "@val1" | sed -e {s/ =.*$//} -e {s/^\[ \]*//} > %t.val1
//RUN: cat %t.ll | grep load | grep "@val2" | sed -e {s/ =.*$//} -e {s/^\[ \]*//} > %t.val2
//--verify that they alias (they're int*'s)
//RUN: cat %t.aa | grep -f %t.val1 | grep -f %t.val2 | grep {^\[ \]*MayAlias}

int *val1, *val2;

static int get( int unused, ... )
{
  va_list ap, ap_copy;
  va_start( ap, unused );
  va_copy( ap_copy, ap );

  val1 = va_arg( ap, int* );
  va_end( ap );

  val2 = va_arg( ap_copy, int* );
  va_end( ap_copy );

  return *val1 - *val2;
}

int main()
{
  int stack_val = 5;

  int ret = get( 0, &stack_val );

  return ret;
}
