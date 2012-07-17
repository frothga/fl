/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class IntensityStatistics --------------------------------------------------

IntensityStatistics::IntensityStatistics (bool ignoreZeros)
: ignoreZeros (ignoreZeros)
{
  average       = 0;
  averageSquare = 0;
  count         = 0;
  minimum       =  INFINITY;
  maximum       = -INFINITY;
}

Image
IntensityStatistics::filter (const Image & image)
{
  if (*image.format != GrayChar  &&  *image.format != GrayFloat  &&  *image.format != GrayDouble)
  {
	return filter (image * GrayFloat);
  }

  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "IntensityStatistics can only handle packed buffers for now";

  register double s1      = 0;
  register double s2      = 0;
  register int    n       = image.width * image.height;
  register double minimum =  INFINITY;
  register double maximum = -INFINITY;

  #define addup(size) \
  { \
	size * pixel = (size *) imageBuffer->base ();	\
	size * end   = pixel + n; \
	int    step  = imageBuffer->stride - image.width * sizeof (size); \
	if (ignoreZeros) \
	{ \
	  n = 0; \
	  while (pixel < end) \
	  { \
		size * rowEnd = pixel + image.width; \
		while (pixel < rowEnd) \
		{ \
		  register double t = *pixel++; \
		  if (t) \
		  { \
			s1 += t; \
			s2 += t * t; \
			minimum = min (minimum, t); \
			maximum = max (maximum, t); \
			n++; \
		  } \
		} \
		pixel = (size *) ((char *) pixel + step); \
	  } \
	} \
	else \
	{ \
	  while (pixel < end) \
	  { \
		size * rowEnd = pixel + image.width; \
		while (pixel < rowEnd) \
		{ \
		  register double t = *pixel++; \
		  s1 += t; \
		  s2 += t * t; \
		  minimum = min (minimum, t); \
		  maximum = max (maximum, t); \
		} \
		pixel = (size *) ((char *) pixel + step); \
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

  average       = s1 / n;
  averageSquare = s2 / n;
  count         = n;
  this->minimum = minimum;
  this->maximum = maximum;

  return image;
}

double
IntensityStatistics::deviation (double average)
{
  if (isnan (average)) average = this->average;
  return sqrt (averageSquare - average * average);
}
