//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

//H, S, G, R, M

#include <stdlib.h>

struct StructType {

  int a;
  int *b;
};

void func() {

  struct StructType tmp[10];
  int i;
  for(i=0;i<10;i++) {
    tmp[i].a = i;
  }
  
  struct StructType s2 = tmp[0];
  int * c = s2.b;
}

