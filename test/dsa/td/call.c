//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>


static int* test() {
  int* a2 = (int*)malloc(sizeof(int));
  *a2 = 10;
  
  if(*a2 > 5 ) {
    return a2; 
  }    
  
  int *b2 = (int*)malloc(sizeof(int));
  return b2; 
    
}

void func() {

  int *a1 = test();
  int *b1 = test();
}

