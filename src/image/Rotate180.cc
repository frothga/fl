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


// class Rotate180 ------------------------------------------------------------

struct triad
{
  unsigned char channel[3];
};

Image
Rotate180::filter (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "Rotate180 can only handle packed buffers for now";

  Image result (image.width, image.height, *image.format);
  result.timestamp = image.timestamp;

  #define transfer(size) \
  { \
	size * dest   = (size *) ((PixelBufferPacked *) result.buffer)->memory; \
	size * end    = (size *) imageBuffer->memory; \
	size * source = end + (image.width * image.height - 1); \
	while (source >= end) \
	{ \
	  *dest++ = *source--; \
	} \
  }

  switch ((int) image.format->depth)
  {
    case 8:
	  transfer (double);
	  break;
    case 4:
	  transfer (unsigned int);
	  break;
    case 3:
	  transfer (triad);
	  break;
    case 2:
	  transfer (unsigned short);
	  break;
    case 1:
    default:
	  transfer (unsigned char);
  }

  return result;
}
