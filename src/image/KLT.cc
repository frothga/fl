/*
  Notes 5/4/04
  In the first pass, try to imitate Birchfield closely.  The go back and
  improve:
  * efficiency: use finite differences or direct bilinear derivatives
  * border coverage: use boosting.  Also, alternate derivates will avoid
  cutting off edges.
  * tracking of blobs (as opposed to corners): use second order Taylor terms

  It's more important to be accurate and tenacious than to be fast.  Therefore,
  avoid using hacks for the derivatives.
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
  pyramidBlur = Gaussian1D (sqrt (pyramidRatio * pyramidRatio / 4.0 - 0.25), Boost);  // sigma_needed^2 = sigma_new^2 - sigma_old^2; sigma_new = pyramidRatio * sigma_old; sigma_old = 0.5 by definition
  G.mode = Boost;  // Default constructor uses sigma = 1, which is correct.
  dG.mode = ZeroFill;  // ditto

  // Create pyramids
  pyramid0.resize (levels, ImageOf<float> (GrayFloat));
  dx0.resize      (levels, ImageOf<float> (GrayFloat));
  dy0.resize      (levels, ImageOf<float> (GrayFloat));
  pyramid1.resize (levels, ImageOf<float> (GrayFloat));
  dx1.resize      (levels, ImageOf<float> (GrayFloat));
  dy1.resize      (levels, ImageOf<float> (GrayFloat));
}

void
KLT::nextImage (const Image & image)
{
  pyramid0[0] = pyramid1[0];
  pyramid1[0] = image * GrayFloat * preBlur;

  const float start = pyramidRatio / 2;

  for (int i = 1; i < pyramid0.size (); i++)
  {
	ImageOf<float> & p = pyramid1[i];
	pyramid0[i] = p;
	p.detach ();

	int hw = pyramid1[i-1].width / pyramidRatio;
	int hh = pyramid1[i-1].height / pyramidRatio;
	p.resize (hw, hh);

	pyramidBlur.direction = Horizontal;
	Image temp = pyramid1[i-1] * pyramidBlur;

	pyramidBlur.direction = Vertical;
	Point t;
	t.y = start;
	for (int y = 0; y < hh; y++)
	{
	  t.x = start;
	  for (int x = 0; x < hw; x++)
	  {
		p(x,y) = pyramidBlur.response (temp, t);
		t.x += pyramidRatio;
	  }
	  t.y += pyramidRatio;
	}
  }

  for (int i = 0; i < dx0.size (); i++)
  {
	dx0[i] = dx1[i];
	dy0[i] = dy1[i];

	G.direction = Vertical;
	dx1[i] = pyramid1[i] * G;
	dG.direction = Horizontal;
	dx1[i] *= dG;

	G.direction = Horizontal;
	dy1[i] = pyramid1[i] * G;
	dG.direction = Vertical;
	dy1[i] *= dG;
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
  x += pyramid1[i].width;
  disp.bitblt (dx0[i] * Rescale (dx0[i]), x, y);
  x += dx0[i].width;
  disp.bitblt (dx1[i] * Rescale (dx1[i]), x, y);
  x += dx1[i].width;
  disp.bitblt (dy0[i] * Rescale (dy0[i]), x, y);
  x += dy0[i].width;
  disp.bitblt (dy1[i] * Rescale (dy1[i]), x, y);
}
window.show (disp);
window.waitForClick ();
*/
}

void
KLT::track (Point & point)
{
  Point offset (0.5, 0.5);
  Point point0 = point;
  point0 += offset;
  point0 /= powf (pyramidRatio, pyramid0.size ());
  point0 -= offset;
  Point point1 = point0;

  for (int level = pyramid0.size () - 1; level >= 0; level--)
  {
	point0 += offset;
	point1 += offset;
	point0 *= pyramidRatio;
	point1 *= pyramidRatio;
	point0 -= offset;
	point1 -= offset;
	track (point0, level, point1);
  }

  point = point1;
}

void
KLT::track (const Point & point0, const int level, Point & point1)
{
  ImageOf<float> & image0 = pyramid0[level];
  ImageOf<float> & dx0    = this->dx0[level];
  ImageOf<float> & dy0    = this->dy0[level];
  ImageOf<float> & image1 = pyramid1[level];
  ImageOf<float> & dx1    = this->dx1[level];
  ImageOf<float> & dy1    = this->dy1[level];

  const int border = windowRadius; // + dG.width / 2;
  const float left = border;
  const float right = image0.width - border - 1;
  const float top = border;
  const float bottom = image0.height - border - 1;
  const int rowAdvance = image0.width - windowWidth;  // Two potential OBOBs: 1) should subtract (windowWidth - 1) rather than windowWidth, 2) should account for extra "x" advance at bottom of inner loop below.

  if (point0.x < left  ||  point0.x >= right  ||  point0.y < top  ||  point0.y >= bottom)
  {
	throw 1;
  }

  // Compute the constant window (from image0)
  Vector<float> gradientX0 (windowWidth * windowWidth);
  Vector<float> gradientY0 (windowWidth * windowWidth);
  Vector<float> intensity0 (windowWidth * windowWidth);
  //   Determine bilinear mixing constants
  int x0 = (int) point0.x;
  int y0 = (int) point0.y;
  const float bx0 = point0.x - x0;
  const float by0 = point0.y - y0;
  x0 -= windowRadius;
  y0 -= windowRadius;
  //   Iterate over images using 12 pointers, 4 for each of 3 images
  float * i0_00 = & image0(x0,y0);
  float * i0_10 = i0_00 + 1;
  float * i0_01 = i0_00 + image0.width;
  float * i0_11 = i0_01 + 1;
  float * dx0_00 = & dx0(x0,y0);
  float * dx0_10 = dx0_00 + 1;
  float * dx0_01 = dx0_00 + dx0.width;
  float * dx0_11 = dx0_01 + 1;
  float * dy0_00 = & dy0(x0,y0);
  float * dy0_10 = dy0_00 + 1;
  float * dy0_01 = dy0_00 + dy0.width;
  float * dy0_11 = dy0_01 + 1;
  float * gx0 = & gradientX0[0];
  float * gy0 = & gradientY0[0];
  float * i0 = & intensity0[0];
  for (int y = 0; y < windowWidth; y++)
  {
	for (int x = 0; x < windowWidth; x++)
	{
	  // Compute intensity difference and gradient values
	  float a = *i0_00 + bx0 * (*i0_10 - *i0_00);
	  float b = *i0_01 + bx0 * (*i0_11 - *i0_01);
	  *i0++ = a + by0 * (b - a);
	  a = *dx0_00 + bx0 * (*dx0_10 - *dx0_00);
	  b = *dx0_01 + bx0 * (*dx0_11 - *dx0_01);
	  *gx0++ = a + by0 * (b - a);
	  a = *dy0_00 + bx0 * (*dy0_10 - *dy0_00);
	  b = *dy0_01 + bx0 * (*dy0_11 - *dy0_01);
	  *gy0++ = a + by0 * (b - a);
	  // Advance to next x position
	  i0_00++;
	  i0_10++;
	  i0_01++;
	  i0_11++;
	  dx0_00++;
	  dx0_10++;
	  dx0_01++;
	  dx0_11++;
	  dy0_00++;
	  dy0_10++;
	  dy0_01++;
	  dy0_11++;
	}
	// Advance pointers to next row
	i0_00 += rowAdvance;
	i0_10 += rowAdvance;
	i0_01 += rowAdvance;
	i0_11 += rowAdvance;
	dx0_00 += rowAdvance;
	dx0_10 += rowAdvance;
	dx0_01 += rowAdvance;
	dx0_11 += rowAdvance;
	dy0_00 += rowAdvance;
	dy0_10 += rowAdvance;
	dy0_01 += rowAdvance;
	dy0_11 += rowAdvance;
  }

  int count = 0;
  while (count++ < maxIterations)
  {
	if (point1.x < left  ||  point1.x >= right  ||  point1.y < top  ||  point1.y >= bottom)
	{
	  throw 1;
	}

	// Compute second moment matrix and error vector
	float gxx = 0;
	float gxy = 0;
	float gyy = 0;
	float ex = 0;
	float ey = 0;
	//   Determine bilinear mixing constants for image1
	int x1 = (int) point1.x;
	int y1 = (int) point1.y;
	const float bx1 = point1.x - x1;
	const float by1 = point1.y - y1;
	x1 -= windowRadius;
	y1 -= windowRadius;
	//   Set up 12 pointers, 4 for each of the 3 images.
	float * i1_00 = & image1(x1,y1);
	float * i1_10 = i1_00 + 1;
	float * i1_01 = i1_00 + image1.width;
	float * i1_11 = i1_01 + 1;
	float * dx1_00 = & dx1(x1,y1);
	float * dx1_10 = dx1_00 + 1;
	float * dx1_01 = dx1_00 + dx1.width;
	float * dx1_11 = dx1_01 + 1;
	float * dy1_00 = & dy1(x1,y1);
	float * dy1_10 = dy1_00 + 1;
	float * dy1_01 = dy1_00 + dy1.width;
	float * dy1_11 = dy1_01 + 1;
	gx0 = & gradientX0[0];
	gy0 = & gradientY0[0];
	i0 = & intensity0[0];
	for (int y = 0; y < windowWidth; y++)
	{
	  for (int x = 0; x < windowWidth; x++)
	  {
		// Compute intensity difference and gradient values
		float a = *i1_00 + bx1 * (*i1_10 - *i1_00);
		float b = *i1_01 + bx1 * (*i1_11 - *i1_01);
		float diff = *i0++ - (a + by1 * (b - a));
		a = *dx1_00 + bx1 * (*dx1_10 - *dx1_00);
		b = *dx1_01 + bx1 * (*dx1_11 - *dx1_01);
		float gx = *gx0++ + a + by1 * (b - a);
		a = *dy1_00 + bx1 * (*dy1_10 - *dy1_00);
		b = *dy1_01 + bx1 * (*dy1_11 - *dy1_01);
		float gy = *gy0++ + a + by1 * (b - a);
		// Accumulate second moment matrix and error vector
		gxx += gx * gx;
		gxy += gx * gy;
		gyy += gy * gy;
		ex += diff * gx;
		ey += diff * gy;
		// Advance to next x position
		i1_00++;
		i1_10++;
		i1_01++;
		i1_11++;
		dx1_00++;
		dx1_10++;
		dx1_01++;
		dx1_11++;
		dy1_00++;
		dy1_10++;
		dy1_01++;
		dy1_11++;
	  }
	  // Advance pointers to next row
	  i1_00 += rowAdvance;
	  i1_10 += rowAdvance;
	  i1_01 += rowAdvance;
	  i1_11 += rowAdvance;
	  dx1_00 += rowAdvance;
	  dx1_10 += rowAdvance;
	  dx1_01 += rowAdvance;
	  dx1_11 += rowAdvance;
	  dy1_00 += rowAdvance;
	  dy1_10 += rowAdvance;
	  dy1_01 += rowAdvance;
	  dy1_11 += rowAdvance;
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
}
