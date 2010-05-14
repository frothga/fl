/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
