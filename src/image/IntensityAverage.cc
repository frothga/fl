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


// class IntensityAverage ---------------------------------------------------

IntensityAverage::IntensityAverage (bool ignoreZeros)
{
  this->ignoreZeros = ignoreZeros;

  average = 0;
  count = 0;
  minimum = INFINITY;
  maximum = -INFINITY;
}

Image
IntensityAverage::filter (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "IntensityAverage can only handle packed buffers for now";
  Pointer imageMemory = imageBuffer->memory;

  average = 0;
  count = image.width * image.height;
  minimum = INFINITY;
  maximum = -INFINITY;

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
		  minimum = min (minimum, (float) *pixel); \
		  maximum = max (maximum, (float) *pixel); \
		  average += *pixel; \
		  count++; \
		} \
		pixel++; \
	  } \
	} \
	else \
	{ \
	  while (pixel < end) \
	  { \
		minimum = min (minimum, (float) *pixel); \
		maximum = max (maximum, (float) *pixel); \
		average += *pixel++; \
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

  average /= count;

  return image;
}
