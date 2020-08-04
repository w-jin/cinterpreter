#include "sysfun.h"

void set(int *a, int b) {
   int c;
   c = b + 1;
   *a = c;
}

int main() {
   int* a; 
   int b;
   a = (int *) malloc(sizeof(int));
   
   b = 10;
   set(a, b);
   print(*a);
   free(a);
   return 0;
}

