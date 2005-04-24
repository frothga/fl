/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
*/


#ifndef random_h
#define random_h


#include <stdlib.h>


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
  float randGaussian ();
}


#endif
