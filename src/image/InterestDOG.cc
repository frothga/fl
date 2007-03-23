/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4, 1.6 and 1.7 Copyright 2005 Sandia Corporation.
Revisions 1.9 thru 1.15    Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.15  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.14  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.13  2006/01/22 05:24:22  Fred
Update revision history.

Revision 1.12  2006/01/22 05:19:12  Fred
Add a "fast" mode that decimates the working image immediately when it reaches
a blur level of 2X.  To generate higher scale level DoGs that have the same
size of the lower scale level ones, it then upsamples the difference images. 
This is less accurate than continuing to blur full-sized images.  However, it
saves a significant amount of time, especially in the first octave.  The "fast"
mode is about 23% faster.  This mode is optional, and off by default.

Revision 1.11  2006/01/18 03:30:21  Fred
Cache every image that results from some computation.  (Before, some of these
were omitted.)

Compute the first couple of dogs at the next octave by decimation rather that a
fresh difference.  Also, decimate the last image computed in the previous
iteration and resume blurring there.  This saves a couple of convolutions,
which are the most expensive part of the process.  (Working on a possible
optimization which will place the downsampling at true octave bounaries, but it
doesn't work yet.)

Revision 1.10  2006/01/15 15:50:51  Fred
Avoid unecessary image copies.  Retain blurred images between octave iterations
for use in other possible optimizations.

Revision 1.9  2006/01/15 05:46:06  Fred
Improve efficiency of downsampling and of image differencing.

Revision 1.8  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.7  2005/10/09 05:04:51  Fred
Add Sandia copyright notice.  Fix revision history to note change from
lapack?.h to lapack.h

Revision 1.6  2005/09/10 16:47:10  Fred
Add detail to revision history.  Add Sandia copyright notice.  This will need
to be updated with license info before release.

Commit to using new cache mechanism.

Use new polymorphic collection of PointInterest pointers as return type.

Revision 1.5  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.4  2005/01/22 21:15:21  Fred
MSVC compilability fix:  Be explicit about float constant.

Revision 1.3  2004/05/03 20:16:15  rothgang
Rearrange parameters for Gaussians so border mode comes before format.

Revision 1.2  2004/03/22 19:25:47  rothgang
Change wording in time elapsed message.

Revision 1.1  2003/12/30 21:09:07  rothgang
Create difference-of-Gaussian point detector.
-------------------------------------------------------------------------------
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
