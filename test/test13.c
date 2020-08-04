#include "sysfun.h"

int b;
int f(int x) {
  return x + 10;
}
int main() {
   int a;
   int b;
   a = -10;
   if (a > 0) 
      b = f(a);
   else 
      b = f(-a);
   print(b);
}
