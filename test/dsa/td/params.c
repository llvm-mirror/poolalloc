//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>

int* test(int **a2, int **b2) {
  int *temp = *a2;
  int *temp1 = *b2;
  if(*temp1 < *temp) {
    return *b2;
  }
  else {
    int *temp2 = (int*)malloc(sizeof(int));
    a2 = &temp2;
  }
    
  return *a2;
}

void func() {

  int* mem1;
  int* mem2;
  int r1 = 5;
  int r2 = 6;
  mem1 = &r1;
  mem2 = &r2;
  int *a1 = test(&mem1, &mem2);
  int *b1 = test(&mem2, &mem1);
}

