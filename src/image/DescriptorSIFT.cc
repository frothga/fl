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


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/color.h"
#include "fl/imagecache.h"


using namespace fl;
using namespace std;


// class DescriptorSIFT ------------------------------------------------------

DescriptorSIFT::DescriptorSIFT (int width, int angles)
{
  this->width = width;
  this->angles = angles;
  angleRange = TWOPIf;
  dimension = width * width * angles;

  supportRadial = 3 * width / 2.0f;  // Causes each bin to cover 3 sigmas.
  supportPixel = (int) ceilf (2 * supportRadial);  // Causes drawn off patch to hold 2 pixels per sigma.
  sigmaWeight = width / 2.0;
  maxValue = 0.2f;

  init ();
}

DescriptorSIFT::~DescriptorSIFT ()
{
  map<int, ImageOf<float> *>::iterator it;
  for (it = kernels.begin (); it != kernels.end (); it++)
  {
	delete it->second;
  }
}

void
DescriptorSIFT::init ()
{
  dimension = width * width * angles;
  angleStep = (angleRange / angles) + 1e-6;

  map<int, ImageOf<float> *>::iterator it;
  for (it = kernels.begin (); it != kernels.end (); it++)
  {
	delete it->second;
  }
  kernels.clear ();
}

float *
DescriptorSIFT::getKernel (int size)
{
  map<int, ImageOf<float> *>::iterator it = kernels.find (size);
  if (it == kernels.end ())
  {
	const float center = (width - 1) / 2.0f;
	const float sigma2 = 2.0f * sigmaWeight * sigmaWeight;
	const float keyScale = (float) width / size;
	const float keyOffset = 0.5f * keyScale - 0.5f;

	ImageOf<float> * G = new ImageOf<float> (size, size, GrayFloat);
	it = kernels.insert (make_pair (size, G)).first;
	float * g = & (*G)(0,0);

	float qy = keyOffset;
	for (int y = 0; y < size; y++)
	{
	  float qx = keyOffset;
	  float yc = qy - center;
	  for (int x = 0; x < size; x++)
	  {
		float xc = qx - center;
		*g++ = expf (- (xc * xc + yc * yc) / sigma2);
		qx += keyScale;
	  }
	  qy += keyScale;
	}
  }

  return & (* it->second)(0,0);
}

Vector<float>
DescriptorSIFT::value (const Image & image, const PointAffine & point)
{
  // First, grab the patch and prepeare the derivative images I_x and I_y.

  // Find or generate gray image at appropriate blur level
  const float scaleTolerance = pow (2.0f, -1.0f / 6);  // TODO: parameterize "6", should be 2 * octaveSteps
  ImageCache::shared.setOriginal (image);
  EntryPyramid * entry = (EntryPyramid *) ImageCache::shared.getClosest (new EntryPyramid (GrayFloat, point.scale));
  if (! entry  ||  scaleTolerance > (entry->scale > point.scale ? point.scale / entry->scale : entry->scale / point.scale))
  {
	entry = (EntryPyramid *) ImageCache::shared.getLE (new EntryPyramid (GrayFloat, point.scale));
	if (! entry)  // No smaller image exists, which means base level image (scale == 0.5) does not exist.
	{
	  entry = (EntryPyramid *) ImageCache::shared.get (new EntryPyramid (GrayFloat));
	}
  }
  const float octave = (float) image.width / entry->image.width;
  PointAffine p = point;
  p.x = (p.x + 0.5f) / octave - 0.5f;
  p.y = (p.y + 0.5f) / octave - 0.5f;
  p.scale /= octave;

  Image patch;
  if (entry->image.width == entry->image.height  &&  p.angle == 0  &&  fabs (2.0f * p.scale * supportRadial - entry->image.width) < 0.5)
  {
	// patch == entire image, so no need to transform
	// Note that the test above should also verify that p is at the center
	// of the image.  However, if the other tests pass, then this is almost
	// certainly the case.
	patch = entry->image;
  }
  else
  {
	int patchSize = 2 * supportPixel;
	const double patchScale = supportPixel / supportRadial;
	Transform t (p.projection (), patchScale);
	t.setWindow (0, 0, patchSize, patchSize);
	patch = entry->image * t;
  }
  float * g = getKernel (patch.width);

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

  I_x = patch * fdX;
  I_y = patch * fdY;

  const float keyScale = (float) width / patch.width;
  const float keyOffset = 0.5f * keyScale - 0.5f;

  const int rowStep = width * angles;

  // Second, gather up the gradient histogram that constitutes the SIFT key.
  Vector<float> result (width * width * angles);
  result.clear ();
  float * r = & result[0];
  float * dx = & I_x(0,0);
  float * dy = & I_y(0,0);
  float qy = keyOffset;
  for (int y = 0; y < I_x.height; y++)
  {
	float qx = keyOffset;

	const int yl = (int) floorf (qy);
	const int yh = yl + 1;
	const float yf = qy - yl;
	const float yf1 = 1.0f - yf;

	// Variable naming scheme for pointers: r{row}{column}
	// Below are base pointers for the rows...
	float * const rl = r + yl * rowStep;
	float * const rh = rl + rowStep;

	for (int x = 0; x < I_x.width; x++)
	{
	  float angle = atan2 (*dy, *dx);
	  if (angle < 0.0f) angle += angleRange;
	  angle /= angleStep;
	  const float weight = sqrtf (*dx * *dx + *dy * *dy) * *g++;
	  dx++;
	  dy++;

	  const int xl = (int) floorf (qx);
	  const int xh = xl + 1;
	  const float xf = qx - xl;

	  const int al = (int) floorf (angle);
	  int ah = al + 1;
	  if (ah >= angles)
	  {
		ah = 0;
	  }
	  const float af = angle - al;
	  const float af1 = 1.0f - af;

	  // Use trilinear method to distribute weight to 8 adjacent bins in histogram.
	  int step = xl * angles;
	  if (xl >= 0)
	  {
		const float xweight = (1.0f - xf) * weight;
		if (yl >= 0)
		{
		  float yweight = yf1 * xweight;
		  float * rll = rl + step;
		  rll[al] += af1 * yweight;
		  rll[ah] += af  * yweight;
		}
		if (yh < width)
		{
		  float yweight = yf * xweight;
		  float * rhl = rh + step;
		  rhl[al] += af1 * yweight;
		  rhl[ah] += af  * yweight;
		}
	  }
	  step += angles;
	  if (xh < width)
	  {
		const float xweight = xf * weight;
		if (yl >= 0)
		{
		  float yweight = yf1 * xweight;
		  float * rlh = rl + step;
		  rlh[al] += af1 * yweight;
		  rlh[ah] += af  * yweight;
		}
		if (yh < width)
		{
		  float yweight = yf * xweight;
		  float * rhh = rh + step;
		  rhh[al] += af1 * yweight;
		  rhh[ah] += af  * yweight;
		}
	  }

	  qx += keyScale;
	}
	qy += keyScale;
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
  bool nosign = fabs (angleRange - M_PI) < 1e-6;
  for (int y = 0; y < width; y++)
  {
	center.y = (y + 0.5f) * size;
	for (int x = 0; x < width; x++)
	{
	  center.x = (x + 0.5f) * size;
	  for (int a = 0; a < angles; a++)
	  {
		double angle = a * angleRange / angles;
		double radius = (size / 2.0) * (*length++ / maxValue);
		tip.x = center.x + cos (angle) * radius;
		tip.y = center.y + sin (angle) * radius;
		canvas->drawSegment (center, tip, BLACK);
		if (nosign)
		{
		  tip.x = center.x - cos (angle) * radius;
		  tip.y = center.y - sin (angle) * radius;
		  canvas->drawSegment (center, tip, BLACK);
		}
	  }
	}
  }
}

Image
DescriptorSIFT::patch (const Vector<float> & value)
{
  const int size = 16;  // size of one cell
  CanvasImage result (width * size, width * size, GrayChar);
  result.clear (WHITE);
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
	result.drawSegment (Point (i * size, 0), Point (i * size, edge), BLACK);
	result.drawSegment (Point (0, i * size), Point (edge, i * size), BLACK);
  }
}

Comparison *
DescriptorSIFT::comparison ()
{
  return new MetricEuclidean (2.0f);
}

void
DescriptorSIFT::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & width;
  archive & angles;
  archive & angleRange;
  archive & supportRadial;
  archive & supportPixel;
  archive & sigmaWeight;
  archive & maxValue;

  if (archive.in) init ();
}
