
//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>
struct StructType1 {
  int a1[10];
  short b1[10];
  int c1[10];
};
struct StructType2 {
  int a2[10];
  int b2[10];
  int c2[10];
};
union UnionType {
  struct StructType1 s1;  
  struct StructType2 s2;  
};

void func() {

  union UnionType obj;
  union UnionType obj_copy;
  int i;
  for(i=0;i<10;i++) {
    obj.s1.a1[i] = i + 10;
    obj.s1.b1[i] = i + 32;
    obj.s1.c1[i] = i + 64;
  }  
  for(i=0;i<10;i++) {
    obj_copy.s2.a2[i] = obj.s1.a1[i];
    obj_copy.s2.b2[i] = obj.s1.b1[i];
    obj_copy.s2.c2[i] = obj.s1.c1[i];
  }  
}

