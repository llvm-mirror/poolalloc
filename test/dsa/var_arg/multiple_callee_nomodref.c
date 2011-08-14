//TODO: Update this to not use ds-aa!
//XFAIL: *
#include <stdarg.h>
//This tests having multiple parameters
//In particular, this verifies that dsa doesn't unnecessarily
//set the mod/ref flag for functions.

//What to check:
//'val' should alias stack_val and stack_val2
//'p1' and 'p2' should alias
//'p1' and 'p2' should be modref'd by assign
//(accordingly stack_val/stack_val2 are modref'd)
//
//--build the code into a .bc
//RUN: clang -O0 %s -S -emit-llvm -o - | llvm-as > %t.bc
//--check if ds-aa breaks, breaks opts, or results in miscompiled code
//RUN: lli %t.bc > %t.refout
//RUN: dsaopt %t.bc -ds-aa -gvn -o - | lli > %t.out
//RUN: diff %t.refout %t.out
//--check properties of this particular test
//RUN: dsaopt %t.bc -ds-aa -aa-eval -o /dev/null \
//  RUN: -print-all-alias-modref-info >& %t.aa
//ds-aa should tell us that assign doesn't mod/ref p1
//RUN: cat %t.aa | grep {Ptr:.*p1.*@assign} | grep NoModRef
//ds-aa should tell us that assign doesn't mod/ref p2
//RUN: cat %t.aa | grep {Ptr:.*p2.*@assign} | grep NoModRef


static int assign( int count, ... )
{
  va_list ap;
  va_start( ap, count );

  int sum = 0;
  int i = 1;
  int ** old = va_arg( ap, int** );
  for ( ; i < count; ++i )
  {
    int **val = va_arg( ap, int** );
    //Unlike 'multiple_callee.c', don't mod/ref the value
    //*old = *val;
    old = val;
  }

  va_end( ap );

  return sum;
}

int main()
{
  int stack_val = 5;
  int stack_val2 = 10;

  int * p1 = &stack_val;
  int * p2 = &stack_val2;

  //Doesn't mod/ref either param
  assign( 2, &p1, &p2 );

  //p1 is still intact, pointing to stack_val
  if ( p1 == &stack_val )
  {
    return 0;
  }
  return -1;
}
