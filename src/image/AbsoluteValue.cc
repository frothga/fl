/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Add missing return value.
02/2006 Fred Rothganger -- Change Image structure.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class AbsoluteValue --------------------------------------------------------

Image
AbsoluteValue::filter (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "AbsoluteValue can only handle packed buffers for now";
  Pointer imageMemory = imageBuffer->memory;

  if (*image.format == GrayFloat)
  {
	Image result (image.width, image.height, GrayFloat);
	result.timestamp = image.timestamp;

	float * toPixel   = (float *) ((PixelBufferPacked *) result.buffer)->memory;
	float * fromPixel = (float *) imageMemory;
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

	double * toPixel   = (double *) ((PixelBufferPacked *) result.buffer)->memory;
	double * fromPixel = (double *) imageMemory;
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
