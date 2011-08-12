//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

#include <stdio.h>

int * globalptr = NULL;

void externallyVisible(int * ptr)
{
  globalptr = ptr;
}

void usesGlobalPtr()
{
  int *ptr = globalptr;
}

int main()
{
  int stack = 1;
  externallyVisible(&stack);
  usesGlobalPtr();
  return 0;
}
