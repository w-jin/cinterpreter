#include "sysfun.h"

int get(int *a) {
   return *a;
}

int main() {
   int* a; 
   int b;
   a = (int *) malloc(sizeof(int));
   *a = 42;   

   b = get(a);
   print(b);
   return 0;
}
