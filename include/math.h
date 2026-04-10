/*
 * include/math.h — bare-metal stub for N64 port
 * Implementations live in src/n64/libc_impl.c
 */
#ifndef MATH_H
#define MATH_H

double sqrt (double x);
float  sqrtf(float x);
double sin  (double x);
double cos  (double x);
float  sinf (float x);
float  cosf (float x);
double fabs (double x);
float  fabsf(float x);
double atan2 (double y, double x);
float  atan2f(float y, float x);

#endif /* MATH_H */
