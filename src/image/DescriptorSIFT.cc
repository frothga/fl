/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/lapackd.h"
#include "fl/color.h"


// Temporarily include imagecache.h until it is put permanently in descriptor.h
#include "fl/imagecache.h"


using namespace fl;
using namespace std;


// class DescriptorSIFT ------------------------------------------------------

DescriptorSIFT::DescriptorSIFT (int width, int angles)
{
  this->width = width;
  this->angles = angles;
  dimension = width * width * angles;

  supportRadial = 3 * width / 2.0f;  // Causes each bin to cover 3 sigmas.
  supportPixel = (int) ceilf (2 * supportRadial);  // Causes drawn off patch to hold 2 pixels per sigma.
  sigmaWeight = width / 2.0;
  maxValue = 0.2f;

  lastBuffer = 0;
}

DescriptorSIFT::DescriptorSIFT (istream & stream)
{
  lastBuffer = 0;
  read (stream);
}

void
DescriptorSIFT::computeGradient (const Image & image)
{
  if (lastBuffer == (void *) image.buffer  &&  lastTime == image.timestamp)
  {
	return;
  }
  lastBuffer = (void *) image.buffer;
  lastTime = image.timestamp;

  ImageOf<float> work = image * GrayFloat;
  I_x = work * FiniteDifferenceX ();
  I_y = work * FiniteDifferenceY ();
}

Vector<float>
DescriptorSIFT::value (const Image & image, const PointAffine & point)
{
  // First, prepeare the derivative images I_x and I_y, and prepare projection
  // information between the derivative images and the bins.

  const float center = (width - 1) / 2.0f;
  const float sigma2 = 2.0f * sigmaWeight * sigmaWeight;

  // Find or generate gray image at appropriate blur level
  const float scaleTolerance = pow (2.0f, -1.0f / 6);  // TODO: parameterize "6", should be 2 * octaveSteps
  ImageCache::shared.add (image);
  PyramidImage * entry = ImageCache::shared.get (ImageCache::monochrome, point.scale);
  if (! entry  ||  scaleTolerance > (entry->scale > point.scale ? point.scale / entry->scale : entry->scale / point.scale))
  {
	entry = ImageCache::shared.getLE (ImageCache::monochrome, point.scale);
	if (! entry)  // No smaller image exists, which means base level image (scale == 0.5) does not exist.
	{
	  entry = ImageCache::shared.add (image * GrayFloat, ImageCache::monochrome);
	}
  }
  float octave = (float) image.width / entry->width;
  PointAffine p = point;
  p.x = (p.x + 0.5f) / octave - 0.5f;
  p.y = (p.y + 0.5f) / octave - 0.5f;
  p.scale /= octave;

  Matrix<double> R = p.rectification ();  // Later, minor changes to R will allow it to function as the rectification from image to histogram bins.
  Matrix<double> S = p.projection ();

  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  float angleOffset;

  if (point.A(0,0) == 1.0f  &&  point.A(0,1) == 0.0f  &&  point.A(1,0) == 0.0f  &&  point.A(1,1) == 1.0f)  // No shape change, so we can work in context of original image.
  {
	// Find or generate derivative images
	PyramidImage * entryX = ImageCache::shared.get (ImageCache::gradientX, entry->scale);
	if (! entryX  ||  entryX->scale != entry->scale)
	{
	  I_x = (*entry) * FiniteDifferenceX ();
	  ImageCache::shared.add (I_x, ImageCache::gradientX, entry->scale);
	}
	else
	{
	  I_x = *entryX;
	}
	PyramidImage * entryY = ImageCache::shared.get (ImageCache::gradientY, entry->scale);
	if (! entryY  ||  entryY->scale != entry->scale)
	{
	  I_y = (*entry) * FiniteDifferenceY ();
	  ImageCache::shared.add (I_y, ImageCache::gradientY, entry->scale);
	}
	else
	{
	  I_y = *entryY;
	}

	// Project the patch into the gradient image in order to find the window
	// for scanning pixels there.

	Vector<double> tl (3);
	tl[0] = -supportRadial;
	tl[1] = supportRadial;
	tl[2] = 1;
	Vector<double> tr (3);
	tr[0] = supportRadial;
	tr[1] = supportRadial;
	tr[2] = 1;
	Vector<double> bl (3);
	bl[0] = -supportRadial;
	bl[1] = -supportRadial;
	bl[2] = 1;
	Vector<double> br (3);
	br[0] = supportRadial;
	br[1] = -supportRadial;
	br[2] = 1;

	Point ptl = S * tl;
	Point ptr = S * tr;
	Point pbl = S * bl;
	Point pbr = S * br;

	sourceL = (int) rint (max (min (ptl.x, ptr.x, pbl.x, pbr.x), 0.0f));
	sourceR = (int) rint (min (max (ptl.x, ptr.x, pbl.x, pbr.x), I_x.width - 1.0f));
	sourceT = (int) rint (max (min (ptl.y, ptr.y, pbl.y, pbr.y), 0.0f));
	sourceB = (int) rint (min (max (ptl.y, ptr.y, pbl.y, pbr.y), I_x.height - 1.0f));

	R.region (0, 0, 1, 2) *= width / (2 * supportRadial);
	R(0,2) += center;
	R(1,2) += center;

	angleOffset = - point.angle;
  }
  else  // Shape change, so we must compute a transformed patch
  {
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (S, scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = (*entry) * t;


	// ensure standard blur level
	/*
	double currentBlur = scale * 0.5 / point.scale;
	currentBlur = max (currentBlur, 0.5);
	double targetBlur = scale;
	if (currentBlur < targetBlur)
	{
	  Gaussian1D blur (sqrt (targetBlur * targetBlur - currentBlur * currentBlur), Boost, GrayFloat, Horizontal);
	  patch *= blur;
	  blur.direction = Vertical;
	  patch *= blur;
	}
	*/

	lastBuffer = 0;
	computeGradient (patch);

	sourceT = 0;
	sourceL = 0;
	sourceB = patchSize - 1;
	sourceR = sourceB;

	R.clear ();
	R(0,0) = (double) width / patchSize;
	R(1,1) = R(0,0);
	R(0,2) = 0.5 * R(0,0) - 0.5;
	R(1,2) = R(0,2);
	R(2,2) = 1;

	angleOffset = 0;
  }

  // Second, gather up the gradient histogram that constitutes the SIFT key.
  Vector<float> result (width * width * angles);
  result.clear ();
  const float binLimit = width - 0.5f;
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  Point q = R * Point (x, y);
	  if (q.x >= -0.5f  &&  q.x < binLimit  &&  q.y >= -0.5f  &&  q.y < binLimit)
	  {
		float dx = I_x(x,y);
		float dy = I_y(x,y);
		float angle = atan2 (dy, dx) + angleOffset;
		angle = mod2pi (angle);
		angle *= angles / (2 * PI);
		float xc = q.x - center;
		float yc = q.y - center;
		float weight = sqrtf (dx * dx + dy * dy) * expf (- (xc * xc + yc * yc) / sigma2);

		int xl = (int) floorf (q.x);
		int xh = xl + 1;
		int yl = (int) floorf (q.y);
		int yh = yl + 1;
		int al = (int) floorf (angle);
		int ah = al + 1;
		if (ah >= angles)
		{
		  ah = 0;
		}

		float xf = q.x - xl;
		float yf = q.y - yl;
		float af = angle - al;

		// Use trilinear method to distribute weight to 8 adjacent bins in histogram.
		if (xl >= 0)
		{
		  float xweight = (1.0f - xf) * weight;
		  if (yl >= 0)
		  {
			float yweight = (1.0f - yf) * xweight;
			result[((xl * width) + yl) * angles + al] += (1.0f - af) * yweight;
			result[((xl * width) + yl) * angles + ah] +=         af  * yweight;
		  }
		  if (yh < width)
		  {
			float yweight = yf * xweight;
			result[((xl * width) + yh) * angles + al] += (1.0f - af) * yweight;
			result[((xl * width) + yh) * angles + ah] +=         af  * yweight;
		  }
		}
		if (xh < width)
		{
		  float xweight = xf * weight;
		  if (yl >= 0)
		  {
			float yweight = (1.0f - yf) * xweight;
			result[((xh * width) + yl) * angles + al] += (1.0f - af) * yweight;
			result[((xh * width) + yl) * angles + ah] +=         af  * yweight;
		  }
		  if (yh < width)
		  {
			float yweight = yf * xweight;
			result[((xh * width) + yh) * angles + al] += (1.0f - af) * yweight;
			result[((xh * width) + yh) * angles + ah] +=         af  * yweight;
		  }
		}
	  }
	}
  }

  result.normalize ();
  bool changed = false;
  for (int i = 0; i < result.rows (); i++)
  {
	if (result[i] > maxValue)
	{
	  result[i] = maxValue;
	  changed = true;
	}
  }
  if (changed)
  {
	result.normalize ();
  }

  return result;
}

inline void
DescriptorSIFT::patch (Canvas * canvas, const Vector<float> & value, int size)
{
  float * length = & value[0];
  Point center;
  Point tip;
  for (int x = 0; x < width; x++)
  {
	center.x = (x + 0.5f) * size;
	for (int y = 0; y < width; y++)
	{
	  center.y = (y + 0.5f) * size;
	  for (int a = 0; a < angles; a++)
	  {
		double angle = a * 2 * PI / angles;
		double radius = (size / 2.0) * (*length++ / maxValue);
		tip.x = center.x + cos (angle) * radius;
		tip.y = center.y + sin (angle) * radius;
		canvas->drawSegment (center, tip, black);
	  }
	}
  }
}

Image
DescriptorSIFT::patch (const Vector<float> & value)
{
  const int size = 16;  // size of one cell
  CanvasImage result (width * size, width * size, GrayChar);
  result.clear (white);
  patch (&result, value, size);
  return result;
}

void
DescriptorSIFT::patch (const string & fileName, const Vector<float> & value)
{
  const int size = 32;  // size of one cell
  int edge = width * size;
  CanvasPS result (fileName, edge, edge);
  patch (&result, value, size);

  result.setLineWidth (0);  // hairline
  for (int i = 0; i <= width; i++)
  {
	result.drawSegment (Point (i * size, 0), Point (i * size, edge), black);
	result.drawSegment (Point (0, i * size), Point (edge, i * size), black);
  }
}

Comparison *
DescriptorSIFT::comparison ()
{
  return new MetricEuclidean (2.0f);
}

void
DescriptorSIFT::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &width,         sizeof (width));
  stream.read ((char *) &angles,        sizeof (angles));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
  stream.read ((char *) &sigmaWeight,   sizeof (sigmaWeight));
  stream.read ((char *) &maxValue,      sizeof (maxValue));

  dimension = width * width * angles;
}

void
DescriptorSIFT::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  stream.write ((char *) &width,         sizeof (width));
  stream.write ((char *) &angles,        sizeof (angles));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &sigmaWeight,   sizeof (sigmaWeight));
  stream.write ((char *) &maxValue,      sizeof (maxValue));
}
