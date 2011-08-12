#include <stdio.h>
//This tests mod/ref behavior of extern var-arg functions
//We *should* special-case those we know about...
//..but for now we don't:
//XFAIL: *

//--build the code into a .bc
//RUN: clang -c -O0 %s -S -emit-llvm -o - | llvm-as > %t.bc
//--check if ds-aa breaks, or breaks opts
//RUN: dsaopt %t.bc -ds-aa -O3 -o /dev/null
//--check properties of this particular test
//RUN: dsaopt %t.bc -ds-aa -aa-eval -o /dev/null \
//  RUN: -print-all-alias-modref-info >& %t.aa

//Unknown external function
//Everything going into this should be assumed to be mod/ref'd
extern void unknown_extern(int, ...);

int main()
{
  int stack_val1 = 5;
  int stack_val2 = 10;
  int stack_val3 = 15;
  int stack_val4 = 20;
  int stack_val5 = 25;
  int stack_val6 = 30;

  //We should special case this--offhand, modref might be best
  //RUN: cat %t.aa | grep {Ptr:.*stack_val1.*scanf} | grep {^\[ \]*ModRef}
  //RUN: cat %t.aa | grep {Ptr:.*stack_val2.*scanf} | grep {^\[ \]*ModRef}
  scanf("%d, %d\n", &stack_val1, stack_val2);

  //We should special case this--ref's vars, not mod
  //RUN: cat %t.aa | grep {Ptr:.*stack_val3.*printf} | grep {^\[ \]*Ref}
  //RUN: cat %t.aa | grep {Ptr:.*stack_val4.*printf} | grep {^\[ \]*Ref}
  printf("%d, %d\n", &stack_val3, &stack_val4);

  //unknown--this these should be marked modref
  //RUN: cat %t.aa | grep {Ptr:.*stack_val5.*unknown_extern} | grep {^\[ \]*ModRef}
  //RUN: cat %t.aa | grep {Ptr:.*stack_val6.*unknown_extern} | grep {^\[ \]*ModRef}
  unknown_extern( 0, &stack_val5, &stack_val6);

  return 0;
}
