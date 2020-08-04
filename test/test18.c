#include "sysfun.h"

int main() {
   int* a;
   int **b;
   int *c;
   a = malloc(sizeof(int)*2);
   b = (int **)malloc(sizeof(int *));

   *b = a;
   *a = 10;
   *(a+1) = 20;

   c = *b;
   print(*c);
   print(*(c+1));
   free(a);
   free((int *)b);
}
