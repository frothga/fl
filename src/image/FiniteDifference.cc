/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.1  2004/01/08 21:30:09  rothgang
Add method for computing derivative images using finite differences.  Main use
is in DescriptorSIFT and for speed in other applications.
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"


using namespace fl;
using namespace std;



// class FiniteDifferenceX ----------------------------------------------------

Image
FiniteDifferenceX::filter (const Image & image)
{
  ImageOf<float> work = image * GrayFloat;
  ImageOf<float> result (image.width, image.height, GrayFloat);

  int lastX = image.width - 1;
  int penX  = lastX - 1;  // "pen" as in "penultimate"
  for (int y = 0; y < image.height; y++)
  {
	result(0,    y) = 2.0f * (work(1,    y) - work(0,   y));
	result(lastX,y) = 2.0f * (work(lastX,y) - work(penX,y));

	float * p = & work(2,y);
	float * m = & work(0,y);
	float * c = & result(1,y);
	float * end = c + (image.width - 2);
	while (c < end)
	{
	  *c++ = *p++ - *m++;
	}
  }

  return result;
}


// class FiniteDifferenceY ----------------------------------------------------

Image
FiniteDifferenceY::filter (const Image & image)
{
  ImageOf<float> work = image * GrayFloat;
  ImageOf<float> result (image.width, image.height, GrayFloat);

  // Compute top and bottom rows
  int lastX = image.width - 1;
  int lastY = image.height - 1;
  int penY  = lastY - 1;
  float * pt = & work(0,1);
  float * mt = & work(0,0);
  float * pb = & work(0,lastY);
  float * mb = & work(0,penY);
  float * ct = & result(0,0);
  float * cb = & result(0,lastY);
  float * end = ct + image.width;
  while (ct < end)
  {
	*ct++ = 2.0f * (*pt++ - *mt++);
	*cb++ = 2.0f * (*pb++ - *mb++);
  }

  // Compute everything else
  float * p = & work(0,2);
  float * m = & work(0,0);
  float * c = & result(0,1);
  end = &result(lastX,penY) + 1;
  while (c < end)
  {
	*c++ = *p++ - *m++;
  }

  return result;
}
