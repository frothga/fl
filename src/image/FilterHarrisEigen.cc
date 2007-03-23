/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/03/23 02:32:02  Fred
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
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class FilterHarrisEigen ----------------------------------------------------

Image
FilterHarrisEigen::process ()
{
  int last = G1_I.width - 1;

  G1_I.direction = Vertical;
  Image Sxx = xx * G1_I;
  Image Sxy = xy * G1_I;
  Image Syy = yy * G1_I;
  G1_I.direction = Horizontal;
  Sxx = Sxx * G1_I;
  Sxy = Sxy * G1_I;
  Syy = Syy * G1_I;

  if (*G1_I.format == GrayFloat)
  {
	ImageOf<float> result (xx.width - last, xx.height - last, GrayFloat);
	ImageOf<float> Sxxf (Sxx);
	ImageOf<float> Sxyf (Sxy);
	ImageOf<float> Syyf (Syy);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		float txx = Sxxf (x, y);
		float txy = Sxyf (x, y);
		float tyy = Syyf (x, y);
		float t2 = (txx + tyy) * (txx + tyy);  // trace^2
		float d4 = 4 * (txx * tyy - txy * txy);  // 4 * determinant
		result (x, y) = fabsf (t2 - fabsf (t2 - d4));
	  }
	}
	return result;
  }
  else if (*G1_I.format == GrayDouble)
  {
	ImageOf<double> result (xx.width - last, xx.height - last, GrayDouble);
	ImageOf<double> Sxxd (Sxx);
	ImageOf<double> Sxyd (Sxy);
	ImageOf<double> Syyd (Syy);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		double txx = Sxxd (x, y);
		double txy = Sxyd (x, y);
		double tyy = Syyd (x, y);
		double t2 = (txx + tyy) * (txx + tyy);  // trace^2
		double d4 = 4 * (txx * tyy - txy * txy);  // 4 * determinant
		result (x, y) = fabs (t2 - fabs (t2 - d4));
	  }
	}
	return result;
  }
  else
  {
	throw "FilterHarrisEigen::process: unimplemented format";
  }
}

double
FilterHarrisEigen::response (const int x, const int y) const
{
  Point p (x + offsetI, y + offsetI);
  double txx = G_I.response (xx, p);
  double txy = G_I.response (xy, p);
  double tyy = G_I.response (yy, p);
  double t2 = (txx + tyy) * (txx + tyy);  // trace^2
  double d4 = 4 * (txx * tyy - txy * txy);  // 4 * determinant
  return fabs (t2 - fabs (t2 - d4));
}
