//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>

struct StructType {

  int a;
  int *b;
};

void func() {

  struct StructType *tmp = (struct StructType*) malloc(sizeof(struct StructType)*10);
  int i;
  for(i=0;i<10;i++) {
    tmp[i].a = i;
    tmp[i].b = (int*) malloc(sizeof(int));
  }
  
  struct StructType s2 = tmp[1];
  int * c = s2.b;
  struct StructType *ptr = &s2;
  struct StructType *ptr1 = ptr + 1;
  
}

