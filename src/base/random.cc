#include "fl/random.h"

#include <math.h>


using namespace fl;


static bool haveNextGaussian = false;
static float nextGaussian;

float
fl::randGaussian ()
{
  if (haveNextGaussian)
  {
	haveNextGaussian = false;
	return nextGaussian;
  }
  else
  {
	float v1, v2, s;
	do
	{ 
	  v1 = randfb ();   // between -1.0 and 1.0
	  v2 = randfb ();
	  s = v1 * v1 + v2 * v2;
	}
	while (s >= 1 || s == 0);
	float multiplier = sqrt (- 2 * log (s) / s);
	nextGaussian = v2 * multiplier;
	haveNextGaussian = true;
	return v1 * multiplier;
  }
}
