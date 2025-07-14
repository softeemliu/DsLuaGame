#include "endian.h"


// 检查系统是否为小端（Windows 默认是小端）
static int is_little_endian() {
	const uint16_t val = 0x0001;
	return *(const uint8_t*)&val == 0x01;
}

// 通用的字节序交换函数（不依赖具体类型）
static void swap_bytes(void* value, size_t size) {
	uint8_t* bytes = (uint8_t*)value;
	for (size_t i = 0; i < size / 2; i++) {
		uint8_t tmp = bytes[i];
		bytes[i] = bytes[size - 1 - i];
		bytes[size - 1 - i] = tmp;
	}
}

//int
int endian_int(int s)
{
// 	if (!is_little_endian()) {
// 		swap_bytes(&s, sizeof(s));
// 	}
// 	return s;
#ifdef DSLITTLE_ENDIAN
	return s;
#else
	int v1;
	byte_t* p = (byte_t*)&s + sizeof(int) - 1;
	byte_t* q = (byte_t*)&v1;
	switch (sizeof(s))
	{
	case 8:
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
	case 4:
		*q++ = *p--;
		*q++ = *p--;
	case 2:
		*q++ = *p--;
	case 1:
		*q = *p;
		break;
	default:
		break;
	}
	return v1;
#endif
}

//short
short endian_short(short s)
{
#ifdef DSLITTLE_ENDIAN
	return s;
#else
	short v1;
	byte_t* p = (byte_t*)&s + sizeof(short) - 1;
	byte_t* q = (byte_t*)&v1;
	switch (sizeof(s))
	{
	case 8:
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
	case 4:
		*q++ = *p--;
		*q++ = *p--;
	case 2:
		*q++ = *p--;
	case 1:
		*q = *p;
		break;
	default:
		assert(false);
		break;
	}
	return v1;
#endif
}

//float
float endian_float(float s)
{
#ifdef DSLITTLE_ENDIAN
	return s;
#else
	float v1;
	byte_t* p = (byte_t*)&s + sizeof(float) - 1;
	byte_t* q = (byte_t*)&v1;
	switch (sizeof(s))
	{
	case 8:
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
	case 4:
		*q++ = *p--;
		*q++ = *p--;
	case 2:
		*q++ = *p--;
	case 1:
		*q = *p;
		break;
	default:
		assert(false);
		break;
	}
	return v1;
#endif
}

//double
double endian_double(double s)
{
#ifdef DSLITTLE_ENDIAN
	return s;
#else
	double v1;
	byte_t* p = (byte_t*)&s + sizeof(double) - 1;
	byte_t* q = (byte_t*)&v1;
	switch (sizeof(s))
	{
	case 8:
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
	case 4:
		*q++ = *p--;
		*q++ = *p--;
	case 2:
		*q++ = *p--;
	case 1:
		*q = *p;
		break;
	default:
		assert(false);
		break;
	}
	return v1;
#endif
}

//long64_t
long64_t endian_long64(long64_t s)
{
#ifdef DSLITTLE_ENDIAN
	return s;
#else
	long64_t v1;
	byte_t* p = (byte_t*)&s + sizeof(long64_t) - 1;
	byte_t* q = (byte_t*)&v1;
	switch (sizeof(s))
	{
	case 8:
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
	case 4:
		*q++ = *p--;
		*q++ = *p--;
	case 2:
		*q++ = *p--;
	case 1:
		*q = *p;
		break;
	default:
		assert(false);
		break;
	}
	return v1;
#endif
}

//uint64_t
uint64_t endian_uint64(uint64_t s)
{
#ifdef DSLITTLE_ENDIAN
	return s;
#else
	uint64_t v1;
	byte_t* p = (byte_t*)&s + sizeof(uint64_t) - 1;
	byte_t* q = (byte_t*)&v1;
	switch (sizeof(s))
	{
	case 8:
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
		*q++ = *p--;
	case 4:
		*q++ = *p--;
		*q++ = *p--;
	case 2:
		*q++ = *p--;
	case 1:
		*q = *p;
		break;
	default:
		assert(false);
		break;
	}
	return v1;
#endif
}