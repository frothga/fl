/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_random_h
#define fl_random_h


#include <stdlib.h>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  // Random number in [0, 1.0]
  inline float
  randf ()
  {
	return (float) rand () / RAND_MAX;
  }

  // Random number in [-1.0, 1.0]
  // "b" stands for "biased".
  inline float
  randfb ()
  {
	return 2.0f * rand () / RAND_MAX - 1.0f;
  }

  // Random number with a Gaussian distribution, mean of zero, and standard
  // deviation of 1.  Note that the range of this function is [-inf, inf].
  SHARED float randGaussian ();
}


#endif
