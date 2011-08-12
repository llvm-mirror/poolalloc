// Checks that structure field offsets are calculated correctly
// Checks that structure is folded

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
    tmp[i].b = &tmp[i].a;
  }
  
  struct StructType s2 = tmp[0];
  int * c = s2.b;
}

