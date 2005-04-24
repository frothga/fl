/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class AbsoluteValue --------------------------------------------------------

Image
AbsoluteValue::filter (const Image & image)
{
  if (*image.format == GrayFloat)
  {
	Image result (image.width, image.height, GrayFloat);
	result.timestamp = image.timestamp;

	float * toPixel   = (float *) result.buffer;
	float * fromPixel = (float *) image.buffer;
	float * end       = fromPixel + (image.width * image.height);

	while (fromPixel < end)
	{
	  *toPixel++ = fabsf (*fromPixel++);
	}

	return result;
  }
  else if (*image.format == GrayDouble)
  {
	Image result (image.width, image.height, GrayDouble);
	result.timestamp = image.timestamp;

	double * toPixel   = (double *) result.buffer;
	double * fromPixel = (double *) image.buffer;
	double * end       = fromPixel + (image.width * image.height);

	while (fromPixel < end)
	{
	  *toPixel++ = fabs (*fromPixel++);
	}

	return result;
  }

  // Ignore all other formats silently, since they (generally) don't have
  // negative values.
  // TODO: RGBAFloat may need attention.
  return image;
}
