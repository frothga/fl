/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


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
