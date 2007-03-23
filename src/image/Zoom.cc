/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.3 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/10/09 05:16:52  Fred
Remove unused headers.  Add Sandia copyright notice.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.1  2004/01/08 21:22:16  rothgang
Add method of upscaling images by and integer amount.  Avoids interpolation, so
pixels remain crisp.
-------------------------------------------------------------------------------
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
