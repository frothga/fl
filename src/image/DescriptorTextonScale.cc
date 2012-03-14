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


// class EntryTextonScale -----------------------------------------------------

class EntryTextonScale : public ImageCacheEntry
{
public:
  EntryTextonScale (int i, int width, int height)
  : i (i)
  {
	image.width  = width;
	image.height = height;
  }

  virtual void generate (ImageCache & cache)
  {
	image.format = &GrayFloat;
	image.resize (image.width, image.height);
	image.clear ();
  }

  virtual bool compare (const ImageCacheEntry & that)
  {
	if (typeid (*this).before (typeid (that))) return true;
	const EntryTextonScale * o = dynamic_cast<const EntryTextonScale *> (&that);
	if (! o) return false;
	return i < o->i;
  }

  int i;  ///< Used only to distinguish one entry from another.
};


// class DescriptorTextonScale ------------------------------------------------

DescriptorTextonScale::DescriptorTextonScale (int angles, float firstScale, float lastScale, int extraSteps)
{
  this->angles     = angles;
  this->firstScale = firstScale;
  this->lastScale  = lastScale;
  this->steps      = extraSteps;

  supportRadial = 1.0f;  // On the assumption that interest points exactly specify the desired region.
}

DescriptorTextonScale::~DescriptorTextonScale ()
{
  clear ();
}

void
DescriptorTextonScale::clear ()
{
  for (int i = 0; i < filters.size (); i++) delete filters[i];
  filters.clear ();
  scales.clear ();
}

void
DescriptorTextonScale::initialize ()
{
  clear ();

  const float enlongation = 3.0f;

  scaleRatio = powf (2.0f, 1.0f / steps);
  float scale = firstScale;
  scales.push_back (scale);
  while (scale <= lastScale)
  {
	float nextScale = scale * scaleRatio;
	DifferenceOfGaussians * d = new DifferenceOfGaussians (nextScale, scale);
	(*d) *= Normalize ();
	filters.push_back (d);

	float sigma = d->scale;
	for (int j = 0; j < angles; j++)
	{
	  float angle = (M_PI / angles) * j;
	  GaussianDerivativeSecond * e = new GaussianDerivativeSecond (1, 1, enlongation * sigma, sigma, angle);
	  (*e) *= Normalize ();
	  filters.push_back (e);
	  GaussianDerivativeFirst * o = new GaussianDerivativeFirst (1, enlongation * sigma, sigma, angle);
	  (*o) *= Normalize ();
	  filters.push_back (o);
	}

	scale = nextScale;
	scales.push_back (scale);
  }

  bankSize = 1 + 2 * angles;
  dimension = 2 * bankSize;
}

inline void
DescriptorTextonScale::processPixel (Image & image, ImageOf<float> & scaleImage, vector<ImageOf<float> > & dogs, vector<ImageOf<float> > & responses, int x, int y)
{
  // Determine scale
  int bestScale = 0;
  int bestResponse = dogs[0](x,y);
  for (int i = 1; i < dogs.size (); i++)
  {
	float response = abs (dogs[i](x,y));
	if (response > bestResponse)
	{
	  bestResponse = response;
	  bestScale = i;
	}
  }
  scaleImage(x,y) = bestScale + 1;

  // Compute other filter responses
  responses[0](x,y) = bestResponse;
  unsigned int f = bestScale * bankSize + 1;
  for (int i = 1; i < bankSize; i++)
  {
	responses[i](x,y) = filters[f++]->response (image, Point (x, y));
  }
}

Vector<float>
DescriptorTextonScale::value (ImageCache & cache, const PointAffine & point)
{
  if (filters.size () == 0) initialize ();

  // Collect all working images
  Image image = cache.get (new EntryPyramid (GrayFloat))->image;
  const int width  = image.width;
  const int height = image.height;
  ImageOf<float> scaleImage = cache.get (new EntryTextonScale (-1, width, height))->image;
  vector<ImageOf<float> > dogs (scales.size () - 1);
  for (int i = 0; i < dogs.size (); i++)
  {
	dogs[i] = cache.get (new EntryDOG (scales[i+1], scales[i], width))->image;
  }
  vector<ImageOf<float> > responses (bankSize);
  for (int i = 0; i < bankSize; i++)
  {
	responses[i] = cache.get (new EntryTextonScale (i, width, height))->image;
  }

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

  int sourceL = (int) roundp (max (min (ptl.x, ptr.x, pbl.x, pbr.x), 0.0));
  int sourceR = (int) roundp (min (max (ptl.x, ptr.x, pbl.x, pbr.x), image.width - 1.0));
  int sourceT = (int) roundp (max (min (ptl.y, ptr.y, pbl.y, pbr.y), 0.0));
  int sourceB = (int) roundp (min (max (ptl.y, ptr.y, pbl.y, pbr.y), image.height - 1.0));

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
		if (scaleImage(x,y) == 0) processPixel (image, scaleImage, dogs, responses, x, y);
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
DescriptorTextonScale::value (ImageCache & cache)
{
  if (filters.size () == 0) initialize ();

  // Collect all working images
  Image image = cache.get (new EntryPyramid (GrayFloat))->image;
  const int width  = image.width;
  const int height = image.height;
  ImageOf<float> scaleImage = cache.get (new EntryTextonScale (-1, width, height))->image;
  vector<ImageOf<float> > dogs (scales.size () - 1);
  for (int i = 0; i < dogs.size (); i++)
  {
	dogs[i] = cache.get (new EntryDOG (scales[i+1], scales[i], width))->image;
  }
  vector<ImageOf<float> > responses (bankSize);
  for (int i = 0; i < bankSize; i++)
  {
	responses[i] = cache.get (new EntryTextonScale (i, width, height))->image;
  }

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
		if (scaleImage(x,y) == 0) processPixel (image, scaleImage, dogs, responses, x, y);
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

  result.bitblt (*filters[0] * value[0]);

  int x = 0;
  int y = result.height;
  for (int i = 0; i < angles; i++)
  {
	int index = 1 + 2 * i;
	result.bitblt (*filters[index] * value[index], x, y);
	x += filters[index]->width;
  }

  x = 0;
  y = result.height;
  for (int i = 0; i < angles; i++)
  {
	int index = 2 + 2 * i;
	result.bitblt (*filters[index] * value[index], x, y);
	x += filters[index]->width;
  }

  return result;
}

void
DescriptorTextonScale::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & angles;
  archive & firstScale;
  archive & lastScale;
  archive & steps;
  archive & supportRadial;

  if (archive.in) initialize ();
}
