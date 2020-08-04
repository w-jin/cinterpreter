#include "sysfun.h"


void swap(int *a, int *b) {
   int temp;
   temp = *a;
   *a = *b;
   *b = temp;
}

int main() {
   int* a; 
   int* b;
   a = (int *)malloc(sizeof(int));
   b = (int *)malloc(sizeof(int *));
   
   *b = 24;
   *a = 42;

   swap(a, b);

   print(*a);
   print(*b);
   free(a);
   free(b);
   return 0;
}

