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

// For debugging only
//#include "fl/slideshow.h"
//#include "fl/time.h"
//#include "fl/random.h"


using namespace fl;
using namespace std;


// class DescriptorTextonScale ------------------------------------------------

DescriptorTextonScale::DescriptorTextonScale (int angles, float firstScale, float lastScale, int extraSteps)
{
  this->angles     = angles;
  this->firstScale = firstScale;
  this->lastScale  = lastScale;
  this->steps      = extraSteps;

  supportRadial = 1.0f;  // On the assumption that interest points exactly specify the desired region.

  initialize ();
}

DescriptorTextonScale::DescriptorTextonScale (istream & stream)
{
  read (stream);
}

void
DescriptorTextonScale::initialize ()
{
  lastBuffer = 0;
  filters.clear ();

  const float dogRatio = 1.33f;
  const float enlongation = 3.0f;

  scaleRatio = powf (2.0f, 1.0f / steps);
  float sigma = firstScale;
  while (sigma <= lastScale)
  {
	DifferenceOfGaussians d (sigma * dogRatio, sigma / dogRatio);
	d *= Normalize ();
	filters.push_back (d);
	for (int j = 0; j < angles; j++)
	{
	  float angle = (M_PI / angles) * j;
	  GaussianDerivativeSecond e (1, 1, enlongation * sigma, sigma, angle);
	  e *= Normalize ();
	  filters.push_back (e);
	  GaussianDerivativeFirst o (1, enlongation * sigma, sigma, angle);
	  o *= Normalize ();
	  filters.push_back (o);
	}

	sigma *= scaleRatio;
  }

  bankSize = 1 + 2 * angles;
  responses.resize (bankSize);
  for (int i = 0; i < bankSize; i++)
  {
	responses[i].format = &GrayFloat;
  }

  dimension = 2 * bankSize;
}

void
DescriptorTextonScale::preprocess (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "DescriptorTextonScale only handles packed buffers for now";

  if (lastBuffer == (void *) imageBuffer->memory  &&  lastTime == image.timestamp)
  {
	return;
  }
  lastBuffer = (void *) imageBuffer->memory;
  lastTime = image.timestamp;

  ImageOf<float> work;
  if (*image.format == GrayFloat)
  {
	work.copyFrom (image);  // duplicates image.buffer
  }
  else
  {
	work = image * GrayFloat;  // should create a new buffer different than image.buffer
  }
  if (firstScale != 0.5f)  // The blur level of a raw image is defined to be 0.5
  {
	Gaussian1D blur (sqrt (firstScale * firstScale - 0.25), Boost, GrayFloat, Horizontal);
	work *= blur;
	blur.direction = Vertical;
	work *= blur;
  }

  ImageOf<float> nextWork;
  ImageOf<unsigned int> scales (image.width, image.height, RGBAChar);  // RGBAChar just provides a pixel of size 4 bytes.  We are really using the pixels as integer indices into the filter bank.
  ImageOf<float> maxima (image.width, image.height, GrayFloat);
  scales.clear ();
  maxima.clear ();

  ImageOf<float> octaveImage = work;

  float scale = firstScale;
  unsigned int l = 0;  // scale level
  while (scale <= lastScale)
  {
	float nextScale = scale * scaleRatio;
	Gaussian1D blur (sqrt (nextScale * nextScale - scale * scale), Boost, GrayFloat, Horizontal);  // Construction of the blur kernels can be pulled into initialize() to make handling multiple images more efficient.
	nextWork = work * blur;
	blur.direction = Vertical;
	nextWork *= blur;

	// Scan thru the current scale level and the next scale level.  Compute
	// the difference image and compare to the maxima image.  For any new
	// maximum, store the current scale level.
	unsigned int * s = (unsigned int *) ((PixelBufferPacked *) scales.buffer)->memory;
	float * w = (float *) ((PixelBufferPacked *) work.buffer)->memory;
	float * n = (float *) ((PixelBufferPacked *) nextWork.buffer)->memory;
	float * m = (float *) ((PixelBufferPacked *) maxima.buffer)->memory;
	float * end = m + image.width * image.height;
	while (m < end)
	{
	  float d = fabsf (*w++ - *n++);
	  if (d > *m)
	  {
		*s = l;
		*m = d;
	  }
	  s++;
	  m++;
	}

	scale = nextScale;
	work = nextWork;
	l++;
  }

  // With scales in hand, process the whole image thru the filter bank
  for (int i = 0; i < bankSize; i++)
  {
	responses[i].resize (image.width, image.height);
  }
  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  unsigned int f = scales(x,y) * bankSize;
	  for (int i = 0; i < bankSize; i++)
	  {
		responses[i](x,y) = filters[f++].response (octaveImage, Point (x, y));
	  }
	}
	cerr << ".";
  }
}

Vector<float>
DescriptorTextonScale::value (const Image & image, const PointAffine & point)
{
  preprocess (image);

  // Project the patch into the image.

  Matrix<double> R = point.rectification ();
  Matrix<double> S = ! R;
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;

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

  int sourceL = (int) roundp (max (min (ptl.x, ptr.x, pbl.x, pbr.x), 0.0f));
  int sourceR = (int) roundp (min (max (ptl.x, ptr.x, pbl.x, pbr.x), image.width - 1.0f));
  int sourceT = (int) roundp (max (min (ptl.y, ptr.y, pbl.y, pbr.y), 0.0f));
  int sourceB = (int) roundp (min (max (ptl.y, ptr.y, pbl.y, pbr.y), image.height - 1.0f));

  // Gather statistics on filter responses

  Matrix<float> result (bankSize, 2);  // first column is averages, second column is deviations
  result.clear ();

  int count = 0;
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  Point q = R * Point (x, y);
	  if (fabsf (q.x) <= 1.0f  &&  fabsf (q.y) <= 1.0f)
	  {
		count++;
		for (int i = 0; i < bankSize; i++)
		{
		  result(i,0) += responses[i](x,y);
		}
	  }
	}
  }
  result.column (0) /= count;

  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  Point q = R * Point (x, y);
	  if (fabsf (q.x) <= 1.0f  &&  fabsf (q.y) <= 1.0f)
	  {
		for (int i = 0; i < bankSize; i++)
		{
		  float d = responses[i](x,y) - result(i,0);
		  result(i,1) += d * d;
		}
	  }
	}
  }
  result.column (1) /= count;
  for (int i = 0; i < bankSize; i++)
  {
	result(i,1) = sqrtf (result(i,1));
  }

  return result;
}

Vector<float>
DescriptorTextonScale::value (const Image & image)
{
  preprocess (image);

  Matrix<float> result (bankSize, 2);
  result.clear ();

  int count = 0;
  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  if (image.getAlpha (x,y))
	  {
		count++;
		for (int i = 0; i < bankSize; i++)
		{
		  result(i,0) += responses[i](x,y);
		}
	  }
	}
  }
  result.column (0) /= count;

  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  if (image.getAlpha (x,y))
	  {
		for (int i = 0; i < bankSize; i++)
		{
		  float d = responses[i](x,y) - result(i,0);
		  result(i,1) += d * d;
		}
	  }
	}
  }
  result.column (1) /= count;
  for (int i = 0; i < bankSize; i++)
  {
	result(i,1) = sqrtf (result(i,1));
  }

  return result;
}

Image
DescriptorTextonScale::patch (const Vector<float> & value)
{
  Image result (GrayFloat);

  result.bitblt (filters[0] * value[0]);

  int x = 0;
  int y = result.height;
  for (int i = 0; i < angles; i++)
  {
	int index = 1 + 2 * i;
	result.bitblt (filters[index] * value[index], x, y);
	x += filters[index].width;
  }

  x = 0;
  y = result.height;
  for (int i = 0; i < angles; i++)
  {
	int index = 2 + 2 * i;
	result.bitblt (filters[index] * value[index], x, y);
	x += filters[index].width;
  }

  return result;
}

void
DescriptorTextonScale::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &angles,        sizeof (angles));
  stream.read ((char *) &firstScale,    sizeof (firstScale));
  stream.read ((char *) &lastScale,     sizeof (lastScale));
  stream.read ((char *) &steps,         sizeof (steps));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));

  initialize ();
}

void
DescriptorTextonScale::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &angles,        sizeof (angles));
  stream.write ((char *) &firstScale,    sizeof (firstScale));
  stream.write ((char *) &lastScale,     sizeof (lastScale));
  stream.write ((char *) &steps,         sizeof (steps));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
}
