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
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class FilterHarris ---------------------------------------------------------

const double FilterHarris::alpha = 0.06;

FilterHarris::FilterHarris (double sigmaD, double sigmaI, const PixelFormat & format)
: G_I (sigmaI, Crop, format),
  G1_I (sigmaI, Crop, format),
  G1_D (sigmaD, Crop, format),
  dG_D (sigmaD, Crop, format),
  xx (format),
  xy (format),
  yy (format)
{
  this->sigmaD = sigmaD;
  this->sigmaI = sigmaI;

  dG_D *= sigmaD;  // Boost to make results comparable across scale.

  offsetI = G1_I.width / 2;
  offsetD = max (G1_D.width, dG_D.width) / 2;
  offset = offsetI + offsetD;

  offset1 = 0;
  offset2 = 0;
  int difference = (G1_D.width - dG_D.width) / 2;
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
FilterHarris::filter (const Image & image)
{
  preprocess (image);
  return process ();
}

void
FilterHarris::preprocess (const Image & image)
{
  if (*image.format != *G1_D.format)
  {
	preprocess (image * (*G1_D.format));
  }

  G1_D.direction = Vertical;
  dG_D.direction = Horizontal;
  Image dx = image * G1_D * dG_D;

  G1_D.direction = Horizontal;
  dG_D.direction = Vertical;
  Image dy = image * G1_D * dG_D;

  xx.resize (min (dx.width, dy.width), min (dx.height, dy.height));
  xy.resize (xx.width, xx.height);
  yy.resize (xx.width, xx.height);

  if (*dx.format == GrayFloat)
  {
	ImageOf<float> dxf (dx);
	ImageOf<float> dyf (dy);
	ImageOf<float> xxf (xx);
	ImageOf<float> xyf (xy);
	ImageOf<float> yyf (yy);
	for (int y = 0; y < xx.height; y++)
	{
	  for (int x = 0; x < xx.width; x++)
	  {
		float tx = dxf (x + offset1, y + offset2);
		float ty = dyf (x + offset2, y + offset1);
		xxf (x, y) = tx * tx;
		xyf (x, y) = tx * ty;
		yyf (x, y) = ty * ty;
	  }
	}
  }
  else if (*dx.format == GrayDouble)
  {
	ImageOf<double> dxd (dx);
	ImageOf<double> dyd (dy);
	ImageOf<double> xxd (xx);
	ImageOf<double> xyd (xy);
	ImageOf<double> yyd (yy);
	for (int y = 0; y < xx.height; y++)
	{
	  for (int x = 0; x < xx.width; x++)
	  {
		double tx = dxd (x + offset1, y + offset2);
		double ty = dyd (x + offset2, y + offset1);
		xxd (x, y) = tx * tx;
		xyd (x, y) = tx * ty;
		yyd (x, y) = ty * ty;
	  }
	}
  }
  else
  {
	throw "FilterHarris::preprocess: unimplemented format";
  }
}

Image
FilterHarris::process ()
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
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		float txx = Sxxf (x, y);
		float txy = Sxyf (x, y);
		float tyy = Syyf (x, y);
		result (x, y) = (txx * tyy - txy * txy) - alpha * (txx + tyy) * (txx + tyy);
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
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		double txx = Sxxd (x, y);
		double txy = Sxyd (x, y);
		double tyy = Syyd (x, y);
		result (x, y) = (txx * tyy - txy * txy) - alpha * (txx + tyy) * (txx + tyy);
	  }
	}
	return result;
  }
  else
  {
	throw "FilterHarris::process: unimplemented format";
  }
}

double
FilterHarris::response (int x, int y) const
{
  Matrix<double> t;
  gradientSquared (x, y, t);
  double & txx = t(0,0);
  double & txy = t(0,1);
  double & tyy = t(1,1);
  return (txx * tyy - txy * txy) - alpha * (txx + tyy) * (txx + tyy);
}

void
FilterHarris::gradientSquared (int x, int y, Matrix<double> & result) const
{
  Point p (x + offsetI, y + offsetI);
  result.resize (2, 2);
  result(0,0) = G_I.response (xx, p);
  result(0,1) = G_I.response (xy, p);
  result(1,0) = result(0,1);
  result(1,1) = G_I.response (yy, p);
}
