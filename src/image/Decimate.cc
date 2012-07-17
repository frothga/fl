/*
Author: Fred Rothganger
Created 12/22/2011


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Decimate -------------------------------------------------------------

Decimate::Decimate (int ratioX, int ratioY)
: ratioX (ratioX),
  ratioY (ratioY)
{
}

Image
Decimate::filter (const Image & image)
{
  const int ratioY = this->ratioY ? this->ratioY : ratioX;

  Image source = image * GrayFloat;
  Image result (source.width / ratioX, source.height / ratioY, GrayFloat);

  PixelBufferPacked * pbpSource = (PixelBufferPacked *) source.buffer;
  PixelBufferPacked * pbpResult = (PixelBufferPacked *) result.buffer;
  float * fromPixel = (float *) pbpSource->base ();
  float * toPixel   = (float *) pbpResult->base ();
  float * end       = toPixel + result.width * result.height;
  int ratioStep = pbpSource->stride * ratioY;

  if (ratioY > 2) fromPixel = (float *) ((char *) fromPixel + ratioY / 2 * pbpSource->stride);
  if (ratioX > 2) fromPixel += ratioX / 2;

  while (toPixel < end)
  {
	float * nextRow = (float *) ((char *) fromPixel + ratioStep);
	float * rowEnd  =                     toPixel   + result.width;  // result is guaranteed to be dense
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel;
	  fromPixel += ratioX;
	}
	fromPixel = nextRow;
  }

  return result;
}
