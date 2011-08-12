//Test that DSA doesn't crash when processing an indirect call to an external function.
//RUN: clang -S -emit-llvm -c %s -o %t.bc
//RUN: dsaopt %t.bc -dsa-td -disable-output

extern void func(void);

int main() {
  void (*fp)(void) = func;
  fp();
  return 0;
}

