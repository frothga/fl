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

static float
interpolate (float x, float y, ImageOf<float> & image)
{
  int xt = (int) x;  /* coordinates of top-left corner */
  int yt = (int) y;
  float ax = x - xt;
  float ay = y - yt;
  float *ptr = & image(xt,yt);

  return ( (1-ax) * (1-ay) * *ptr +
           ax   * (1-ay) * *(ptr+1) +
           (1-ax) *   ay   * *(ptr+(image.width)) +
           ax   *   ay   * *(ptr+(image.width)+1) );
}

static void
computeIntensityDifference (ImageOf<float> & image1, ImageOf<float> & image2,
							float x1, float y1,
							float x2, float y2,
							int width, int height,
							float * imgdiff)
{
  register int hw = width/2, hh = height/2;
  float g1, g2;
  register int i, j;

  for (j = -hh ; j <= hh ; j++)
  {
    for (i = -hw ; i <= hw ; i++)
	{
      g1 = interpolate (x1+i, y1+j, image1);
      g2 = interpolate (x2+i, y2+j, image2);
      *imgdiff++ = g1 - g2;
    }
  }
}

static void
computeGradientSum (ImageOf<float> gradx1, ImageOf<float> & grady1,
					ImageOf<float> gradx2, ImageOf<float> & grady2,
					float x1, float y1,
					float x2, float y2,
					int width, int height,
					float * gradx,
					float * grady)
{
  register int hw = width/2, hh = height/2;
  float g1, g2;
  register int i, j;

  for (j = -hh ; j <= hh ; j++)
  {
    for (i = -hw ; i <= hw ; i++)
	{
      g1 = interpolate(x1+i, y1+j, gradx1);
      g2 = interpolate(x2+i, y2+j, gradx2);
      *gradx++ = g1 + g2;
      g1 = interpolate(x1+i, y1+j, grady1);
      g2 = interpolate(x2+i, y2+j, grady2);
      *grady++ = g1 + g2;
    }
  }
}

static void
compute2by2GradientMatrix (float * gradx, float * grady,
						   int width, int height,
						   float & gxx, float & gxy, float & gyy)
{
  register float gx, gy;
  register int i;

  gxx = 0;
  gxy = 0;
  gyy = 0;
  for (i = 0; i < width * height; i++)
  {
    gx = *gradx++;
    gy = *grady++;
    gxx += gx * gx;
    gxy += gx * gy;
    gyy += gy * gy;
  }
}

static void
compute2by1ErrorVector (float * imgdiff, float * gradx, float * grady,
						int width, int height,
						float & ex, float & ey)
{
  register float diff;
  register int i;

  ex = 0;
  ey = 0;
  for (i = 0; i < width * height; i++)
  {
    diff = *imgdiff++;
    ex += diff * (*gradx++);
    ey += diff * (*grady++);
  }
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

  int width = windowWidth;
  int height = width;
  int hw = windowRadius;
  int hh = hw;
  int nc = image0.width;
  int nr = image0.height;

  float * imgdiff = (float *) malloc (width * height * sizeof (float));
  float * gradx = (float *) malloc (width * height * sizeof (float));
  float * grady = (float *) malloc (width * height * sizeof (float));

  float gxx, gxy, gyy, ex, ey, dx, dy;
  const float one_plus_eps = 1.000001f;

  // Iteratively update the window position
  int iteration = 0;
  do
  {
    if (point0.x-hw < 0.0f  ||  point0.x+hw > nc-one_plus_eps  ||
        point1.x-hw < 0.0f  ||  point1.x+hw > nc-one_plus_eps  ||
        point0.y-hh < 0.0f  ||  point0.y+hh > nr-one_plus_eps  ||
        point1.y-hh < 0.0f  ||  point1.y+hh > nr-one_plus_eps)
	{
	  throw 1;
    }

    // Compute gradient and difference windows
    computeIntensityDifference (image0, image1,
								point0.x, point0.y, point1.x, point1.y,
								width, height, imgdiff);

    computeGradientSum (dx0, dy0, dx1, dy1,
						point0.x, point0.y, point1.x, point1.y,
						width, height,
						gradx, grady);
		

    // Use these windows to construct matrices
    compute2by2GradientMatrix (gradx, grady,
							   width, height,
                               gxx, gxy, gyy);
    compute2by1ErrorVector (imgdiff, gradx, grady,
							width, height,
                            ex, ey);
				
    // Using matrices, solve equation for new displacement
	float det = gxx * gyy - gxy * gxy;
	if (det < minDeterminant) throw 2;
	dx = (gyy * ex - gxy * ey) / det;
	dy = (gxx * ey - gxy * ex) / det;

    point1.x += dx;
    point1.y += dy;
    iteration++;
  }
  while ((fabs (dx) >= minDisplacement  ||  fabs (dy) >= minDisplacement)  &&  iteration < maxIterations);

  // Free memory
  free (imgdiff);
  free (gradx);
  free (grady);
}

void
KLT::track (Point & point)
{
  Point loc = point;

  // Transform location to coarsest resolution
  for (int r = pyramid0.size () - 1; r >= 0; r--)
  {
	loc /= pyramidRatio;
  }
  Point locout = loc;

  // Beginning with coarsest resolution, do...
  for (int r = pyramid0.size () - 1; r >= 0; r--)
  {
	// Track feature at current resolution
	loc *= pyramidRatio;
	locout *= pyramidRatio;
	track (loc, r, locout);
  }

  point = locout;
}


/*
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

  const int border = windowRadius + dG.width / 2;
  const float left = border;
  const float right = image0.width - border - 1;
  const float top = border;
  const float bottom = image0.height - border - 1;
  const int rowAdvance = image0.width - windowWidth;  // Two potential OBOBs: 1) should subtract (windowWidth - 1) rather than windowWidth, 2) should account for extra "x" advance at bottom of inner loop below.

  if (point0.x < left  ||  point0.x >= right  ||  point0.y < top  ||  point0.y >= bottom)
  {
	throw 1;
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

	//   Determine bilinear mixing constants
	int xl = (int) point1.x;
	int yl = (int) point1.y;
	float bx = point1.x - xl;
	float by = point1.y - yl;
	xl -= windowRadius;
	yl -= windowRadius;
	int xh = xl + windowWidth - 1;
	int yh = yl + windowWidth - 1;

	//   Iterate over window
int step = 0;
	//     Set up 24 pointers, 4 for each of the 6 images.
	float * i0_00 = & image0(xl,yl);
	float * i0_10 = i0_00 + 1;
	float * i0_01 = i0_00 + image0.width;
	float * i0_11 = i0_01 + 1;
	float * dx0_00 = & dx0(xl,yl);
	float * dx0_10 = dx0_00 + 1;
	float * dx0_01 = dx0_00 + dx0.width;
	float * dx0_11 = dx0_01 + 1;
	float * dy0_00 = & dy0(xl,yl);
	float * dy0_10 = dy0_00 + 1;
	float * dy0_01 = dy0_00 + dy0.width;
	float * dy0_11 = dy0_01 + 1;
	float * i1_00 = & image1(xl,yl);
	float * i1_10 = i1_00 + 1;
	float * i1_01 = i1_00 + image1.width;
	float * i1_11 = i1_01 + 1;
	float * dx1_00 = & dx1(xl,yl);
	float * dx1_10 = dx1_00 + 1;
	float * dx1_01 = dx1_00 + dx1.width;
	float * dx1_11 = dx1_01 + 1;
	float * dy1_00 = & dy1(xl,yl);
	float * dy1_10 = dy1_00 + 1;
	float * dy1_01 = dy1_00 + dy1.width;
	float * dy1_11 = dy1_01 + 1;
	for (int y = yl; y <= yh; y++)
	{
	  for (int x = xl; x <= xh; x++)
	  {
		// Compute intensity difference and gradient values
		float a = *i0_00 + bx * (*i0_10 - *i0_00);
		float b = *i0_01 + bx * (*i0_11 - *i0_01);
		a      -= *i1_00 + bx * (*i1_10 - *i1_00);
		b      -= *i1_01 + bx * (*i1_11 - *i1_01);
		float diff = a + by * (b - a);
		a  = *dx0_00 + bx * (*dx0_10 - *dx0_00);
		b  = *dx0_01 + bx * (*dx0_11 - *dx0_01);
		a += *dx1_00 + bx * (*dx1_10 - *dx1_00);
		b += *dx1_01 + bx * (*dx1_11 - *dx1_01);
		float gx = a + by * (b - a);
		a  = *dy0_00 + bx * (*dy0_10 - *dy0_00);
		b  = *dy0_01 + bx * (*dy0_11 - *dy0_01);
		a += *dy1_00 + bx * (*dy1_10 - *dy1_00);
		b += *dy1_01 + bx * (*dy1_11 - *dy1_01);
		float gy = a + by * (b - a);
cerr << "   " << step++ << " " << gx << " " << gy << " " << diff << endl;

		// Accumulate second moment matrix and error vector
		gxx += gx * gx;
		gxy += gx * gy;
		gyy += gy * gy;
		ex += diff * gx;
		ey += diff * gy;

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
	cerr << count << " " << gxx << " " << gxy << " " << gyy << " " << ex << " " << ey << " " << dx << " " << dy << endl;

	if (sqrtf (dx * dx + dy * dy) < minDisplacement)
	{
	  break;
	}
  }
}
*/
