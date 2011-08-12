
//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>

struct StructType {

  int a;
  int *b;
};

void func() {

  int *tmp = (int*) malloc(sizeof(int));
  struct StructType s1;
  s1.a = 10;
  s1.b = tmp;

  int *c = &s1.a;
  struct StructType s2 = s1;
}

