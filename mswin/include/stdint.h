/**
	A minimal replacement for stdint.h, to make up for deficiency in MS Visual Studio.
**/


#ifndef stdint_h
#define stdint_h

typedef signed char        int8_t;
typedef short              int16_t;
typedef long               int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned long      uint32_t;
typedef unsigned long long uint64_t;

#define INT64_C(x) x ## LL

#endif
