#include "sysfun.h"

int main() {
   int a = 0;
   int b = 0;
   a = 0;

   for (a = 0; a < 10; a = a + 1) {
      b = b + 2;
   }
   print(b);
}
