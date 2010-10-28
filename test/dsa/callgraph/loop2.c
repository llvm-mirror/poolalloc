#include<stdio.h>
#include<stdlib.h>

typedef int* (*funcptr)(int *);
typedef int* (*funcptr2)(void *);

funcptr FP;

int * B() {
}
int * A() {
}
void D(funcptr f) {
  f = B;
} 
int * SetFP(void * f){
  D(FP);
}

int * init() {
  FP = A;
  funcptr2 setter = SetFP;
  (*setter)(malloc(3));
  (*FP)(malloc(sizeof(int)));
  return (*FP)(malloc(sizeof(int)));
}

int main() {
  init();
  
}
