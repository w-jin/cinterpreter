#include "sysfun.h"

int main() {
   int a, b, c;
   a = 0;
   c = 0;
   while (a < 10) {
      a = a + 1;
      b = 0;
      while (b < 10) {
          b = b + 1;
          c = c + 1;
      }
   }
   print(c);
}
