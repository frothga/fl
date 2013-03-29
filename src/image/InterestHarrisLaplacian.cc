/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestHarrisLaplacian ----------------------------------------------

InterestHarrisLaplacian::InterestHarrisLaplacian (int maxPoints, float thresholdFactor, float neighborhood, float firstScale, float lastScale, int steps, int extraSteps)
{
  this->maxPoints       = maxPoints;
  this->thresholdFactor = thresholdFactor;
  this->firstScale      = firstScale;
  this->lastScale       = lastScale;
  this->steps           = steps;
  this->extraSteps      = extraSteps;

  if (neighborhood > 0)
  {
	neighborhood = ceilf (neighborhood);
  }
  else if (neighborhood == 0)
  {
	neighborhood = 1;
  }
  this->neighborhood = neighborhood;
}

InterestHarrisLaplacian::~InterestHarrisLaplacian ()
{
  clear ();
}

void
InterestHarrisLaplacian::init ()
{
  clear ();

  // Note: as written, both sets of filters are one octave larger than the
  // image they operate on.  This is because Laplacians become ill-conditioned
  // below a scale of 1 (while the default native scale of an image is 0.5).

  // Generate Laplacian filters
  int octaveSteps = steps * extraSteps;
  double stepSize = pow (2.0, 1.0 / octaveSteps);
  for (int s = 0; s < octaveSteps + extraSteps; s++)
  {
	double scale = 0.5 * pow (stepSize, s);
	Laplacian * l = new Laplacian (scale);
	(*l) *= scale * scale;
	laplacians.push_back (l);
  }

  // Generate Harris filters
  for (int s = 1; s <= steps; s++)
  {
	double scale = laplacians[s*extraSteps]->sigma;
	filters.push_back (new FilterHarris (scale, scale * M_SQRT2));  // sigmaI seems to be the truer representative of characteristic scale; multiply by sqrt(2) to match Laplacian
  }
}

void
InterestHarrisLaplacian::clear ()
{
  for (int i = 0; i < filters   .size (); i++) delete filters   [i];
  for (int i = 0; i < laplacians.size (); i++) delete laplacians[i];

  filters.clear ();
  laplacians.clear ();
}

void
InterestHarrisLaplacian::run (ImageCache & cache, PointSet & result)
{
  if (filters.size () == 0) init ();

  multiset<PointInterest> sorted;

  float originalScale  = cache.original->scale;
  int   originalWidth  = cache.original->image.width;
  int   originalHeight = cache.original->image.height;
  int ratio = 0x1 << max (0, EntryPyramid::octave (firstScale, originalScale));

  bool done = false;
  while (! done)
  {
	int width  = originalWidth  / ratio;
	Image work = cache.get (new EntryPyramid (GrayFloat, originalScale * ratio, width))->image;

	for (int i = 0; i < filters.size (); i++)
	{
	  FilterHarris & filter = *filters[i];
	  float scale = filter.sigmaI * ratio;
	  if (scale < firstScale) continue;

	  ImageOf<float> filtered = work * filter;
	  if (scale > lastScale  ||  filtered.width == 0  ||  filtered.height == 0)
	  {
		done = true;
		break;
	  }

	  int nmsSize;
	  if (neighborhood < 0)
	  {
		nmsSize = (int) ceilf (-neighborhood * filter.sigmaI);
	  }
	  else
	  {
		nmsSize = (int) neighborhood;
	  }
	  NonMaxSuppress nms (nmsSize);
	  filtered *= nms;

	  IntensityStatistics stats (true);
	  filtered * stats;
	  float threshold = stats.deviation (0) * thresholdFactor;

	  for (int y = 0; y < filtered.height; y++)
	  {
		for (int x = 0; x < filtered.width; x++)
		{
		  float pixel = filtered (x, y);
		  if (pixel > threshold  &&  (sorted.size () < maxPoints  ||  pixel > sorted.begin ()->weight))
		  {
			PointInterest p;
			p.x = x + filter.offset;
			p.y = y + filter.offset;

			int l = i * extraSteps;
			int h = l + 2 * extraSteps;
			vector<float> r (h - l);
			for (int j = l; j < h; j++)
			{
			  r[j - l] = abs (laplacians[j]->response (work, p));
			}

			p.weight = 0;
			p.scale = 0;
			for (int j = 1; j < r.size () - 1; j++)
			{
			  if (r[j] > r[j-1]  &&  r[j] > r[j+1]  &&  r[j] > p.weight)
			  {
				p.weight = r[j];
				p.scale = laplacians[j + l]->sigma;
			  }
			}

			if (p.scale > 0)
			{
			  p.x = (p.x + 0.5f) * ratio - 0.5f;
			  p.y = (p.y + 0.5f) * ratio - 0.5f;
			  p.scale *= M_SQRT2 * ratio;
			  p.weight = pixel;
			  p.detector = PointInterest::Corner;
			  sorted.insert (p);
			  if (sorted.size () > maxPoints)
			  {
				sorted.erase (sorted.begin ());
			  }
			}
		  }
		}
	  }
	}

	ratio *= 2;
  }

  result.add (sorted);
}

void
InterestHarrisLaplacian::serialize (Archive & archive, uint32_t version)
{
  archive & *((InterestOperator *) this);
  archive & maxPoints;
  archive & thresholdFactor;
  archive & neighborhood;
  archive & firstScale;
  archive & lastScale;
  archive & steps;
  archive & extraSteps;

  if (archive.in) clear ();
}
