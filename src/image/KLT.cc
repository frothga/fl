/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
05/2005 Fred Rothganger -- Limit descent in scale pyramid based on point scale
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/track.h"
#include "fl/zroots.h"

#include <float.h>


// For debugging
//#include "fl/slideshow.h"


using namespace fl;
using namespace std;


// KLT ------------------------------------------------------------------------

/**
   \param windowRadius The number of discrete pixels beyond the center pixel
   to use when solving the position update equation.  The full window size
   will be 2 * windowRadius + 1.
   \param searchRadius The largest expected distance between the previous
   and current positions of a given point.  Determines number of levels
   in pyramid and degree of downsampling.
   \todo Track blobs (as opposed to just corners):  The code currently
   depends on a large determinant in the second moment matrix, which is
   generally associated with Harris points.  Examine whether it is possible
   to formulate a similar numerical search for blob-like structures.
   For example, if the determinant of the second moment matrix is negligible,
   then the first order terms of the Taylor expansion (see papers) is zero,
   and we can use second order Taylor terms instead.  Unfortunately, this
   would only work for blobs on the same scale as the search window.  We
   may need to code an adaptive window size.  Furthermore, this may vary
   depending on the level in the pyramid.
 **/
KLT::KLT (int windowRadius, int searchRadius)
{
  this->windowRadius = windowRadius;
  windowWidth = 2 * windowRadius + 1;
  minDeterminant = 2e-12f;
  minDisplacement = 0.1f;
  maxIterations = 10;
  maxError = 0.06f;

  // Determine size of pyramid.  The goal is to have searchRadius (in the
  // full-sized image) fit within a region covered by the Minkowski sum
  // of the search window at each of the respective scales in the pyramid.
  // Birchfield gives the following formula for this:
  // searchRadius = windowRadius * \sum_{i=0}^{levels-1} pyramidRatio^i
  //              = windowRadius * (pyramidRatio^levels - 1) / (pyramidRatio - 1)
  // Since there are two variables (pyramidRatio, levels) but one equation,
  // we use the the following heuristics:
  // * pyramidRatio in (1,8]
  // * Minimize the number of pyramid levels; levels in {1,2,3}
  int levels;
  float w = (float) searchRadius / windowRadius;
  if (w <= 1)  // pyramidRatio^0 = 1
  {
	levels = 1;
  }
  else if (w <= 9)  // pyramidRatio^0 + pyramidRatio^1 = 1 + 8 = 9, when pyramidRatio is at its max value of 8.
  {
	levels = 2;
	pyramidRatio = (w + sqrtf (w * w - 4.0f * (w - 1.0f))) / 2.0f;  // larger solution to quadratic equation: 0 = pyramidRatio^levels - w * pyramidRatio + w - 1
  }
  else
  {
	levels = 3;
	// Solve the equation 0 = pyramidRatio^levels - w * pyramidRation + w - 1, where levels = 3.
	Vector<complex<double> > coeffs (4);
	coeffs[0] = w - 1;
	coeffs[1] = -w;
	coeffs[2] = 0;
	coeffs[3] = 1;
	Vector<complex<double> > result;
	zroots (coeffs, result);
	pyramidRatio = 1;
	for (int i = 0; i < result.rows (); i++)
	{
	  if (fabs (imag (result[i])) < DBL_EPSILON)  // Use only real roots
	  {
		pyramidRatio = max (pyramidRatio, (float) real (result[i]));
	  }
	}
  }
  cerr << "KLT: levels=" << levels << " pyramidRatio=" << pyramidRatio << endl;

  // Create blurring kernel
  preBlur = Gaussian1D (windowWidth * 0.1, Boost);

  // Create pyramids
  pyramid0.resize (levels, ImageOf<float> (GrayFloat));
  pyramid1.resize (levels, ImageOf<float> (GrayFloat));
}

void
KLT::nextImage (const Image & image)
{
  pyramid0[0] = pyramid1[0];
  pyramid1[0] = image * GrayFloat * preBlur;

  TransformGauss t (1.0 / pyramidRatio, 1.0 / pyramidRatio);
  for (int i = 1; i < pyramid0.size (); i++)
  {
	ImageOf<float> & p = pyramid1[i];  // "picture"
	ImageOf<float> & pp = pyramid1[i-1];  // "previous picture"
	pyramid0[i] = p;

	// It is important to place the top left corner of pp *exactly* at the
	// top left corner of p so that coordinate conversions are meaningful
	// across scale levels.  This means putting any leftover part of one
	// pixel at the bottom and right edges.
	int w = (int) ceilf (pp.width / pyramidRatio);
	int h = (int) ceilf (pp.height / pyramidRatio);
	float x = (w - 1) * 0.5 + 0.5 - 0.5 / pyramidRatio;
	float y = (h - 1) * 0.5 + 0.5 - 0.5 / pyramidRatio;
	t.setWindow (x, y, w, h);
	p = pp * t;
  }

/*
SlideShow window;
Image disp (GrayFloat);
for (int i = 0; i < pyramid0.size (); i++)
{
  int y = disp.height;
  int x = 0;
  disp.bitblt (pyramid0[i], x, y);
  x += pyramid0[i].width;
  disp.bitblt (pyramid1[i], x, y);
}
window.show (disp);
window.waitForClick ();
*/

}

/**
   \throw int Indicates the following kinds of failures:
   <UL>
   <LI>2 -- Determinant of second moment matrix is too small, so can't
   solve tracking equation.
   <LI>3 -- Did not converge within maxIterations.  Not
   necessarily fatal, since it could be a cyclical fixed point near the
   correct answer (but not near enough to be under maxDisplacement).
   <LI>4 -- Point has move out of bounds.
   <LI>5 -- RMS error of pixels in window exceeds maxError, suggesting that
   we may no longer be looking at the same spot on the object.
   </UL>
   In all cases, the best estimate of the point's location is returned.
 **/
void
KLT::track (Point & point)
{
  Point offset (0.5, 0.5);
  Point point0 = point;
  point0 += offset;
  point0 /= powf (pyramidRatio, pyramid0.size ());
  point0 -= offset;
  Point point1 = point0;

  int highestLevel = pyramid0.size () - 1;
  int lowestLevel;
  if (PointInterest * p = dynamic_cast<PointInterest *> (&point))
  {
	lowestLevel = (int) rint (log (p->scale / windowRadius) / log (pyramidRatio));
	lowestLevel = max (lowestLevel, 0);
	lowestLevel = min (lowestLevel, highestLevel);
  }
  else
  {
	lowestLevel = 0;
  }

  float error = 0;
  int code = 0;
  int level;
  for (level = highestLevel; level >= lowestLevel; level--)
  {
	point0 += offset;
	point1 += offset;
	point0 *= pyramidRatio;
	point1 *= pyramidRatio;
	point0 -= offset;
	point1 -= offset;
	try
	{
	  error = track (point0, level, point1);
	}
	catch (int exceptionCode)
	{
	  code = exceptionCode;
	  if (code != 3)  // All codes besides 3 are fatal and call for immediate termination.
	  {
		level--;  // to guarantee that level is alwyas 1 less than last level processed by this loop
		break;
	  }
	}
  }

  if (level >= 0)
  {
	// To fulfill the guarantee that we return the best estimate of
	// location, we must move the point into the scale of the original
	// image.
	point1 += offset;
	point1 *= powf (pyramidRatio, level + 1);
	point1 -= offset;
  }

  point = point1;

  // The more serious exceptions should be listed here first.
  // Not sure whether overiterated > undercorrelated or vice-versa.
  if (code) throw code;
  if (error > maxError)
  {
	throw 5;
  }
}

/**
   \return RMS error of pixel intensity within window.
 **/
float
KLT::track (const Point & point0, const int level, Point & point1)
{
  ImageOf<float> & image0 = pyramid0[level];
  ImageOf<float> & image1 = pyramid1[level];

  int lastH = image0.width - 1;
  int lastV = image0.height - 1;

  if (point0.x < 0  ||  point0.x >= lastH  ||  point0.y < 0  ||  point0.y >= lastV)
  {
	throw 4;
  }

  const int xl = min ((int) floorf (point0.x),         windowRadius);
  const int xh = min ((int) floorf (lastH - point0.x), windowRadius);
  const int yl = min ((int) floorf (point0.y),         windowRadius);
  const int yh = min ((int) floorf (lastV - point0.y), windowRadius);
  lastH -= xh;
  lastV -= yh;
  const int width = xh + xl + 1;
  const int height = yh + yl + 1;
  const int pixels = width * height;

  const int rowAdvance = image0.width - width;

  // Compute the constant window (from image0)
  Vector<float> gradientX0 (pixels);
  Vector<float> gradientY0 (pixels);
  Vector<float> intensity0 (pixels);
  //   Determine bilinear mixing constants
  int x = (int) point0.x;
  int y = (int) point0.y;
  float dx = point0.x - x;
  float dy = point0.y - y;
  x -= xl;
  y -= yl;
  float dx1 = 1.0f - dx;
  float dy1 = 1.0f - dy;
  //   Iterate over images using 12 pointers, 4 for each of 3 images
  float * p00 = & image0(x,y);
  float * p10 = p00 + 1;
  float * p01 = p00 + image0.width;
  float * p11 = p01 + 1;
  float * gx0 = & gradientX0[0];
  float * gy0 = & gradientY0[0];
  float * i0 = & intensity0[0];
  for (int y = 0; y < height; y++)
  {
	for (int x = 0; x < width; x++)
	{
	  // Compute intensity difference and gradient values
	  float a = *p00 + dx * (*p10 - *p00);
	  float b = *p01 + dx * (*p11 - *p01);
	  *i0++ = a + dy * (b - a);
	  *gx0++ = dy1 * (*p10 - *p00) + dy * (*p11 - *p01);
	  *gy0++ = dx1 * (*p01 - *p00) + dx * (*p11 - *p10);
	  // Advance to next x position
	  p00++;
	  p10++;
	  p01++;
	  p11++;
	}
	// Advance pointers to next row
	p00 += rowAdvance;
	p10 += rowAdvance;
	p01 += rowAdvance;
	p11 += rowAdvance;
  }

  float error;
  int count = 0;
  while (true)
  {
	if (count++ >= maxIterations)
	{
	  throw 3;
	}

	if (point1.x < xl  ||  point1.x >= lastH  ||  point1.y < yl  ||  point1.y >= lastV)
	{
	  throw 4;
	}

	// Compute second moment matrix and error vector
	float gxx = 0;
	float gxy = 0;
	float gyy = 0;
	float ex = 0;
	float ey = 0;
	error = 0;
	//   Determine bilinear mixing constants for image1
	x = (int) point1.x;
	y = (int) point1.y;
	dx = point1.x - x;
	dy = point1.y - y;
	x -= xl;
	y -= yl;
	//   Set up pointers
	p00 = & image1(x,y);
	p10 = p00 + 1;
	p01 = p00 + image1.width;
	p11 = p01 + 1;
	gx0 = & gradientX0[0];
	gy0 = & gradientY0[0];
	i0 = & intensity0[0];
	for (int y = 0; y < height; y++)
	{
	  for (int x = 0; x < width; x++)
	  {
		// Compute intensity difference and gradient values
		float a = *p00 + dx * (*p10 - *p00);
		float b = *p01 + dx * (*p11 - *p01);
		float diff = *i0++ - (a + dy * (b - a));
		float gx = *gx0++ + dy1 * (*p10 - *p00) + dy * (*p11 - *p01);
		float gy = *gy0++ + dx1 * (*p01 - *p00) + dx * (*p11 - *p10);
		// Accumulate second moment matrix and error vector
		gxx += gx * gx;
		gxy += gx * gy;
		gyy += gy * gy;
		ex += diff * gx;
		ey += diff * gy;
		error += diff * diff;
		// Advance to next x position
		p00++;
		p10++;
		p01++;
		p11++;
	  }
	  // Advance pointers to next row
	  p00 += rowAdvance;
	  p10 += rowAdvance;
	  p01 += rowAdvance;
	  p11 += rowAdvance;
	}

	// Solve for displacement and update point1
	float det = gxx * gyy - gxy * gxy;
	if (det < minDeterminant)
	{
	  throw 2;
	}
	float dx = (gyy * ex - gxy * ey) / det;
	float dy = (gxx * ey - gxy * ex) / det;
	point1.x += dx;
	point1.y += dy;

	if (sqrtf (dx * dx + dy * dy) < minDisplacement)
	{
	  break;
	}
  }

  return sqrtf (error / pixels);
}
