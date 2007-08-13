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
Revision 1.7  2007/08/13 03:19:26  Fred
Treat depth as a float value.

Revision 1.6  2007/03/23 02:32:06  Fred
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

Revision 1.2  2003/07/13 19:24:50  rothgang
Update to handle 3-byte formats.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"
#include "fl/pi.h"


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
