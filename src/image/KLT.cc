/*
  Notes 5/4/04
  * border coverage: Adapt shape of window so center can reach all the way
    to the edge of the image.
  * tracking of blobs (as opposed to corners): use second order Taylor terms
*/


#include "fl/track.h"


// For debugging
#include "fl/slideshow.h"


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
 **/
KLT::KLT (int windowRadius, int searchRadius)
{
  this->windowRadius = windowRadius;
  windowWidth = 2 * windowRadius + 1;
  minDeterminant = 2e-12;
  minDisplacement = 0.1;
  maxIterations = 10;
  maxError = 0.06;

  // Determine size of pyramid.  The goal is to have searchRadius (in the
  // full-sized image) fit within a region covered by the Minkowski sum
  // of the search window at each of the respective scales in the pyramid.
  // Birchfield gives the following formula for this:
  // searchRadius = windowRadius * \sum_{i=0}^{levels-1} pyramidRatio^i
  //              = windowRadius * (pyramidRatio^levels - 1) / (pyramidRatio - 1)
  // Since there are two variables (pyramidRatio, levels) but one equation,
  // we use the the following heuristics:
  // * Pick pyramidRatio to be a power of 2 that is no greater than 8.
  // * Minimize the number of pyramid levels.
  int levels;
  float w = (float) searchRadius / windowRadius;
  if (w <= 1)
  {
	pyramidRatio = 1;
	levels = 1;
  }
  else if (w <= 3)  // pyramidRatio + pyramidRatio^2 = 1 + 2 = 3
  {
	pyramidRatio = 2;
	levels = 2;
  }
  else if (w <= 5)  // pyramidRatio + pyramidRatio^2 = 1 + 4 = 5
  {
	pyramidRatio = 4;
	levels = 2;
  }
  else  // pyramidRatio fixed at 8; calculate # of levels
  {
	pyramidRatio = 8;
	levels = (int) ceil (log (7.0 * w + 1.0) / log (8.0));
  }

  // Create convolution kernels
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
	ImageOf<float> & p = pyramid1[i];
	pyramid0[i] = p;
	p = pyramid1[i-1] * t;
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

  float error = 0;
  bool overiterated = false;
  for (int level = pyramid0.size () - 1; level >= 0; level--)
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
	catch (int code)
	{
	  if (code == 3)
	  {
		overiterated = true;
	  }
	  else  // All other codes are fatal and call for immediate termination.
	  {
		// To fulfill the guarantee that we return the best estimate of
		// location, we must move the point into the scale of the original
		// image.
		point0 += offset;
		point1 += offset;
		point0 *= powf (pyramidRatio, level);
		point1 *= powf (pyramidRatio, level);
		point0 -= offset;
		point1 -= offset;
		throw code;
	  }
	}
  }

  point = point1;

  // The more serious exceptions should be listed here first.
  // Not sure whether overiterated > undercorrelated or vice-versa.
  if (overiterated)
  {
	throw 3;
  }
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

  const int border = windowRadius;
  const float left = border;
  const float right = image0.width - border - 1;
  const float top = border;
  const float bottom = image0.height - border - 1;
  const int rowAdvance = image0.width - windowWidth;  // Two potential OBOBs: 1) should subtract (windowWidth - 1) rather than windowWidth, 2) should account for extra "x" advance at bottom of inner loop below.

  if (point0.x < left  ||  point0.x >= right  ||  point0.y < top  ||  point0.y >= bottom)
  {
	throw 4;
  }

  // Compute the constant window (from image0)
  Vector<float> gradientX0 (windowWidth * windowWidth);
  Vector<float> gradientY0 (windowWidth * windowWidth);
  Vector<float> intensity0 (windowWidth * windowWidth);
  //   Determine bilinear mixing constants
  int x = (int) point0.x;
  int y = (int) point0.y;
  float dx = point0.x - x;
  float dy = point0.y - y;
  x -= windowRadius;
  y -= windowRadius;
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
  for (int y = 0; y < windowWidth; y++)
  {
	for (int x = 0; x < windowWidth; x++)
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

	if (point1.x < left  ||  point1.x >= right  ||  point1.y < top  ||  point1.y >= bottom)
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
	x -= windowRadius;
	y -= windowRadius;
	//   Set up pointers
	p00 = & image1(x,y);
	p10 = p00 + 1;
	p01 = p00 + image1.width;
	p11 = p01 + 1;
	gx0 = & gradientX0[0];
	gy0 = & gradientY0[0];
	i0 = & intensity0[0];
	for (int y = 0; y < windowWidth; y++)
	{
	  for (int x = 0; x < windowWidth; x++)
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

  return sqrtf (error / (windowWidth * windowWidth));
}
