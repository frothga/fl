#ifndef fl_pi_h
#define fl_pi_h


#include <math.h>

#define PI 3.1415926535897932384626433832795029


inline double
mod2pi (double a)
{
  a = fmod (a, 2.0 * PI);
  if (a < 0)
  {
	a += 2.0 * PI;
  }
  return a;
}

inline float
mod2pi (float a)
{
  a = fmodf (a, 2.0 * PI);
  if (a < 0)
  {
	a += 2.0 * PI;
  }
  return a;
}


#endif
