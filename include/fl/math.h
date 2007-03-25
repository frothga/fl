/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2, 1.3, 1.5 Copyright 2005 Sandia Corporation.
Revisions 1.7 thru 1.9  Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/25 13:31:09  Fred
Fix MSVC 2005 compile errors.  It doesn't define pow(int,int).

Revision 1.8  2007/03/23 11:38:05  Fred
Correct which revisions are under Sandia copyright.

Revision 1.7  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/08 19:42:13  Fred
Update revision history and add Sandia copyright notice.

Move isinf() and isnan() into std namespace.

Undefine min and max macros created by some subsidiary of windows.h

Revision 1.4  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.3  2005/01/22 20:44:27  Fred
MSVC compilability fix

Revision 1.2  2005/01/12 04:57:58  rothgang
Add functions (sqrt, pow) to the std namespace to ensure that all common
numeric types are covered.

Adapt to environments (such as cygwin) that lack a definition of INFINITY, NAN,
and fpclassify.

Revision 1.1  2004/05/06 16:25:11  rothgang
Add functions to force use of "f" forms of math functions for floats rather
than converting up to double.
-------------------------------------------------------------------------------
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

  inline int
  pow (int a, int b)
  {
	return (int) floor (pow ((double) a, b));
  }

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
