#include "sysfun.h"

int main() {
   int* a;
   int* b;
   int* c[2];
   a = malloc(sizeof(int)*2);

   *a = 10;
   *(a+1) = 20;
   c[0] = a;
   c[1] = a+1;

   print(*c[0]);
   print(*c[1]);
}
