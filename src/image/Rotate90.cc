/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.5 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/08/13 03:21:08  Fred
Treat depth as a float value.

Revision 1.4  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/13 19:26:22  rothgang
Create class to handle 90 degree rotations directly (without use of
interpolation, as in the Transform class).
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Rotate90 ------------------------------------------------------------

Rotate90::Rotate90 (bool clockwise)
{
  this->clockwise = clockwise;
}

struct triad
{
  unsigned char channel[3];
};

Image
Rotate90::filter (const Image & image)
{
  #define transfer(size) \
  { \
	ImageOf<size> input = image; \
	ImageOf<size> result (image.height, image.width, *image.format); \
	if (clockwise) \
	{ \
	  for (int y = 0; y < result.height; y++) \
	  { \
		for (int x = 0; x < result.width; x++) \
		{ \
		  result (x, y) = input (input.width - y - 1, x); \
		} \
	  } \
	} \
	else \
	{ \
	  for (int y = 0; y < result.height; y++) \
	  { \
		for (int x = 0; x < result.width; x++) \
		{ \
		  result (x, y) = input (y, input.height - x - 1); \
		} \
	  } \
	} \
    return result; \
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
}
