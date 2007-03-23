/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 thru 1.7 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.6  2006/03/20 05:33:36  Fred
Fix identification of class in exception text.

Revision 1.5  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.4  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.2  2003/08/06 22:20:49  rothgang
Compute minimum, maximum, and count, as well as average.

Revision 1.1  2003/07/30 14:11:11  rothgang
Added filters for finding basic statistics on images.
-------------------------------------------------------------------------------
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
