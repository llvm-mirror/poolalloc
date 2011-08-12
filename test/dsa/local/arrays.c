
//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

//H, S, G, R, M

#include <stdlib.h>

void func() {

  int *arr = (int*) malloc(sizeof(int)*10);
  int i;
  for(i=0;i<10;i++)
    arr[i] = i;

  int *b = &arr[5];
  int **c = &arr;
  int **d = &arr + 4;
}

