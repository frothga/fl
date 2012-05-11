/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"

#include <float.h>


using namespace std;
using namespace fl;


// class DescriptorSpatialPyramid ---------------------------------------------

DescriptorSpatialPyramid::DescriptorSpatialPyramid (int levels, Descriptor * descriptor, ClusterMethod * cluster)
: levels (levels),
  descriptor (descriptor),
  cluster (cluster)
{
  firstScale = 1;
  lastScale = INFINITY;
  substeps = 2;
}

DescriptorSpatialPyramid::~DescriptorSpatialPyramid ()
{
  if (descriptor) delete descriptor;
  if (cluster) delete cluster;
}

Vector<float>
DescriptorSpatialPyramid::value (ImageCache & cache, const PointAffine & point)
{
  throw "DescriptorSpatialPyramid only works on whole images, not specific points.";
}

Vector<float>
DescriptorSpatialPyramid::value (ImageCache & cache)
{
  if (! cluster) throw "ClusterMethod must be set before call to value()";
  if (! descriptor) throw "Descriptor must be set before call to extract()";

  const int classCount = cluster->classCount ();
  const int w = cache.original->image.width;
  const int h = cache.original->image.height;

  // Quantize descriptors and accumulate histogram
  int d = 2;  // spatial dimension
  int ratio = pow (2, d);
  int histogramCount = (1 - pow (ratio, levels)) / (1 - ratio);
  Vector<float> result (histogramCount * classCount);
  result.clear ();

  PointAffine p;
  for (p.scale = firstScale; p.scale <= lastScale; p.scale *= 2)
  {
	// Tesselate image
	float sp = descriptor->supportRadial * p.scale * 2;
	float l = sp - 0.5;
	float t = sp - 0.5;
	float r = w - 0.5 - sp;
	float b = h - 0.5 - sp;
	if (r < l  ||  b < t) break;
	float step = sp / substeps;
	float stepsX = ceil ((r - l) / step);
	float stepX  = (r - l) / stepsX;
	float stepsY = ceil ((b - t) / step);
	float stepY  = (b - t) / stepsY;
	r += FLT_EPSILON;  // for rouding errors during loop below
	b += FLT_EPSILON;
	//cerr << p.scale << " " << l << " " << t << " " << r << " " << b << " " << stepX << " " << stepY << " " << stepsX << " " << stepsY << endl;

	// Gather descriptors and increment histogram bins
	cache.get (new EntryPyramid (GrayFloat, p.scale));  // Force generation of scale pyramid
	for (p.y = t; p.y <= b; p.y += stepY)
	{
	  for (p.x = l; p.x <= r; p.x += stepX)
	  {
		Vector<float> value = descriptor->value (cache, p);
		if (value.rows ())
		{
		  int c = cluster->classify (value);
		  result[c]++;
		  int base = 1;
		  int increment = ratio;
		  for (int level = 1; level < levels; level++)
		  {
			int steps = 1 << level;
			int binX = (int) floor (p.x * steps / w);
			int binY = (int) floor (p.y * steps / h);
			result[(base + binY * steps + binX) * classCount + c]++;
			base += increment;
			increment *= ratio;
		  }
		}
	  }
	}
  }

  // Normalize
  float total = result.region (0, 0, classCount-1, 0).norm (1);
  if (total) result /= total;
  int base = 0;
  int increment = 1;
  float weight = pow (2.0, 1 - levels);
  for (int level = 0; level < levels; level++)
  {
	int nextBase = base + increment;
	result.region (base * classCount, 0, nextBase * classCount - 1, 0) *= weight;
	base = nextBase;
	increment *= ratio;
	if (level) weight *= 2;
  }

  return result;
}

/**
   This code duplicates the outer loop of value(ImageCache).  This is a
   convenience function that gives a calling program exactly the same set
   of descriptors that value() works with.
 **/
void
DescriptorSpatialPyramid::extract (ImageCache & cache, vector<Vector<float> > & descriptors)
{
  if (! descriptor) throw "Descriptor must be set before call to extract()";

  int originalWidth  = cache.original->image.width;
  int originalHeight = cache.original->image.height;

  // Extract descriptors at each level
  PointAffine p;
  for (p.scale = firstScale; p.scale <= lastScale; p.scale *= 2)
  {
	// Tesselate image
	float sp = descriptor->supportRadial * p.scale * 2;
	float l = sp - 0.5;
	float t = sp - 0.5;
	float r = originalWidth  - 0.5 - sp;
	float b = originalHeight - 0.5 - sp;
	if (r < l  ||  b < t) break;
	float step = sp / substeps;
	float stepsX = ceil ((r - l) / step);
	float stepX  = (r - l) / stepsX;
	float stepsY = ceil ((b - t) / step);
	float stepY  = (b - t) / stepsY;
	r += FLT_EPSILON;  // for rouding errors during loop below
	b += FLT_EPSILON;
	//cerr << p.scale << " " << l << " " << t << " " << r << " " << b << " " << stepX << " " << stepY << " " << stepsX << " " << stepsY << endl;

	// Gather descriptors
	cache.get (new EntryPyramid (GrayFloat, p.scale));  // Force generation of scale pyramid
	for (p.y = t; p.y <= b; p.y += stepY)
	{
	  for (p.x = l; p.x <= r; p.x += stepX)
	  {
		Vector<float> value = descriptor->value (cache, p);
		if (value.rows ()) descriptors.push_back (value);
	  }
	}
  }
}

Comparison *
DescriptorSpatialPyramid::comparison ()
{
  return new PyramidMatchKernel (levels);
}

int
DescriptorSpatialPyramid::dimension ()
{
  int classCount = cluster->classCount ();
  int d = 2;  // spatial dimension
  int r = pow (2, d);
  int histogramCount = (1 - pow (r, levels)) / (1 - r);
  return histogramCount * classCount;
}

void
DescriptorSpatialPyramid::serialize (Archive & archive, uint32_t version)
{
  if (archive.in)
  {
	if (descriptor) delete descriptor;
	if (cluster   ) delete cluster;
  }

  archive & *((Descriptor *) this);
  archive & levels;
  archive & descriptor;
  archive & cluster;
  archive & firstScale;
  archive & lastScale;
  archive & substeps;
}


// PyramidMatchKernel ---------------------------------------------------------

float
PyramidMatchKernel::value (const Vector<float> & value1, const Vector<float> & value2) const
{
  int count = value1.rows ();
  assert (count == value2.rows ());
  float result = 0;
  for (int i = 0; i < count; i++)
  {
	result += min (value1[i], value2[i]);
  }
  return result;
}
