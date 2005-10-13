/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
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
  Image result (image.width, image.height, *image.format);
  result.timestamp = image.timestamp;

  int pixels = image.width * image.height;

  #define transfer(size) \
  { \
	for (int i = 0; i < pixels; i++) \
    { \
	  ((size *) result.buffer)[i] = ((size *) image.buffer)[pixels - 1 - i]; \
	} \
  }
  switch (image.format->depth)
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
