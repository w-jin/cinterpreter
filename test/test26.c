#include "sysfun.h"

int a[10];

int main() {
    print(a[2]);
    print(a[4]);
    int *b[2];
    b[0] = a + 2;
    b[1] = a + 4;
    **b = 20;
    **(b + 1) = 40;
    print(a[2]);
    print(a[4]);
}
