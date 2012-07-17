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


using namespace fl;
using namespace std;


// class Rescale --------------------------------------------------------------

Rescale::Rescale (double a, double b)
{
  this->a = a;
  this->b = b;
}

Rescale::Rescale (const Image & image, bool useFullRange)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer)
  {
	// We are only interested in gray pixel types, but other types are
	// not an error.  Therefore, set default values
	a = 1;
	b = 0;
	return;
  }
  void * start = (void *) imageBuffer->base ();

  double lo = INFINITY;
  double hi = -INFINITY;

  if (*image.format == GrayFloat)
  {
	float * i   = (float *) start;
	float * end = i + image.width * image.height;
	while (i < end)
	{
	  hi = max ((double) *i,   hi);
	  lo = min ((double) *i++, lo);
	}
  }
  else if (*image.format == GrayDouble)
  {
	double * i   = (double *) start;
	double * end = i + image.width * image.height;
	while (i < end)
	{
	  hi = max (*i,   hi);
	  lo = min (*i++, lo);
	}
  }
  else
  {
	a = 1;
	b = 0;
	return;
  }

  if (! useFullRange  &&  hi <= 1)
  {
	if (lo >= 0)
	{
	  a = 1;
	  b = 0;
	  return;
	}
	else if (lo >= -1)
	{
	  a = 0.5;
	  b = 0.5;
	  return;
	}
  }

  a = 1.0 / (hi - lo);
  b = -lo / (hi - lo);
}

Image
Rescale::filter (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) return Image (image);
  void * start = (void *) imageBuffer->base ();

  if (*image.format == GrayFloat)
  {
	Image result (image.width, image.height, *image.format);
	float * r = (float *) ((PixelBufferPacked *) result.buffer)->base ();
	float * i = (float *) start;
	float * end = i + image.width * image.height;
	while (i < end)
	{
	  *r++ = *i++ * a + b;
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	Image result (image.width, image.height, *image.format);
	double * r = (double *) ((PixelBufferPacked *) result.buffer)->base ();
	double * i = (double *) start;
	double * end = i + image.width * image.height;
	while (i < end)
	{
	  *r++ = *i++ * a + b;
	}
	return result;
  }
  else
  {
	return Image (image);
  }
}
