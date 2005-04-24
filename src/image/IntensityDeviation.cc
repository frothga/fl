/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class IntensityDeviation ---------------------------------------------------

IntensityDeviation::IntensityDeviation (float average, bool ignoreZeros)
{
  this->average = average;
  this->ignoreZeros = ignoreZeros;
}

Image
IntensityDeviation::filter (const Image & image)
{
  float variance = 0;
  int count = 0;

  #define addup(size) \
  { \
	size * pixel = (size *) image.buffer; \
	size * end   = pixel + image.width * image.height; \
	if (ignoreZeros) \
	{ \
	  while (pixel < end) \
	  { \
		if (*pixel != 0) \
		{ \
		  float t = *pixel - average; \
		  variance += t * t; \
		  count++; \
		} \
		pixel++; \
	  } \
	} \
	else \
	{ \
	  while (pixel < end) \
	  { \
		float t = *pixel++ - average; \
		variance += t * t; \
	  } \
	  count = image.width * image.height; \
	} \
  }

  if (*image.format == GrayFloat)
  {
	addup (float);
  }
  else if (*image.format == GrayDouble)
  {
	addup (double);
  }
  else if (*image.format == GrayChar)
  {
	addup (unsigned char);
  }
  else
  {
	filter (image * GrayFloat);
  }

  variance /= count;
  deviation = sqrtf (variance);

  return image;
}
