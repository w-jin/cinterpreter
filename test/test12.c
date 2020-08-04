#include "sysfun.h"

int main() {
	int a[3];
        int i;
	int b = 0;
        for (i=0; i<3; i = i + 1) {
           a[i] = i * i;
        }
        b = a[2];
	print(b);
}
