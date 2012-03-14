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
#include "fl/imagecache.h"


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
}

Vector<float>
DescriptorOrientationHistogram::value (ImageCache & cache, const PointAffine & point)
{
  // First, grab the patch and prepare the derivative images I_x and I_y.

  // Find or generate gray image at appropriate blur level
  const float scaleTolerance = pow (2.0f, -1.0f / 6);  // TODO: parameterize "6", should be 2 * octaveSteps
  EntryPyramid * entry = (EntryPyramid *) cache.getClosest (new EntryPyramid (GrayFloat, point.scale));
  if (! entry  ||  scaleTolerance > (entry->scale > point.scale ? point.scale / entry->scale : entry->scale / point.scale))
  {
	entry = (EntryPyramid *) cache.getLE (new EntryPyramid (GrayFloat, point.scale));
	if (! entry)  // No smaller image exists, which means base level image (scale == 0.5) does not exist.
	{
	  entry = (EntryPyramid *) cache.get (new EntryPyramid (GrayFloat));
	}
  }
  const float octave = (float) cache.original->image.width / entry->image.width;
  PointAffine p = point;
  p.x = (p.x + 0.5f) / octave - 0.5f;
  p.y = (p.y + 0.5f) / octave - 0.5f;
  p.scale /= octave;

  Image patch;
  float sigma;
  float radius;
  if (entry->image.width == entry->image.height  &&  p.angle == 0  &&  fabs (2.0f * p.scale * supportRadial - entry->image.width) < 0.5)
  {
	// patch == entire image, so no need to transform
	// Note that the test above should also verify that p is at the center
	// of the image.  However, if the other tests pass, then this is almost
	// certainly the case.
	patch = entry->image;
	radius = p.scale * supportRadial;
	sigma = p.scale;
  }
  else
  {
	int patchSize = 2 * supportPixel;
	const double patchScale = supportPixel / supportRadial;
	Transform t (p.projection (), patchScale);
	t.setWindow (0, 0, patchSize, patchSize);
	patch = entry->image * t;
	radius = supportPixel;
	sigma = supportPixel / supportRadial;
  }

  ImageOf<float> I_x = patch * FiniteDifference (Horizontal);
  ImageOf<float> I_y = patch * FiniteDifference (Vertical);

  // Second, gather up the gradient histogram.
  float * histogram = new float[bins];
  memset (histogram, 0, bins * sizeof (float));
  float radius2 = radius * radius;
  float sigma2 = 2.0 * sigma * sigma;
  Point center ((patch.width - 1) / 2.0, (patch.height - 1) / 2.0);
  for (int y = 0; y < patch.height; y++)
  {
	for (int x = 0; x < patch.width; x++)
	{
	  float cx = x - center.x;
	  float cy = y - center.y;
	  float d2 = cx * cx + cy * cy;
	  if (d2 < radius2)
	  {
		float dx = I_x(x,y);
		float dy = I_y(x,y);
		float angle = atan2 (dy, dx);
		int bin = (int) ((angle + M_PI) * bins / TWOPI);
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
	  angles.insert (make_pair (h1, (i + 0.5f + peak) * TWOPI / bins - M_PI));
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
DescriptorOrientationHistogram::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & supportRadial;
  archive & supportPixel;
  archive & kernelSize;
  archive & bins;
  archive & cutoff;
}
