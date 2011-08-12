
//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output
#include <stdlib.h>

void func() {
  int a = 10;
  int a1 = 20;
  int *b = &a;
  int *b1 = &a1;
  int **c;
  if( a > a1) {
    c = &b1;
  } else {
    c = &b;
  } 
  int *d = *c;
  int e = *d;
}

