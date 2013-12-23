/**
	A minimal replacement for inttypes.h, to make up for deficiency in MS Visual Studio.
**/


#ifndef inttypes_h
#define inttypes_h

#include <stdint.h>

#define PRIx64 "I64x"
#define PRId64 "I64d"
#define PRIi64 "I64i"
#define PRIu64 "I64u"

#endif
