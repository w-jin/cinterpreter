#include "sysfun.h"

int f(int x) {
  int i = 0;
  while (i < x) {
     i = i + 2;
  }
  return i + 10;
}
int main() {
   int a;
   int b;
   a = 1;
   b = f(a);
   print(b);
}
