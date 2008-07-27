/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"


using namespace fl;
using namespace std;


// class DescriptorOrientationHistogram ---------------------------------------

DescriptorOrientationHistogram::DescriptorOrientationHistogram (float supportRadial, int supportPixel, float kernelSize, int bins)
{
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;
  this->kernelSize    = kernelSize;
  this->bins          = bins;

  cutoff = 0.8f;

  lastBuffer = 0;
}

DescriptorOrientationHistogram::DescriptorOrientationHistogram (istream & stream)
{
  lastBuffer = 0;
  read (stream);
}

void
DescriptorOrientationHistogram::computeGradient (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "DescriptorOrientationHistogram only handles packed buffers for now";

  if (lastBuffer == (void *) imageBuffer->memory  &&  lastTime == image.timestamp)
  {
	return;
  }
  lastBuffer = (void *) imageBuffer->memory;
  lastTime = image.timestamp;

  Image work = image * GrayFloat;
  I_x = work * FiniteDifferenceX ();
  I_y = work * FiniteDifferenceY ();
}

Vector<float>
DescriptorOrientationHistogram::value (const Image & image, const PointAffine & point)
{
  // First, prepeare the derivative images I_x and I_y.
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  Point center;
  float sigma;
  float radius;
  if (point.A(0,0) == 1.0f  &&  point.A(0,1) == 0.0f  &&  point.A(1,0) == 0.0f  &&  point.A(1,1) == 1.0f)  // No shape change, so we can work in context of original image.
  {
	ImageCache::shared.add (image);
	PyramidImage * entry = ImageCache::shared.get (ImageCache::monochrome, point.scale);
	if (! entry) throw "Could not find cached image";
	float octave = (float) image.width / entry->width;
	PointAffine p = point;
	p.x = (p.x + 0.5f) / octave - 0.5f;
	p.y = (p.y + 0.5f) / octave - 0.5f;
	p.scale /= octave;

	computeGradient (*entry);

	radius = p.scale * supportRadial;
	sourceL = (int) floorf (p.x - radius);
	sourceR = (int) ceilf  (p.x + radius);
	sourceT = (int) floorf (p.y - radius);
	sourceB = (int) ceilf  (p.y + radius);

	sourceL = max (sourceL, 0);
	sourceR = min (sourceR, I_x.width - 1);
	sourceT = max (sourceT, 0);
	sourceB = min (sourceB, I_x.height - 1);

	center = p;
	sigma = p.scale;
  }
  else  // Shape change, so we must compute a transformed patch
  {
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (point.projection (), scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = image * t;
	patch *= GrayFloat;

	// Add necessary blur.  Assume blur level in source image is 0.5.
	// The current blur level in the transformed patch depends on
	// the ratio of scale to point.scale.
	double currentBlur = scale * 0.5 / point.scale;
	currentBlur = max (currentBlur, 0.5);
	double targetBlur = supportPixel / kernelSize;
	if (currentBlur < targetBlur)
	{
	  Gaussian1D blur (sqrt (targetBlur * targetBlur - currentBlur * currentBlur), Boost, GrayFloat, Horizontal);
	  patch *= blur;
	  blur.direction = Vertical;
	  patch *= blur;
	}

	lastBuffer = 0;
	computeGradient (patch);

	sourceT = 0;
	sourceL = 0;
	sourceB = patchSize - 1;
	sourceR = sourceB;

	center.x = supportPixel - 0.5f;
	center.y = center.x;
	sigma = supportPixel / supportRadial;
	radius = supportPixel;
  }

  // Second, gather up the gradient histogram.
  float * histogram = new float[bins];
  memset (histogram, 0, bins * sizeof (float));
  float radius2 = radius * radius;
  float sigma2 = 2.0 * sigma * sigma;
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  float cx = x - center.x;
	  float cy = y - center.y;
	  float d2 = cx * cx + cy * cy;
	  if (d2 < radius2)
	  {
		float dx = I_x(x,y);
		float dy = I_y(x,y);
		float angle = atan2 (dy, dx);
		int bin = (int) ((angle + PI) * bins / (2 * PI));
		bin = min (bin, bins - 1);  // Technically, these two lines should not be necessary.  They compensate for numerical jitter.
		bin = max (bin, 0);
		float weight = sqrtf (dx * dx + dy * dy) * expf (- (cx * cx + cy * cy) / sigma2);
		histogram[bin] += weight;
	  }
	}
  }

  // Smooth histogram
  for (int i = 0; i < 6; i++)
  {
	float previous = histogram[bins - 1];
	for (int j = 0; j < bins; j++)
	{
      float current = histogram[j];
      histogram[j] = (previous + current + histogram[(j + 1) % bins]) / 3.0;
      previous = current;
	}
  }

  // Find max value
  float maximum = 0;
  for (int i = 0; i < bins; i++)
  {
	maximum = max (maximum, histogram[i]);
  }
  float threshold = cutoff * maximum;

  // Collect peaks
  map<float, float> angles;
  for (int i = 0; i < bins; i++)
  {
	float h0 = histogram[(i == 0 ? bins : i) - 1];
	float h1 = histogram[i];
	float h2 = histogram[(i + 1) % bins];
	if (h1 > h0  &&  h1 > h2  &&  h1 >= threshold)
	{
	  float peak = 0.5f * (h0 - h2) / (h0 - 2.0f * h1 + h2);
	  angles.insert (make_pair (h1, (i + 0.5f + peak) * 2 * PI / bins - PI));
	}
  }
  delete [] histogram;

  // Store peak angles in result
  Vector<float> result (angles.size ());
  float * r = & result[0];
  map<float, float>::reverse_iterator ri;
  for (ri = angles.rbegin (); ri != angles.rend (); ri++)
  {
	*r++ = ri->second;
  }
  return result;
}

Image
DescriptorOrientationHistogram::patch (const Vector<float> & value)
{
  CanvasImage result;

  // TODO: actually implement this method!

  result.clear ();
  return result;
}

void
DescriptorOrientationHistogram::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
  stream.read ((char *) &kernelSize,    sizeof (kernelSize));
  stream.read ((char *) &bins,          sizeof (bins));
  stream.read ((char *) &cutoff,        sizeof (cutoff));
}

void
DescriptorOrientationHistogram::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &kernelSize,    sizeof (kernelSize));
  stream.write ((char *) &bins,          sizeof (bins));
  stream.write ((char *) &cutoff,        sizeof (cutoff));
}
