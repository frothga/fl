#ifndef fl_math_h
#define fl_math_h


#include <cmath>


namespace std
{
  inline int sqrt (int a)
  {
	return (int) floorf (sqrtf ((float) a));
  }

  inline int pow (int a, int b)
  {
	return (int) floor (pow ((double) a, b));
  }

  inline float pow (int a, float b)
  {
	return powf ((float) a, b);
  }
}


#ifndef INFINITY
static union
{
  unsigned char c[4];
  float f;
} fl_infinity = {0, 0, 0x80, 0x7f};
#define INFINITY fl_infinity.f
#endif

#ifndef NAN
static union
{
  unsigned char c[4];
  float f;
} fl_nan = {0, 0, 0xc0, 0x7f};
#define NAN fl_nan.f
#endif


inline bool
issubnormal (float a)
{
  return    ((*(unsigned int *) &a) & 0x7F800000) == 0
	     && ((*(unsigned int *) &a) &   0x7FFFFF) != 0;
}

inline bool
issubnormal (double a)
{
  return    ((*(unsigned long long *) &a) & 0x7FF0000000000000ll) == 0
	     && ((*(unsigned long long *) &a) &    0xFFFFFFFFFFFFFll) != 0;
}


#endif
