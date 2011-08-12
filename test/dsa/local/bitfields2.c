//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

#include <stdio.h>
#include <stdlib.h>

typedef union {
	struct {
		unsigned int fn:6;
		unsigned int sh:5;
		unsigned int rd:5;
	};
	struct {
		signed int immed:16;
		unsigned int rt:5;
		unsigned int rs:5;
		unsigned int opcode:6;
	};
	unsigned int w;
} mips_format_t;


int main()
{
	int i;
	mips_format_t ia;
	unsigned int codes[] = {0x27bdffe8, 0xAFBE0010, 0x03A0F021, 0x2402000E};
	int n = sizeof(codes)/sizeof(int);
	for (i=0; i<n; i++) {
		ia.w = codes[i];
		printf("\ninstruction: %X\n",ia.w);
		printf("opcode: %X\n",ia.opcode);
		printf("rs: %d rt: %d\n", ia.rs, ia.rt);
		if (ia.opcode==0) {
			printf("rd: %d  sh: %d  fn: %X\n",ia.rd,ia.sh,ia.fn);
		}
		else {
			printf("immed: %d\n",ia.immed);
		}
	}
	return 0;
}

