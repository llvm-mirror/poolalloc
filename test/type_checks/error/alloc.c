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
#include<stdlib.h>

void **alloc_matrix (int nrmin, int nrmax, int ncmin, int ncmax,
                     size_t elsize) {


  int i;
  char **cptr;

  cptr = (char **) malloc ((nrmax - nrmin + 1) * sizeof (char *));
  cptr -= nrmin;
  for (i=nrmin;i<=nrmax;i++) {
    cptr[i] = (char *) malloc ((ncmax - ncmin + 1) * elsize);
    cptr[i] -= ncmin * elsize / sizeof(char);  /* sizeof(char) = 1 */
  }
  return ((void **) cptr);
}

int main() {
  int i;
  int j;
  int pins_per_clb = 5;
  int ** pinloc = (int **) alloc_matrix (0, 3, 0, pins_per_clb-1, sizeof (int));

   for (i=0;i<=3;i++)
     for (j=0;j<pins_per_clb;j++)  {
       pinloc[i][j] = 0;
     }

  return 0;
}
