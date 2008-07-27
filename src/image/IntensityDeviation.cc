/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
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
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "IntensityDeviation can only handle packed buffers for now";
  Pointer imageMemory = imageBuffer->memory;

  float variance = 0;
  int count = image.width * image.height;

  #define addup(size) \
  { \
	size * pixel = (size *) imageMemory; \
	size * end   = pixel + count; \
	if (ignoreZeros) \
	{ \
	  count = 0; \
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
