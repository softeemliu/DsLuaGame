#ifndef __ENDIAN_H__
#define __ENDIAN_H__
#include "public.h"

//int
int endian_int(int s);
//short
short endian_short(short s);
//float
float endian_float(float s);
//double
double endian_double(double s);
//long64_t
long64_t endian_long64(long64_t s);
//uint64_t
uint64_t endian_uint64(uint64_t s);

#endif /*__ENDIAN_H__*/