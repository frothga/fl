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
	return 2.0 * rand () / RAND_MAX - 1.0;
  }

  // Random number with a Gaussian distribution, mean of zero, and standard
  // deviation of 1.  Note that the range of this function is [-inf, inf].
  float randGaussian ();
}


#endif
