/*
 * Four system built-in functions. The header just includes
 * their declarations without implements. Just include this
 * file and use them, and the interpreter will execute them
 * correctly.
 */

#ifndef SYSFUN_H
#define SYSFUN_H

extern int get();
extern void print(int);
extern void *malloc(int);
extern void free(void *);

#endif // ~SYSFUN_H
