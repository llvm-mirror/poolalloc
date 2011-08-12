// Go through at least one of every operation to verify flags are set appropriately...

//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

//H, S, G, R, M

#include <stdlib.h>

//Not touched
int global_a;
//M
int global_b;
//R
int global_c;
//M/R
int global_d;

void func() {
  //Don't mod/ref
  int stack_a;
  //Mod
  int * heap_a = malloc(sizeof(int));

  //Mod
  int stack_b;
  int * heap_b = malloc(sizeof(int));

  //Ref
  int stack_c;
  int * heap_c = malloc(sizeof(int));

  //Mod/Ref
  int stack_d;
  int * heap_d = malloc(sizeof(int));

  //Mod the b's, ref the c's
  stack_b = stack_c;
  *heap_b = *heap_c;
  global_b = global_c;

  //Mod/ref all the d's
  stack_d = global_d;
  global_d = *heap_d;
  *heap_d = stack_d;

  free(heap_a);
  free(heap_b);
  free(heap_c);
  free(heap_d);
}

