//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>
int* test1(int*);

int* test() {
  int* a2 = (int*)malloc(sizeof(int));
  *a2 = 10;
  
  if(*a2 > 5 ) {
    return test();
  }    
  
  int *b2 = (int*)malloc(sizeof(int));
  return test1(b2); 
    
}

int*test1(int *b3) {
  return test()+*b3; 
}

int *test2(int *b4) {
  return test() + *b4;
}
void func() {

  int *a1 = test();
  int *b1 = test();
}

