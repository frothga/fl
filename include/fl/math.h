/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_math_h
#define fl_math_h


#include <cmath>

#ifdef _MSC_VER
#  include <float.h>
#  undef min
#  undef max
#endif


namespace std
{
  inline int
  sqrt (int a)
  {
	return (int) floorf (sqrtf ((float) a));
  }

  inline float
  pow (int a, float b)
  {
	return powf ((float) a, b);
  }

#if ! defined (_MSC_VER)  ||  _MSC_VER > 1310
  inline int
  pow (int a, int b)
  {
	return (int) floor (pow ((double) a, b));
  }
#endif

#ifndef _MSC_VER

  inline int
  isinf (float a)
  {
	return isinff (a);
  }

  inline int
  isnan (float a)
  {
	return isnanf (a);
  }

#else

  inline float
  rint (float a)
  {
	return floorf (a + 0.5f);
  }

  inline double
  rint (double a)
  {
	return floor (a + 0.5);
  }

  inline int
  isnan (double a)
  {
	return _isnan (a);
  }

  inline int
  isinf (double a)
  {
	int c = _fpclass (a);
	return c == _FPCLASS_PINF  ||  c == _FPCLASS_NINF;
  }

#endif

  /**
	 Four-way max.  Used mainly for finding limits of a set of four points in the plane.
   **/
  template<class T>
  inline T
  max (T a, T b, T c, T d)
  {
	return max (a, max (b, max (c, d)));
  }

  /**
	 Four-way min.  Used mainly for finding limits of a set of four points in the plane.
   **/
  template<class T>
  inline T
  min (T a, T b, T c, T d)
  {
	return min (a, min (b, min (c, d)));
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
  #ifdef _MSC_VER
	int c = _fpclass (a);
	return c == _FPCLASS_ND  ||  c == _FPCLASS_PD;
  #else
	return    ((*(unsigned long long *) &a) & 0x7FF0000000000000ll) == 0
	       && ((*(unsigned long long *) &a) &    0xFFFFFFFFFFFFFll) != 0;
  #endif
}


#endif
