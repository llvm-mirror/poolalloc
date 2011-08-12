//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

#include <stdio.h>
#include <stdlib.h>

typedef union {
        struct {
                signed int immed:16;
                unsigned int rt:5;
                unsigned int rs:5;
                unsigned int rf:5;
                unsigned int opcode:6;
        };
        unsigned int w;
} I_format_t;

 
int main()
{
        I_format_t *ia = (I_format_t *)malloc(sizeof(I_format_t));
        ia->w = 0xAFBE0010;
        printf("\ninstruction: %X\n",ia->w);
        printf("opcode: %X\n",ia->opcode);
        printf("rs: %d rt: %d rj: %d \n", ia->rs, ia->rt, ia->rf);
        printf("immed: %d\n",ia->immed);
        return 0;
}
