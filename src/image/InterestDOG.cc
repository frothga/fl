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


#include "fl/interest.h"
#include "fl/lapack.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestDOG ----------------------------------------------------------

InterestDOG::InterestDOG (float firstScale, float lastScale, int extraSteps)
{
  this->firstScale = firstScale;
  this->lastScale  = lastScale;
  this->steps      = extraSteps;

  crop = (int) roundp (Gaussian2D::cutoff);
  thresholdEdge = 0.06f;
  thresholdPeak = 0.04f / steps;

  fast = false;
}

inline bool
InterestDOG::isLocalMax (float value, ImageOf<float> & dog, int x, int y)
{
  if (value > 0)
  {
	return    dog(x-1,y-1) < value
	       && dog(x-1,y  ) < value
	       && dog(x-1,y+1) < value
	       && dog(x  ,y-1) < value
	       && dog(x  ,y+1) < value
	       && dog(x+1,y-1) < value
	       && dog(x+1,y  ) < value
	       && dog(x+1,y+1) < value;
  }
  else
  {
	return    dog(x-1,y-1) > value
	       && dog(x-1,y  ) > value
	       && dog(x-1,y+1) > value
	       && dog(x  ,y-1) > value
	       && dog(x  ,y+1) > value
	       && dog(x+1,y-1) > value
	       && dog(x+1,y  ) > value
	       && dog(x+1,y+1) > value;
  }
}

inline bool
InterestDOG::notOnEdge (ImageOf<float> & dog, int x, int y)
{
  float H00 = dog(x-1,y) - 2.0f * dog(x,y) + dog(x+1,y);
  float H11 = dog(x,y-1) - 2.0f * dog(x,y) + dog(x,y+1);
  float H01 = ((dog(x+1,y+1) - dog(x+1,y-1)) - (dog(x-1,y+1) - dog(x-1,y-1))) / 4.0f;
  float det = H00 * H11 - H01 * H01;
  float trace = H00 + H11;
  return det > thresholdEdge * trace * trace;
}

float
InterestDOG::fitQuadratic (ImageOf<float> & dog0, ImageOf<float> & dog1, ImageOf<float> & dog2, int x, int y, Vector<float> & result)
{
  Vector<float> g (3);
  g[0] = (dog2(x,  y  ) - dog0(x,  y  )) / 2.0f;
  g[1] = (dog1(x+1,y  ) - dog1(x-1,y  )) / 2.0f;
  g[2] = (dog1(x,  y+1) - dog1(x,  y-1)) / 2.0f;

  Matrix<float> H (3, 3);
  H(0,0) = dog0(x,  y  ) - 2.0f * dog1(x,y) + dog2(x,  y  );
  H(1,1) = dog1(x-1,y  ) - 2.0f * dog1(x,y) + dog1(x+1,y  );
  H(2,2) = dog1(x,  y-1) - 2.0f * dog1(x,y) + dog1(x,  y+1);
  H(0,1) = ((dog2(x+1,y  ) - dog2(x-1,y  )) - (dog0(x+1,y  ) - dog0(x-1,y  ))) / 4.0f;
  H(0,2) = ((dog2(x,  y+1) - dog2(x,  y-1)) - (dog0(x,  y+1) - dog0(x,  y-1))) / 4.0f;
  H(1,2) = ((dog1(x+1,y+1) - dog1(x+1,y-1)) - (dog1(x-1,y+1) - dog1(x-1,y-1))) / 4.0f;
  H(1,0) = H(0,1);
  H(2,0) = H(0,2);
  H(2,1) = H(1,2);

  gelss (H, result, g * -1.0f);

  return dog1(x,y) + 0.5f * result.dot (g);
}

void
InterestDOG::run (ImageCache & cache, InterestPointSet & result)
{
  Stopwatch timer;

  multiset<PointInterest> sorted;

  // Create our own preblur, to prevent operation from being broken into
  // several steps by image cache mechanism.
  EntryPyramid * entry = (EntryPyramid *) cache.getLE (new EntryPyramid (GrayFloat, firstScale));
  if (! entry) entry = (EntryPyramid *) cache.get (new EntryPyramid (GrayFloat));
  if (entry->scale < firstScale)
  {
	Gaussian1D blur (sqrt (firstScale * firstScale - entry->scale * entry->scale), Boost, GrayFloat, Horizontal);
	Image temp = entry->image * blur;
	blur.direction = Vertical;
	temp *= blur;
	cache.get (new EntryPyramid (temp, firstScale));
  }

  // Step thru octaves until image is too small to process
  int originalWidth  = cache.original->image.width;
  int originalHeight = cache.original->image.height;
  float scaleRatio = pow (2.0, 1.0 / steps);
  int minsize = 2 * crop + 3;
  int ratio = 1;
  while (ratio * firstScale <= lastScale)
  {
	int width  = originalWidth  / ratio;
	int height = originalHeight / ratio;
	if (width < minsize  ||  height < minsize) break;

	// Collect DOG images
	vector<EntryDOG *> dogs (steps + 2);
	float scale = firstScale * ratio;
	for (int i = 0; i < steps + 2; i++)
	{
	  float nextScale = scale * scaleRatio;
	  dogs[i] = (EntryDOG *) cache.get (new EntryDOG (nextScale, scale, width));
	  scale = nextScale;
	}

	// Search for maxima in DoG at each scale level
	for (int i = 0; i < steps; i++)
	{
	  ImageOf<float> dog0 = dogs[i  ]->image;
	  ImageOf<float> dog1 = dogs[i+1]->image;
	  ImageOf<float> dog2 = dogs[i+2]->image;
	  float scale0 = dogs[i  ]->scale;
	  float scale1 = dogs[i+1]->scale;
	  float scale2 = dogs[i+2]->scale;
	  for (int y = crop; y < dog1.height - crop; y++)
	  {
		for (int x = crop; x < dog1.width - crop; x++)
		{
		  float & value = dog1(x,y);
		  if (   fabsf (value) > 0.8 * thresholdPeak
			  && isLocalMax (value, dog0, x, y)
			  && isLocalMax (value, dog1, x, y)
			  && isLocalMax (value, dog2, x, y)
			  && notOnEdge (dog1, x, y))
		  {
			// Search for precise location of maximum using interpolation.
			Vector<float> offset;
			int u = x;
			int v = y;
			float peak;
			int count = 0;
			bool changed = true;
			while (changed  &&  count++ < 5)
			{
			  int oldu = u;
			  int oldv = v;
			  peak = fitQuadratic (dog0, dog1, dog2, u, v, offset);
			  if (offset[1] > 0.6f  &&  u < dog1.width - crop)
			  {
				u++;
			  }
			  if (offset[1] < -0.6f  &&  u > crop)
			  {
				u--;
			  }
			  if (offset[2] > 0.6f  &&  v < dog1.height - crop)
			  {
				v++;
			  }
			  if (offset[2] < -0.6f  &&  v > crop)
			  {
				v--;
			  }
			  changed =  u != oldu  ||  v != oldv;
			}

			// Store the point if quadratic interpolation returned reasonable
			// results and the DoG response is strong enough.
			if (fabs (offset[0]) < 1.5f  &&  fabs (offset[1]) < 1.5f  &&  fabs (offset[2]) < 1.5f  &&  fabs (peak) > thresholdPeak)
			{
			  PointInterest p;
			  if (offset[0] > 0) p.scale =  offset[0] * (scale2 - scale1) + scale1;
			  else               p.scale = -offset[0] * (scale0 - scale1) + scale1;
			  p.x = (x + offset[1] + 0.5f) * ratio - 0.5f;  // Coordinates all use center-of-pixel convention.  Must shift to left edge of pixel to properly overly images w/ different scales.
			  p.y = (y + offset[2] + 0.5f) * ratio - 0.5f;
			  p.weight = abs (peak);
			  p.detector = PointInterest::Blob;
			  sorted.insert (p);
			}
		  }
		}
	  }
	}

 	ratio *= 2;
  }

  result.add (sorted);

  cerr << "DoG time = " << timer << endl;
}

void
InterestDOG::serialize (Archive & archive, uint32_t version)
{
  archive & *((InterestOperator *) this);
  archive & firstScale;
  archive & lastScale;
  archive & steps;
  archive & crop;
  archive & thresholdEdge;
  archive & thresholdPeak;
  archive & fast;
}
