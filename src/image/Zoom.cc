/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Zoom -----------------------------------------------------------------

Zoom::Zoom (int scaleX, int scaleY)
{
  this->scaleX = scaleX;
  this->scaleY = scaleY;
}

Image
Zoom::filter (const Image & image)
{
  Image result (image.width * scaleX, image.height * scaleY, *image.format);

  int sy = 0;
  for (int y = 0; y < image.height; y++)
  {
	int sx = 0;
	for (int x = 0; x < image.width; x++)
	{
	  unsigned int rgba = image.getRGBA (x, y);

	  for (int v = 0; v < scaleY; v++)
	  {
		for (int u = 0; u < scaleX; u++)
		{
		  result.setRGBA (sx + u, sy + v, rgba);
		}
	  }
	  sx += scaleX;
	}
	sy += scaleY;
  }

  return result;
}
