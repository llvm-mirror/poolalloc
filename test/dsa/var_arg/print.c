#include <stdarg.h>
#include <stdio.h>

//--build the code into a .bc
//RUN: llvm-gcc -O0 %s -S --emit-llvm -o - | llvm-as > %t.bc
//--check if ds-aa breaks, breaks opts, or results in miscompiled code
//RUN: lli %t.bc > %t.refout
//RUN: dsaopt %t.bc -ds-aa -O3 -o - | lli > %t.out
//RUN: diff %t.refout %t.out
//--check properties of this particular test
//N/A

void generic_sendmsg (const char *fmt, ...)
{
  va_list ap;
  printf( "@");
  va_start(ap, fmt);
  vprintf( fmt, ap);
  va_end(ap);
  printf("\n");
}

void main() {
  int *x = malloc(sizeof(int));
  generic_sendmsg("F %li %li %3.2f %3.2f", 1234, 1234,123.22, 123.45);
  generic_sendmsg("%s ID3:%s%s", "TEST", "AAA" ,  "Unknown");
  generic_sendmsg("%s ID3:%s%s %p", "TEST", "AAA" ,  "Unknown", x);
}
