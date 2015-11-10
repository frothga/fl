/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/track.h"


// For debugging
//#include "fl/slideshow.h"


using namespace fl;
using namespace std;


// KLT ------------------------------------------------------------------------

/**
   @param searchRadius The largest expected distance between the previous
   and current positions of a given point.  Determines number of levels
   in pyramid and degree of downsampling.
   @param windowRadius The number of discrete pixels beyond the center pixel
   to use when solving the position update equation.  The full window size
   will be 2 * windowRadius + 1.
   @param scaleRatio Determines how much blur to apply before comparing
   two windows for alignment. Actual blur amount is
   sqrt(expected offset) * scaleRatio
   @todo Track blobs (as opposed to just corners):  The code currently
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
KLT::KLT (int searchRadius, int windowRadius, float scaleRatio)
: windowRadius (windowRadius)
{
  minDeterminant  = 2e-12f;
  minDisplacement = 0.1f;
  maxIterations   = 10;
  maxError        = 0.06f;

  // Determine size of pyramid.
  // At any given level, the windowRadius should be at least as large as
  // the (downsampled) searchRadius.  That is, it should be possible to
  // fully solve the tracking problem at any given level.
  // Let two consecutive levels be called L (for lower) and U (for upper).
  // The area covered by one pixel in U should be no larger than the entire
  // search window in L. If it were larger, then a solution at U might not
  // fall within the window at L. The downsampling ratio (pyramidRatio) must
  // not exceed windowWidth, so we create enough levels that
  // pyramidRatio^levels >= searchRadius.
  int windowWidth = 2 * windowRadius + 1;
  double w = (double) searchRadius / windowRadius;
  int topLevel = (int) ceil (log (w) / log ((double) windowWidth));
  if (topLevel) pyramidRatio = (int) ceil (pow (w, 1.0 / topLevel));  // minimize downsample ratio by distributing it equally across all levels
  else          pyramidRatio = 1;
  int levels = topLevel + 1;  // topLevel is a zero-based index, whereas levels is the actual count

  // Create blurring kernels
  // Note that level 0 is a blurred but full-size version of the base image.
  blursPre .resize (levels);
  blursPost.resize (levels);
  double currentBlur = 0.5;  // default value for base image
  double downsample  = 1;    // to produce level 0 from the base image
  const double minBlur = 0.55;  // Threshold below which it is not worth applying a blur.  Should be at least 0.5
  for (int level = 0; level < levels; level++)
  {
	// Determine blur scale in new image (target) that will be generated for
	// this level.
	double radius;  // search radius at current level
	if (level == topLevel) radius = searchRadius / pow ((double) pyramidRatio, level);
	else                   radius = pyramidRatio / 2.0;  // level L expects level U to solve offset to within its center pixel
	double targetBlur = sqrt (radius) * scaleRatio;

	// Generate a kernel to blur the source image to (downsample / 2).  This
	// is the right level of blur to have just before a decimation by
	// "downsample", resulting in an image with blur scale 0.5.
	double blurAfterDecimation;
	double applyBlur = sqrt (downsample * downsample / 4.0 - currentBlur * currentBlur);
	if (std::isnan (applyBlur)  ||  applyBlur < minBlur)  // blur too small
	{
	  blursPre[level] = 0;
	  blurAfterDecimation = currentBlur / downsample;
	}
	else
	{
	  blursPre[level] = new Gaussian1D (applyBlur, Boost);
	  blurAfterDecimation = 0.5;
	}

	// Generate a kernal to blur the decimated image to targetBlur.
	applyBlur = sqrt (targetBlur * targetBlur - blurAfterDecimation * blurAfterDecimation);
	if (std::isnan (applyBlur)  ||  applyBlur < minBlur)
	{
	  blursPost[level] = 0;
	  currentBlur = blurAfterDecimation;
	}
	else
	{
	  blursPost[level] = new Gaussian1D (applyBlur, Boost);
	  currentBlur = targetBlur;
	}

	downsample = pyramidRatio;  // for levels above 0
  }

  // Create pyramids
  pyramid0.resize (levels, ImageOf<float> (GrayFloat));
  pyramid1.resize (levels, ImageOf<float> (GrayFloat));
}

KLT::~KLT ()
{
  for (int i = 0; i < blursPre .size (); i++) if (blursPre [i]) delete blursPre [i];
  for (int i = 0; i < blursPost.size (); i++) if (blursPost[i]) delete blursPost[i];
}

void
KLT::nextImage (const Image & image)
{
  // Blur level 0.  No decimation or pre-blurring required.
  ImageOf<float> & p = pyramid1[0];
  pyramid0[0] = p;
  p = image * GrayFloat;
  if (blursPost[0])
  {
	Gaussian1D & b = *blursPost[0];
	b.direction = Horizontal;
	p *= b;
	b.direction = Vertical;
	p *= b;
  }

  // Higher levels
  for (int level = 1; level < pyramid0.size (); level++)
  {
	ImageOf<float> & p  = pyramid1[level];   // "picture"
	ImageOf<float> & pp = pyramid1[level-1]; // "previous picture"
	pyramid0[level] = p;
	p.detach ();

	int hw = pp.width  / pyramidRatio;
	int hh = pp.height / pyramidRatio;
	p.resize (hw, hh);

	if (blursPre[level])  // downsample and decimate
	{
	  Gaussian1D & b = *blursPre[level];

	  b.direction = Horizontal;
	  Image temp = pp * b;

	  b.direction = Vertical;
	  Point t;
	  const float start = pyramidRatio / 2;
	  t.y = start;
	  for (int y = 0; y < hh; y++)
	  {
		t.x = start;
		for (int x = 0; x < hw; x++)
		{
		  p(x,y) = b.response (temp, t);
		  t.x += pyramidRatio;
		}
		t.y += pyramidRatio;
	  }
	}
	else  // decimate only
	{
	  const int start = pyramidRatio / 2;
	  int fromY = start;
	  for (int y = 0; y < hh; y++)
	  {
		int fromX = start;
		for (int x = 0; x < hw; x++)
		{
		  p(x,y) = pp(fromX,fromY);
		  fromX += pyramidRatio;
		}
		fromY += pyramidRatio;
	  }
	}

	if (blursPost[level])
	{
	  Gaussian1D & b = *blursPost[level];
	  b.direction = Horizontal;
	  p *= b;
	  b.direction = Vertical;
	  p *= b;
	}
  }
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
	lowestLevel = (int) roundp (log (p->scale / windowRadius) / log ((float) pyramidRatio));
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

  const int xl = min ((int) floorf (               point0.x), windowRadius);
  const int xh = min ((int) floorf (lastH - 1e-5 - point0.x), windowRadius);
  const int yl = min ((int) floorf (               point0.y), windowRadius);
  const int yh = min ((int) floorf (lastV - 1e-5 - point0.y), windowRadius);
  lastH -= xh;
  lastV -= yh;
  const int width  = xh + xl + 1;
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
  //   Iterate over image using 4 pointers
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
	if (point1.x < xl  ||  point1.x >= lastH  ||  point1.y < yl  ||  point1.y >= lastV)
	{
	  throw 4;
	}

	if (count++ >= maxIterations)
	{
	  throw 3;
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

  if (point1.x < xl  ||  point1.x >= lastH  ||  point1.y < yl  ||  point1.y >= lastV)
  {
	throw 4;
  }

  return sqrtf (error / pixels);
}
