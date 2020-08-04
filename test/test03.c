#include "sysfun.h"

int main() {
   int a;
   int b = 10;
   a = 10;
   if (a == 10) {
     b = 20;
   } else {
     b = 0;
   }
   print(a*b);
}
