// similiar to scc3. But with 2 SCCs

//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


typedef int* (*funcptr)(int *);

static int* A1(void);
static int* A2(void);

static int* func1(int * arg) {
  A1();
  return arg;
}
static int* func2(int * arg) {
  A2();
  return arg;
}

static int* C1(funcptr f, int *arg) {
  (*f)(arg);
}
static int* C2(funcptr f, int *arg) {
  (*f)(arg);
}

static int *B1() {
  func1(A1());
  return C1(func2, A1());
}
static int *B2() {
  func2(A2());
  return C2(func1, A2());
}
static int* A1() {
  return func1(B1());
}

static int* A2() {
  return func2(B2());
}

static int* D() {
   A2();
  return A1();
}
