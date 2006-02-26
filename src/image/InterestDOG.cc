/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
03/2005 Fred Rothganger -- Change cache mechanism
05/2005 Fred Rothganger -- Changed interface to return a collection of pointers
09/2005 Fred Rothganger -- Changed lapacks.h to lapack.h
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


01/2006 Fred Rothganger -- Improve efficiency of decimation and image
        difference.  Avoid unecessary image copies.  Add "fast" mode.
02/2006 Fred Rothganger -- Change Image structure.
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

  crop = (int) rint (Gaussian2D::cutoff);
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

/**
   Downsample the given image by a factor of 2.
 **/
static inline Image
decimate (const Image & image)
{
  Image result (image.width / 2, image.height / 2, GrayFloat);
  float * fromPixel = (float *) ((PixelBufferPacked *) image .buffer)->memory;
  float * toPixel   = (float *) ((PixelBufferPacked *) result.buffer)->memory;
  float * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float * nextRow = fromPixel + 2 * image.width;
	float * rowEnd  = toPixel   +     result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel++;
	  fromPixel++;
	}
	fromPixel = nextRow;
  }
  return result;
}

/**
   The inverse operation of decimate().  Requires the target size be specified,
   since it could be odd or even.
 **/
static inline Image
upsample (const Image & image, int width, int height)
{
  /*  Slightly more accurate, but much slower
  Transform u (2.0, 2.0);
  float x = width  / 2.0f - 0.5f;
  float y = height / 2.0f - 0.5f;
  u.setWindow (x, y, width, height);
  return image * u;
  */

  Image result (width, height, GrayFloat);
  float * resultBuffer = (float *) ((PixelBufferPacked *) result.buffer)->memory;

  // Double the main body of the image
  float * p00 = (float *) ((PixelBufferPacked *) image.buffer)->memory;
  float * p10 = p00 + 1;
  float * p01 = p00 + image.width;
  float * p11 = p01 + 1;

  float * q00 = (float *) resultBuffer;
  float * q10 = q00 + 1;
  float * q01 = q00 + result.width;
  float * q11 = q01 + 1;
  int lastStep = width + 2 + width % 2;  // to step over odd pixel at end, if it is there

  float * end = p00 + image.width * image.height;
  while (p11 < end)
  {
	float * rowEnd = p00 + image.width;
	while (p10 < rowEnd)
	{
	  *q00 =  *p00;
	  *q10 = (*p00 + *p10) / 2.0f;
	  *q01 = (*p00 + *p01) / 2.0f;
	  *q11 = (*p00 + *p10 + *p01 + *p11) / 4.0f;

	  p00 = p10++;
	  p01 = p11++;

	  q00 += 2;
	  q10 += 2;
	  q01 += 2;
	  q11 += 2;
	}
	*q00 = *q10 =  *p00;
	*q01 = *q11 = (*p00 + *p01) / 2.0f;

	q00 += lastStep;
	q10 = q00 + 1;
	q01 += lastStep;
	q11 = q01 + 1;

	p00 = p10++;
	p01 = p11++;
  }

  // Fill in bottom
  if (height % 2)  // odd: copy two rows of pixels
  {
	float * q02 = q01 + result.width;
	float * q12 = q02 + 1;
	float * rowEnd = p00 + image.width;
	while (p10 < rowEnd)
	{
	  *q00 = *q01 = *q02 =  *p00;
	  *q10 = *q11 = *q12 = (*p00 + *p10) / 2.0f;

	  p00 = p10++;

	  q00 += 2;
	  q10 += 2;
	  q01 += 2;
	  q11 += 2;
	  q02 += 2;
	  q12 += 2;
	}
	*q00 = *q10 = *q01 = *q11 = *q02 = *q12 = *p00;
  }
  else  // even: copy one row of pixels
  {
	float * rowEnd = p00 + image.width;
	while (p10 < rowEnd)
	{
	  *q00 = *q01 =  *p00;
	  *q10 = *q11 = (*p00 + *p10) / 2.0f;

	  p00 = p10++;

	  q00 += 2;
	  q10 += 2;
	  q01 += 2;
	  q11 += 2;
	}
	*q00 = *q10 = *q01 = *q11 = *p00;
  }

  // Fill in right side
  if (width % 2)  // odd: copy an extra column of pixels
  {
	float * q1 = (float *) resultBuffer;
	float * end = q1 + result.width * result.height;
	q1 += (result.width - 1);
	float * q0 = q1 - 1;
	while (q1 < end)
	{
	  *q1 = *q0;
	  q0 += width;
	  q1 += width;
	}
  }

  return result;
}

void
InterestDOG::run (const Image & image, InterestPointSet & result)
{
  Stopwatch timer;

  ImageCache::shared.add (image);
  multiset<PointInterest> sorted;

  ImageOf<float> work;
  if (*image.format == GrayFloat)
  {
	work.copyFrom (image);
	ImageCache::shared.add (image, ImageCache::monochrome);
  }
  else
  {
	work = image * GrayFloat;
	ImageCache::shared.add (work, ImageCache::monochrome);
  }
  if (firstScale != 0.5f)  // The blur level of a raw image is defined to be 0.5
  {
	Gaussian1D blur (sqrt (firstScale * firstScale - 0.25), Boost, GrayFloat, Horizontal);
	work *= blur;
	blur.direction = Vertical;
	work *= blur;
	ImageCache::shared.add (work, ImageCache::monochrome, firstScale);
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
		  ImageCache::shared.add (work, ImageCache::monochrome, octave * firstScale * powf (scaleRatio, i+1));
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
		  ImageCache::shared.add (work, ImageCache::monochrome, octave * firstScale * powf (scaleRatio, i+1));
		}
	  }
	  int w = work.width;
	  int h = work.height;
	  work = decimate (work);
	  for (int i = steps; i < steps + 2; i++)
	  {
		blurred[i] = work;
		Gaussian1D & blur = blurs[i-steps];
		blur.direction = Horizontal;
		work *= blur;
		blur.direction = Vertical;
		work *= blur;
		smalldogs[i-steps] = difference (blurred[i], work);
		dogs[i] = upsample (smalldogs[i-steps], w, h);
		ImageCache::shared.add (blurred[i], ImageCache::monochrome, octave * firstScale * powf (scaleRatio, i));
	  }
	  blurred[steps+2] = work;
	  ImageCache::shared.add (work, ImageCache::monochrome, octave * firstScale * powf (scaleRatio, steps+2));
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
		  ImageCache::shared.add (blurred[i], ImageCache::monochrome, octave * firstScale * powf (scaleRatio, i));
		}
	  }
	  else
	  {
		dogs[0] = decimate (dogs[steps  ]);
		dogs[1] = decimate (dogs[steps+1]);
		blurred[2] = decimate (blurred[steps+2]);
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
		  ImageCache::shared.add (blurred[i], ImageCache::monochrome, octave * firstScale * powf (scaleRatio, i));
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
