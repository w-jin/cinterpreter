CINTERPRETER is a simple c language interpreter based on clang.

## Build And Run It

The project is based on clang and llvm, therefore, before building it, you must download and install clang and llvm. The two softwares can usually be found in official repositories. Just like on ubuntu 18.04, you can install them by

```
sudo apt install libclang-dev llvm-dev
```

They are installed to the directories `/usr/lib/llvm-6.0/include` and `/usr/lib/llvm-6.0/lib`, which is the same as that I write in CMakeLists.txt and Makefile. The other version is most probably similar to 6.0.

After installing clang and llvm, building process is very simple:

```
git clone https://github.com/w-jin/cinterpreter.git
cd cinterpreter
make
```

Then you can look up a executable file named `cinterpreter`. If you prefer cmake, just follow the process:

```
git clone https://github.com/w-jin/cinterpreter.git
mkdir cinterpreter/build
cd cinterpreter/build
cmake ..
make
```

But you must take note of that the header file is located in project root directory, so you script must guarantee that include path is correct, for example, copying `sysfun.h` to `build`.

## Built-in Functions

There no support for third-party library, which may be considered in the future. There are just four built-in functions: `get`, `print`, `malloc`, and `free`, whose functions are reading an integer from standard input, writting an integer to standard output, allocating memory, and freeing memory.

To use these functions, you must include the header file `sysfun.h`, and its location must be the same as the current directory when you execute cinterpreter. An example is as follows.

```c
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
```

This program recieve an interger to `n` and output the `nth` Fibonacci number.

## Examples

There are some test cases in the directory test. They will tell you the supported syntaxes.

## TODO

It can not be executed as a repl and some syntaxes are not supported. I may be implement them in the future.