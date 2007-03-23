/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.4  2004/05/03 20:16:15  rothgang
Rearrange parameters for Gaussians so border mode comes before format.

Revision 1.3  2003/09/07 22:05:56  rothgang
Fixed bug in selection of image format for double.

Revision 1.2  2003/07/30 14:06:45  rothgang
Finish rewriting FilterHarris into FilterHessian.

Revision 1.1  2003/07/24 21:08:06  rothgang
Created new Hessian filter.
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class FilterHessian --------------------------------------------------------

FilterHessian::FilterHessian (double sigma, const PixelFormat & format)
: G (sigma, Crop, format),
  dG (sigma, Crop, format)
{
  this->sigma = sigma;

  dG *= sigma * sigma;  // Boost to make results comparable across scale.

  offset = max (G.width, dG.width) / 2;

  offset1 = 0;
  offset2 = 0;
  int difference = (G.width - dG.width) / 2;
  if (difference > 0)
  {
	offset1 = difference;
  }
  else if (difference < 0)
  {
	offset2 = -difference;
  }
}

Image
FilterHessian::filter (const Image & image)
{
  if (*image.format != *G.format)
  {
	return filter (image * (*G.format));
  }

  G.direction = Vertical;
  dG.direction = Horizontal;
  Image dxx = image * G * dG;

  G.direction = Horizontal;
  dG.direction = Vertical;
  Image dyy = image * G * dG;

  if (*dxx.format == GrayFloat)
  {
	ImageOf<float> result (min (dxx.width, dyy.width), min (dxx.height, dyy.height), GrayFloat);
	ImageOf<float> dxxf (dxx);
	ImageOf<float> dyyf (dyy);
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		result (x, y) = dxxf (x + offset1, y + offset2) + dyyf (x + offset2, y + offset1);
	  }
	}
	return result;
  }
  else if (*dxx.format == GrayDouble)
  {
	ImageOf<double> result (min (dxx.width, dyy.width), min (dxx.height, dyy.height), GrayDouble);
	ImageOf<double> dxxd (dxx);
	ImageOf<double> dyyd (dyy);
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		result (x, y) = dxxd (x + offset1, y + offset2) + dyyd (x + offset2, y + offset1);
	  }
	}
	return result;
  }
  else
  {
	throw "FilterHessian::filter: unimplemented format";
  }
}
