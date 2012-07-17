/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class AbsoluteValue --------------------------------------------------------

Image
AbsoluteValue::filter (const Image & image)
{
  if (image.width == 0  ||  image.height == 0) return image;
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "AbsoluteValue can only handle packed buffers for now";

  if (*image.format == GrayFloat)
  {
	Image result (image.width, image.height, GrayFloat);
	result.timestamp = image.timestamp;

	float * toPixel   = (float *) ((PixelBufferPacked *) result.buffer)->base ();
	float * fromPixel = (float *) imageBuffer->base ();
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

	double * toPixel   = (double *) ((PixelBufferPacked *) result.buffer)->base ();
	double * fromPixel = (double *) imageBuffer->base ();
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
