// A, B, func are in an SCC
// C is called indirectly from A/B
// The indirect call site in C is resolved, once we process the SCC
// But since, we never process the SCC graph again, 
// we never inline func into the SCC graph(of A, B, and func) at that
// call site.

// when we inline the SCCGraph into D(due to call to A), the unresolved
// call site is also inlined. As it is complete and can be resolved
// we inline the SCC graph again, pulling in the unresolved call site
// again. This causes an infinte looping in BU.

//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

typedef int* (*funcptr)(int *);

static int* A(void);

static int* func(int * arg) {
  A();
  return arg;
}

static int* C(funcptr f, int *arg) {
  (*f)(arg);
}

static int *B() {
  func(A());
  return C(func, A());
}

static int* A() {
  return func(B());
}

static int* D() {
  return A();
}
