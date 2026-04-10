/*
 * include/stdlib.h — bare-metal stub for N64 port
 * Implementations live in src/n64/libc_impl.c
 */
#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void  free(void *p);
void  exit(int code) __attribute__((noreturn));

int   atoi(const char *s);
int   abs(int x);
long  labs(long x);
int   rand(void);
void  srand(unsigned int seed);

#endif /* STDLIB_H */
