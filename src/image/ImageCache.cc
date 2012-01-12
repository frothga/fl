/*
Author: Fred Rothganger
Created 02/06/2005


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/imagecache.h"
#include "fl/convolve.h"

#include <limits>


using namespace fl;
using namespace std;


// class ImageCacheEntry ------------------------------------------------------

void
ImageCacheEntry::generate (ImageCache & cache)
{
  // Do nothing, a reasonable action if image has already been filled, say at
  // construction time.
}

bool
ImageCacheEntry::compare (const ImageCacheEntry & that) const
{
  return typeid (*this).before (typeid (that));
}

float
ImageCacheEntry::distance (const ImageCacheEntry & that) const
{
  if (typeid (*this) == typeid (that)) return 0;
  return INFINITY;
}

void
ImageCacheEntry::print (ostream & stream) const
{
  stream << typeid (*this).name ();
}

ostream &
fl::operator << (ostream & stream, const ImageCacheEntry & data)
{
  data.print (stream);
  return stream;
}


// class ImageCache -----------------------------------------------------------

ImageCache ImageCache::shared;  // instantiate the global cache

ImageCache::ImageCache ()
{
  original = 0;
}

ImageCache::~ImageCache ()
{
  clear ();
}

void
ImageCache::clear ()
{
  cacheType::iterator i;
  for (i = cache.begin (); i != cache.end (); i++)
  {
	delete *i;
  }
  cache.clear ();
  original = 0;
}

void
ImageCache::setOriginal (const Image & image, float scale)
{
  if (original)
  {
	if (original->image == image) return;  // ignore scale in this case
	clear ();
  }
  original = new EntryPyramid (image, scale);
  cache.insert (original);
}

ImageCacheEntry *
ImageCache::get (ImageCacheEntry * query)
{
  cacheType::iterator i = cache.find (query);
  if (i != cache.end ())
  {
	delete query;
	return *i;
  }
  query->generate (*this);
  cache.insert (i, query);
  return query;
}

ImageCacheEntry *
ImageCache::getClosest (ImageCacheEntry * query)
{
  ImageCacheEntry * result = 0;
  cacheType::iterator i = cache.lower_bound (query);
  cacheType::iterator j = i--;  // postdecrement, so j comes after i in sequence
  float di = INFINITY;
  float dj = INFINITY;
  if (i != cache.end ()) di = query->distance (**i);
  if (j != cache.end ()) dj = query->distance (**j);
  if      (dj < di)        result = *j;
  else if (di != INFINITY) result = *i;
  delete query;
  return result;
}

ImageCacheEntry *
ImageCache::getLE (ImageCacheEntry * query)
{
  ImageCacheEntry * result = 0;
  cacheType::iterator i = cache.lower_bound (query);
  cacheType::iterator j = i--;
  float di = INFINITY;
  float dj = INFINITY;
  if (i != cache.end ()) di = query->distance (**i);
  if (j != cache.end ()) dj = query->distance (**j);
  if      (dj == 0)        result = *j;  // only use j if it exactly equals the query
  else if (di != INFINITY) result = *i;
  delete query;
  return result;
}

ostream &
fl::operator << (ostream & stream, const ImageCache & data)
{
  ImageCache::cacheType::iterator i;
  for (i = data.cache.begin (); i != data.cache.end (); i++)
  {
	stream << **i << endl;
  }
  return stream;
}


// class EntryPyramid ---------------------------------------------------------

bool EntryPyramid::fast = false;

EntryPyramid::EntryPyramid (const PixelFormat & format, float scale, int width)
: scale (scale)
{
  image.format = &format;
  image.width = width;
}

EntryPyramid::EntryPyramid (const Image & that, float scale)
: scale (scale)
{
  image = that;
}

static inline float
ratioDistance (const float & a, const float & b)
{
  if (a == 0  ||  b == 0) return 0;
  return (a > b ? a / b : b / a) - 1;
}

/**
   This generic function makes minimal assumptions about what resources have
   already been computed, and tries to make optimal use of what it finds.
   However, specific applications can be more efficient if they make more
   assumptions, so consider overriding this in a subclass.
 **/
void
EntryPyramid::generate (ImageCache & cache)
{
  // If image preloaded, then done.
  if (image.height > 0  &&  image.format != 0  &&  image.buffer != 0) return;

  // Find closest entry that comes after this one.  If same scale, then
  // resample and done.
  ImageCache::cacheType::iterator i = cache.cache.lower_bound (this);
  EntryPyramid * o;
  if (i != cache.cache.end ()  &&  (o = dynamic_cast<EntryPyramid *> (*i)))
  {
	if (*o->image.format == *image.format  &&  o->scale == scale)
	{
	  resample (cache, o);
	  return;
	}
  }

  // Search below us for an image to blur/resample.
  float minScale = scale / 2;  // only search within one octave
  EntryPyramid * bestEntry = 0;
  float bestRatio = INFINITY;
  if (i != cache.cache.end ()) i--;
  for (; i != cache.cache.end (); i--)
  {
	o = dynamic_cast<EntryPyramid *> (*i);
	if (! o) break;
	if (*o->image.format != *image.format) break;
	if (o->scale < minScale) break;
	if (bestEntry  &&  o->scale < bestEntry->scale) break;
	float ratio = ratioDistance (o->image.width, image.width);
	if (ratio < bestRatio)
	{
	  bestEntry = o;
	  bestRatio = ratio;
	}
  }
  if (! bestEntry)
  {
	if (! cache.original) throw "ImageCache::original not set";
	if (minScale < cache.original->scale)
	{
	  bestEntry = cache.original;
	}
	else  // automatic pyramid generation via recursive calls
	{
	  int w = (int) floor (log (minScale / cache.original->scale) / log (2));
	  w = cache.original->image.width / pow (2, w);
	  bestEntry = (EntryPyramid *) cache.get (new EntryPyramid (*image.format, minScale, w));
	}
  }

  resample (cache, bestEntry);
}

bool
EntryPyramid::compare (const ImageCacheEntry & that) const
{
  if (typeid (*this).before (typeid (that))) return true;
  const EntryPyramid * o = dynamic_cast<const EntryPyramid *> (&that);
  if (! o) return false;
  if (image.format != 0  &&  o->image.format != 0  &&  image.format->precedence < o->image.format->precedence) return true;
  if (scale              &&  o->scale              &&  scale                    < o->scale                   ) return true;
  if (image.width        &&  o->image.width        &&  image.width              > o->image.width             ) return true;
  return false;
}

float
EntryPyramid::distance (const ImageCacheEntry & that) const
{
  if (typeid (*this) != typeid (that)) return INFINITY;
  EntryPyramid & o = (EntryPyramid &) that;

  float result = 0;
  if (*image.format != *o.image.format) result = 1e4;
  result += ratioDistance (scale, o.scale) * 4 + ratioDistance (image.width, o.image.width);
  return result;
}

void
EntryPyramid::print (ostream & stream) const
{
  stream << "EntryPyramid(" << typeid (*image.format).name () << " " << scale << " " << image.width << ")";
}

/**
   Choose the most efficient filter to convert source into our desired size
   and scale.
 **/
void
EntryPyramid::resample (ImageCache & cache, const EntryPyramid * source)
{
  const int   originalWidth = cache.original->image.width;
  const int   sourceWidth   = source->image.width;
  const float sourceScale   = source->scale;
  const int   targetWidth   = image.width == 0 ? sourceWidth : image.width;
  const float targetScale   = scale       == 0 ? sourceScale : scale;

  bool sameWidth = targetWidth == sourceWidth;
  bool sameScale = targetScale == sourceScale;

  // Early-out if only a format change
  if (sameWidth  &&  sameScale)
  {
	image = source->image * *image.format;
	return;
  }

  const float epsilon = numeric_limits<float>::epsilon ();
  float ratio = (float) sourceWidth / targetWidth;  // downsampling ratio; values < 1 mean upsampling
  float decimal = abs (ratio - roundp (ratio));

  double a = targetScale * targetWidth / originalWidth;  // native scale after
  double b = sourceScale * sourceWidth / originalWidth;  // native scale before

  // BlurDecimate if applicable
  if (fast  &&  ratio > 2 - epsilon  &&  decimal < epsilon)  // integer downsample, so decimate is appropriate
  {
	image = source->image * BlurDecimate ((int) roundp (ratio), b, a) * *image.format;
	return;
  }

  Image work = source->image;

  // Blur if needed
  if (ratio > 1 + epsilon  ||  targetScale > sourceScale)
  {
	a *= ratio;
	double s = sqrt (a * a - b * b);
	Gaussian1D blur (s, Boost);
	blur.direction = Horizontal;
	work *= blur;
	blur.direction = Vertical;
	work *= blur;
  }

  // Resample if needed
  if (abs (ratio - 1) > epsilon)
  {
	if (fast)
	{
	  float ratio1 = (float) source->image.width / (image.width - 1);
	  if (abs (ratio - 0.5) < epsilon  ||  abs (ratio1 - 0.5) < epsilon)
	  {
		int targetHeight = (int) roundp ((float) source->image.height * image.width / source->image.width);
		work *= DoubleSize (image.width % 2, targetHeight % 2);
	  }
	  else work *= Transform (1 / ratio, 1 / ratio);
	}
	else work *= Transform (1 / ratio, 1 / ratio);
  }

  image = work * *image.format;
}
