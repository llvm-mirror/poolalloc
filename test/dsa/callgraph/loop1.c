#include<stdio.h>
#include<stdlib.h>

typedef int* (*funcptr)(int *);

funcptr FP;
struct S {
funcptr f;
};

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

static int * init() {
  FP = A;
  (*FP)(malloc(sizeof(int)));
  return (*FP)(malloc(sizeof(int)));
}
void init2(struct S *o){
  o->f = B;
}
static void init1() {
  struct S * t = malloc(sizeof(struct S));
  t->f = FP;
  init2(t);
  
}

int main() {
  init();
  init1();
}
