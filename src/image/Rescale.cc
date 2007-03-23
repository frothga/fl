/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 thru 1.6 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.5  2006/02/25 22:38:32  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.4  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.2  2003/12/29 23:35:09  rothgang
Make default behavior of Rescale to use the full range [0,1].

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
  if (! imageBuffer) throw "Rescale only handles packed buffers for now";
  void * start = (void *) imageBuffer->memory;

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
  if (! imageBuffer) throw "Rescale only handles packed buffers for now";
  void * start = (void *) imageBuffer->memory;

  if (*image.format == GrayFloat)
  {
	Image result (image.width, image.height, *image.format);
	float * r = (float *) ((PixelBufferPacked *) result.buffer)->memory;
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
	double * r = (double *) ((PixelBufferPacked *) result.buffer)->memory;
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
