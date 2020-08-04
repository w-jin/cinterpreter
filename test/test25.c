#include "sysfun.h"

int main() {
    int n = get();
    int f0 = 1, f1 = 1;
    int f2;
    int i = 2;
    while (i <= n) {
        f2 = f0 + f1;
        f0 = f1;
        f1 = f2;
        i = i + 1;
    }

    print(f1);
}
