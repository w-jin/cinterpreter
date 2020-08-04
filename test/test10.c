#include "sysfun.h"

int main() {
   int a = 0;
   int b = 0;
   while (a < 10) {
      a = a + 1; 

      if (a > 5) b = b + 1;
   }
   print(b);
}
