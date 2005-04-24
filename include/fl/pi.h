/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
*/


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
  a = fmodf (a, (float) (2 * PI));
  if (a < 0)
  {
	a += (float) (2 * PI);
  }
  return a;
}


#endif
