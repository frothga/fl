/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 thru 1.5 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.4  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.3  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/30 14:11:11  rothgang
Added filters for finding basic statistics on images.
-------------------------------------------------------------------------------
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
