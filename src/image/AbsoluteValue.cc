/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2 and 1.4  Copyright 2005 Sandia Corporation.
Revisions 1.6 thru 1.8 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.7  2006/03/20 05:21:06  Fred
Image now has null PixelBuffer if it is empty, so trap this case.

Revision 1.6  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.5  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/09 04:07:33  Fred
Add detail to revision history.

Revision 1.3  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/22 21:04:09  Fred
MSVC compilability fix:  Add missing return value.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
