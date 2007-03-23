/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"


// For debugging
#include <iostream>
using namespace std;


using namespace fl;


// class ClearAlpha -----------------------------------------------------------

ClearAlpha::ClearAlpha (unsigned int color)
{
  this->color = color;
}

Image
ClearAlpha::filter (const Image & image)
{
  Image result (image.width, image.height, *image.format);

  //float rc = ((color &   0xFF0000) >> 16) / 255.0f;
  //float gc = ((color &     0xFF00) >>  8) / 255.0f;
  //float bc =  (color &       0xFF)        / 255.0f;

  Pixel c;
  c.setRGBA (color);

  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  /*
	  unsigned int rgb = image.getRGBA (x, y);
	  float rf    = ((rgb &   0xFF0000) >> 16) / 255.0f;
	  float gf    = ((rgb &     0xFF00) >>  8) / 255.0f;
	  float bf    =  (rgb &       0xFF)        / 255.0f;
	  float alpha = ((rgb & 0xFF000000) >> 24) / 255.0f;

	  rf = rf * alpha + (1 - alpha) * rc;
	  gf = gf * alpha + (1 - alpha) * gc;
	  bf = bf * alpha + (1 - alpha) * bc;

	  unsigned int r = (unsigned int) (rf * 255);
	  unsigned int g = (unsigned int) (gf * 255);
	  unsigned int b = (unsigned int) (bf * 255);
	  result.setRGB (x, y, (r << 16) | (g << 8) | b);
	  */

	  result (x, y) = c << image (x, y);
	}
  }

  return result;
}
