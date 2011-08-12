/*
 * Build into bitcode
 * RUN: clang -O0 %s -emit-llvm -c -o %t.bc
 * RUN: adsaopt -internalize -mem2reg -typechecks %t.bc -o %t.tc.bc
 * RUN: tc-link %t.tc.bc -o %t.tc1.bc
 * RUN: llc %t.tc1.bc -o %t.tc1.s
 * RUN: clang++ %t.tc1.s -o %t.tc2
 * Execute
 * RUN: %t.tc2 >& %t.tc.out
 * RUN: grep "Type.*mismatch" %t.tc.out
 */

int first_ones[65536];

int FirstOne(unsigned long arg1)
{
  union doub {
    unsigned short i[4];
    unsigned long  d;
  };
  union doub x;
  x.d=arg1;
  if (x.i[3])
    return (first_ones[x.i[3]]);
  if (x.i[2])
    return (first_ones[x.i[2]]+16);
  if (x.i[1])
    return (first_ones[x.i[1]]+32);
  if (x.i[0])
    return (first_ones[x.i[0]]+48);
  return(64);
}

int main() {
  int a = FirstOne(12345678);
  return 0;
}
