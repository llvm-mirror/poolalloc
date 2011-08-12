//--Make sure we can run DSA on it! 
//RUN: clang %s -c -emit-llvm -o - |  \
//RUN: dsaopt -dsa-bu -dsa-td -disable-output


#include <stdlib.h>

struct InfoStruct { 
  int count;
  int valid;
  float factor;
};

void initialize(struct InfoStruct **arr, int size) {
  struct InfoStruct *temp = *arr;
  while(temp < (*arr + size)) {
    temp->count = 0;
    temp->valid = 0;
    temp->factor = 0.0;
    temp++;
  }
}

void process(struct InfoStruct **arr, int loc, int count, float fact) {

  struct InfoStruct *ptr = *arr;
  struct InfoStruct obj;
  obj.count = count;
  obj.factor = fact;
  obj.valid = 1;
  ptr[loc]=obj;
}

int main() {

  struct InfoStruct* InfoArray= (struct InfoStruct*)malloc(sizeof(struct InfoStruct) * 10);
  initialize(&InfoArray, 10);
  process(&InfoArray, 4, 3, 5.5);   
}
