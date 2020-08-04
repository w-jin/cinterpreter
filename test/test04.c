#include "sysfun.h"

int main() {
   int a;
   int b = 10;
   a = -10;
   if (a > 0 ) {
     b = a;
   } else {
     b = -a;
   }
   print(b);
}
