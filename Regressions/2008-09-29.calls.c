#include <stdio.h>

struct OP {
 void (*func)(struct OP*);
};

void bar(struct OP *op);

void foo(struct OP *op) {
 printf("Foo\n");
 op->func = bar;
}

void bar(struct OP *op) {
 printf("Bar\n");
 op->func = foo;
}

int main(int argc, char **argv) {
 int i;
 struct OP op;
 op.func = foo;
 for(i = 0; i < 10; ++i) {
   op.func(&op);
 }
 return 0;
}

