
//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output
#include <stdlib.h>

void func() {
  int a = 10;
  int *b = &a;
  int **c = &b;
  int *d = *c;
  int e = *d;
}

