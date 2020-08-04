#include "sysfun.h"


int main() {
   int* a;
   int b;
   b = 10;
   
   a = malloc(sizeof(int));
   *a = b;
   print(*a);
   free(a);
}
