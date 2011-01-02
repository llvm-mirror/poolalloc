// A, B, C, func are in an SCC
// C is called indirectly from B
// The indirect call site in C is resolved, once we inline C into B.

// But since, B and C are in the same graph, inlining C with B, 
// merges the arguments for C with the actual arguments passed in 
// from B. This makes the function pointer being called to be 
// marked incomplete(coming from argument).

// when we inline the SCCGraph into D(due to call to A), the unresolved
// call site is also inlined. As it is complete and can be resolved
// we inline the SCC graph again, pulling in the unresolved call site
// again. This causes an infinte looping in BU.
// XFAIL:*
// RUN: not
#include<stdio.h>

typedef int* (*funcptr)(int *);

static int* A(void);
static int* B(void);
static int* func(int*);
static int* C(funcptr f, int *arg) ;

static int* func(int * arg) {
  A();
  return C(NULL, arg);
}

static int* C(funcptr f, int *arg) {
  A();
  func(B());
  (*f)(arg);
}

static int *B() {
  func(A());
  return C(func, A());
}

static int* A() {
  return C(NULL,func(B()));
}

static int* D() {
  return A();
}
