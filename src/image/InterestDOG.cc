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
InterestDOG::fitQuadratic (vector< ImageOf<float> > & dogs, int s, int x, int y, Vector<float> & result)
{
  ImageOf<float> & dog0 = dogs[s-1];
  ImageOf<float> & dog1 = dogs[s];
  ImageOf<float> & dog2 = dogs[s+1];

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

/**
   Efficiently subtract image b from image a, where both consist of floats.
 **/
static inline Image
difference (const Image & a, const Image & b)
{
  int w = a.width;
  int h = a.height;
  Image result (w, h, GrayFloat);

  float * source1 = (float *) ((PixelBufferPacked *) a     .buffer)->memory;
  float * source2 = (float *) ((PixelBufferPacked *) b     .buffer)->memory;
  float * dest    = (float *) ((PixelBufferPacked *) result.buffer)->memory;
  float * end     = dest + w * h;
  while (dest < end)
  {
	*dest++ = *source1++ - *source2++;
  }

  return result;
}

void
InterestDOG::run (ImageCache & cache, InterestPointSet & result)
{
  Stopwatch timer;

  multiset<PointInterest> sorted;

  EntryPyramid * entry = (EntryPyramid *) cache.get (new EntryPyramid (GrayFloat));
  ImageOf<float> work = entry->image;
  if (firstScale != 0.5f)  // The blur level of a raw image is defined to be 0.5
  {
	Gaussian1D blur (sqrt (firstScale * firstScale - 0.25), Boost, GrayFloat, Horizontal);
	work *= blur;
	blur.direction = Vertical;
	work *= blur;
	cache.get (new EntryPyramid (work, firstScale));
  }

  // Make a set of blurring kernels, one for each step of blurring while
  // processing one octave.  There are actually two more steps than a full
  // octave in order to accomodate search for scale space maxima in DoG images.
  vector<Gaussian1D> blurs (steps + 2);
  float scaleRatio = powf (2.0f, 1.0f / steps);
  float sigmaRatio = sqrtf (scaleRatio * scaleRatio - 1.0f);  // amount of blurring needed to go from current scale step to next scale step
  float scale = firstScale;
  for (int i = 0; i < steps + 2; i++)
  {
	blurs[i] = Gaussian1D (scale * sigmaRatio, Boost, GrayFloat, Horizontal);
	scale *= scaleRatio;
  }

  // Step thru octaves until image is too small to process
  int minsize = 2 * crop + 3;
  float octave = 1.0f;  // actually, this variable stores 2^octave
  vector<Image> blurred (steps + 3);
  vector< ImageOf<float> > dogs (steps + 2);
  vector< ImageOf<float> > smalldogs (2);
  Decimate decimate (2);
  while (work.width >= minsize  &&  work.height >= minsize  &&  octave * firstScale <= lastScale)
  {
	if (fast)
	{
	  if (octave <= 1.0f)
	  {
		for (int i = 0; i < steps; i++)
		{
		  blurred[i] = work;
		  Gaussian1D & blur = blurs[i];
		  blur.direction = Horizontal;
		  work *= blur;
		  blur.direction = Vertical;
		  work *= blur;
		  dogs[i] = difference (blurred[i], work);
		  cache.get (new EntryPyramid (work, octave * firstScale * powf (scaleRatio, i+1)));
		}
	  }
	  else
	  {
		dogs[0] = smalldogs[0];
		dogs[1] = smalldogs[1];
		work = blurred[steps+2];
		for (int i = 2; i < steps; i++)
		{
		  blurred[i] = work;
		  Gaussian1D & blur = blurs[i];
		  blur.direction = Horizontal;
		  work *= blur;
		  blur.direction = Vertical;
		  work *= blur;
		  dogs[i] = difference (blurred[i], work);
		  cache.get (new EntryPyramid (work, octave * firstScale * powf (scaleRatio, i+1)));
		}
	  }
	  int w = work.width;
	  int h = work.height;
	  work *= decimate;
	  for (int i = steps; i < steps + 2; i++)
	  {
		blurred[i] = work;
		Gaussian1D & blur = blurs[i-steps];
		blur.direction = Horizontal;
		work *= blur;
		blur.direction = Vertical;
		work *= blur;
		smalldogs[i-steps] = difference (blurred[i], work);
		dogs[i] = smalldogs[i-steps] * DoubleSize (w % 2, h % 2);
		cache.get (new EntryPyramid (blurred[i], octave * firstScale * powf (scaleRatio, i)));
	  }
	  blurred[steps+2] = work;
	  cache.get (new EntryPyramid (work, octave * firstScale * powf (scaleRatio, steps+2)));
	}
	else  // more repeatable, but slower
	{
	  if (octave <= 1.0f)
	  {
		blurred[0] = work;
		for (int i = 1; i < steps + 3; i++)
		{
		  Gaussian1D & blur = blurs[i-1];
		  blur.direction = Horizontal;
		  work *= blur;
		  blur.direction = Vertical;
		  work *= blur;
		  blurred[i] = work;
		  dogs[i-1] = difference (blurred[i-1], blurred[i]);
		  cache.get (new EntryPyramid (blurred[i], octave * firstScale * powf (scaleRatio, i)));
		}
	  }
	  else
	  {
		dogs[0] = dogs[steps  ] * decimate;
		dogs[1] = dogs[steps+1] * decimate;
		blurred[2] = blurred[steps+2] * decimate;
		work = blurred[2];
		for (int i = 3; i < steps + 3; i++)
		{
		  Gaussian1D & blur = blurs[i-1];
		  blur.direction = Horizontal;
		  work *= blur;
		  blur.direction = Vertical;
		  work *= blur;
		  blurred[i] = work;
		  dogs[i-1] = difference (blurred[i-1], blurred[i]);
		  cache.get (new EntryPyramid (blurred[i], octave * firstScale * powf (scaleRatio, i)));
		}
	  }
	}

	// Search for maxima in DoG at each scale level
	for (int i = 1; i <= steps; i++)
	{
	  ImageOf<float> & dog = dogs[i];
	  for (int y = crop; y < dog.height - crop; y++)
	  {
		for (int x = crop; x < dog.width - crop; x++)
		{
		  float & value = dog(x,y);
		  if (   fabsf (value) > 0.8 * thresholdPeak
			  && isLocalMax (value, dog, x, y)
			  && isLocalMax (value, dogs[i-1], x, y)
			  && isLocalMax (value, dogs[i+1], x, y)
			  && notOnEdge (dog, x, y))
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
			  peak = fitQuadratic (dogs, i, u, v, offset);
			  if (offset[1] > 0.6f  &&  u < dog.width - crop)
			  {
				u++;
			  }
			  if (offset[1] < -0.6f  &&  u > crop)
			  {
				u--;
			  }
			  if (offset[2] > 0.6f  &&  v < dog.height - crop)
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
			  p.scale = octave * firstScale * powf (scaleRatio, i + offset[0]);
			  p.x = (x + offset[1] + 0.5f) * octave - 0.5f;  // Coordinates all use center-of-pixel convention.  Must shift to left edge of pixel to properly overly images w/ different scales.
			  p.y = (y + offset[2] + 0.5f) * octave - 0.5f;
			  p.weight = peak;
			  p.detector = PointInterest::Blob;
			  sorted.insert (p);
			}
		  }
		}
	  }
	}

 	octave *= 2.0f;
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
