#include "sysfun.h"

int  x(int y) {
	return y + 10;
}

int  f(int b) {
   return x(b) + 10;
}

int main() {
   int a;
   a = 10;

   a = f(a);
   print(a);
   return 0;
}

