//--Make sure we can run DSA on it! 
//RUN: clang -Wno-format %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output

#include <stdio.h>

struct taxonomy {
	unsigned kingdom: 2;
	unsigned phylum: 4;
	unsigned genus: 12;
};

int main()
{
	struct taxonomy t = {0, 0, 21};
	t.kingdom = 1;
	t.phylum = 7;
	printf("sizeof(struct taxonomy): %d bytes\n",(int)sizeof(struct taxonomy));
	printf("taxonomy: 0x%x\n",t);
	printf("kingdom: %d\n",t.kingdom);
	printf("phylum: %d\n",t.phylum);
	printf("genus: %d\n",t.genus);
	
	return 0;
}

