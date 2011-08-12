//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

static int * G = 0;

typedef void (*fp)(int **);

extern void external(fp f);

static void C(int ** a) {
  // Make the argument point to the global.
  // Eventually, this link will make the global external because eventually, in TD,
  // our argument will be +E because we're externally callable.
  *a = G;
}

static void B(void) {
  // Pass something to C, doesn't really matter here.
  int a;
  int * ptr = &a;
  C(&ptr);
}

static void B2(void) {
  // This makes use of the global.
  // At somepoint, we should know that 'a' is external because
  // the global aliases the parameter and since C becomes externally callable
  int * a = G;
  int val = *a;
}

// Makes its argument external
static void externalize(fp f) {
  external(f);
}

static void A(void) {
  // Here we make 'f' externally callable, but we don't know that
  // Until BU is run.
  fp f = C;
  externalize(f);

  // Call tree...
  B();
  B2();
}

