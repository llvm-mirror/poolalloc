//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>

int* test(int **a2, int **b2) {
  int *temp;
  temp = *a2;
  a2 = b2;
  *b2 = temp;
    
  return *a2;
}

void func() {

  int* mem1 = (int *)malloc(sizeof(int));
  int* mem2 = (int *)malloc(sizeof(int));
  int *a1 = test(&mem1, &mem2);
  int *b1 = test(&mem1, &mem2);
}

