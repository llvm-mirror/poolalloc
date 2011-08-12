//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>
struct NestedStructType {

  float a1;
  int *b1;
};
struct StructType {

  float a2;
  int *b2;
  struct NestedStructType ns2;
};
union UnionType {

  int a;
  int *b;
  int c[100];
  struct StructType obj;  
  
};

void func() {

  int *tmp = (int*) malloc(sizeof(int));
  union UnionType s1;
  
  s1.b = tmp;

  int *c = s1.b;
  int d = s1.a;
  int arr = s1.c[0];
  struct StructType x = s1.obj;
  float y = x.a2;
}

