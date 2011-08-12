
//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>
struct StructType1 {

  int a1;
  int b1;
  int c1;
};
struct StructType2 {

  int a2;
  short b2;
  int c2;
};

union UnionType {
  struct StructType1 s1;  
  struct StructType2 s2;  
};

void func() {

  union UnionType obj;
  
  obj.s1.a1 = 2l;
  obj.s1.b1 = 33;
  obj.s1.c1 = 22;
  
  struct StructType2 s = obj.s2;

  int t = obj.s2.c2;
  int t1 = obj.s1.c1;
}

