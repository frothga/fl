/*
Author: Fred Rothganger
Created 02/06/2005


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/math.h"
#include "fl/imagecache.h"
#include "fl/convolve.h"

#include <float.h>


using namespace fl;
using namespace std;


// class ImageCacheEntry ------------------------------------------------------

ImageCacheEntry::~ImageCacheEntry ()
{
}

void
ImageCacheEntry::generate (ImageCache & cache)
{
  // Do nothing, a reasonable action if image has already been filled, say at
  // construction time.
}

ptrdiff_t
ImageCacheEntry::memory () const
{
  return (ptrdiff_t) ceil (image.width * image.height * image.format->depth);
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
  memory = 0;
}

ImageCache::~ImageCache ()
{
  clear ();
}

void
ImageCache::clear ()
{
  mutex.lock ();
  cacheType::iterator i;
  for (i = cache.begin (); i != cache.end (); i++)
  {
	delete *i;
  }
  cache.clear ();
  original = 0;
  memory = 0;
  mutex.unlock ();
}

void
ImageCache::clear (ImageCacheEntry * query)
{
  mutex.lock ();
  cacheType::iterator i = cache.find (query);
  while (i != cache.end ())
  {
	memory -= (*i)->memory ();
	if (*i == original) original = 0;  // don't keep pointer to something we delete
	if (*i != query) delete *i;  // query itself will be deleted below; this avoids double free
	cache.erase (i);
	i = cache.find (query);
  }
  mutex.unlock ();
  delete query;
}

void
ImageCache::setOriginal (const Image & image, float scale)
{
  mutex.lock ();
  if (original == 0  ||  original->image != image  ||  original->scale != scale)
  {
	setOriginal (new EntryPyramid (image, scale));
  }
  mutex.unlock ();
}

void
ImageCache::setOriginal (EntryPyramid * entry)
{
  mutex.lock ();
  if (cache.find (entry) == cache.end ())
  {
	clear ();
	cache.insert (entry);
	memory = entry->memory ();
  }
  original = entry;
  mutex.unlock ();
}

ImageCacheEntry *
ImageCache::get (ImageCacheEntry * query)
{
  mutex.lock ();
  cacheType::iterator i = cache.find (query);
  if (i != cache.end ())
  {
	mutex.unlock ();
	delete query;
	return *i;
  }
  query->generate (*this);
  cache.insert (i, query);
  memory += query->memory ();
  mutex.unlock ();
  return query;
}

ImageCacheEntry *
ImageCache::getClosest (ImageCacheEntry * query)
{
  ImageCacheEntry * result = 0;
  float di = INFINITY;
  float dj = INFINITY;
  mutex.lock ();
  cacheType::iterator j = cache.lower_bound (query);
  cacheType::iterator i = j;  // i is supposed to come before j, but only if j is a valid entry
  if (j != cache.end ())
  {
	dj = query->distance (**j);
	i--;  // one entry before j
	if (i != cache.end ()) di = query->distance (**i);
  }
  mutex.unlock ();
  if      (dj < di)        result = *j;
  else if (di != INFINITY) result = *i;
  delete query;
  return result;
}

ImageCacheEntry *
ImageCache::getLE (ImageCacheEntry * query)
{
  ImageCacheEntry * result = 0;
  float di = INFINITY;
  float dj = INFINITY;
  mutex.lock ();
  cacheType::iterator i = cache.lower_bound (query);
  cacheType::iterator j = i--;
  if (i != cache.end ()) di = query->distance (**i);
  if (j != cache.end ()) dj = query->distance (**j);
  mutex.unlock ();
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

bool  EntryPyramid::fast                = false;
float EntryPyramid::toleranceScaleRatio = 1e-2;

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

int
EntryPyramid::octave (float scale, float base)
{
  float octave = log2 (scale / base);
  int o = (int) floor (octave);
  if (o + 1 - octave < FLT_EPSILON) o++;  // If it was extremely close to, but just short of, and octave boundary then don't drop to lower octave.
  return o;
}

int
EntryPyramid::targetWidth (float targetScale, int sourceWidth, float sourceScale)
{
  int o = octave (targetScale, sourceScale);
  if (o >= 0) return sourceWidth >>  o;
  else        return sourceWidth << -o;
}

float
EntryPyramid::ratioDistance (const float & a, const float & b)
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
  while (i != cache.cache.begin ())
  {
	i--;
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
	  float originalScale = cache.original->scale;
	  int   originalWidth = cache.original->image.width;
	  float octave     = log2 (scale / originalScale);
	  float nextOctave = floor (octave);  // quantize to the nearest containing octave
	  float ratio      = pow (2, octave);
	  float nextRatio  = pow (2, nextOctave);
	  if (ratioDistance (ratio, nextRatio) <= toleranceScaleRatio) nextRatio /= 2;  // drop an octave, because we are essentially at the start of the current one
	  float nextScale  = nextRatio * originalScale;
	  int   nextWidth  = originalWidth / (int) nextRatio;
	  if (! fast) nextWidth = max (nextWidth, image.width);
	  bestEntry = (EntryPyramid *) cache.get (new EntryPyramid (*image.format, nextScale, nextWidth));
	}
  }

  resample (cache, bestEntry);
}

/**
   This function will also match subclasses of EntryPyramid. This means that
   subclasses will be treated as the same kind of resource. The only value
   in overriding EntryPyramid is to adjust details about how it functions,
   not to create new resource types. A new resource type can, of course, copy
   code from this class, or even contain an instance of it.
 **/
bool
EntryPyramid::compare (const ImageCacheEntry & that) const
{
  const EntryPyramid * o = dynamic_cast<const EntryPyramid *> (&that);
  if (! o) return typeid (*this).before (typeid (that));

  if (image.format != 0  &&  o->image.format != 0)
  {
	if (image.format->precedence < o->image.format->precedence) return true;
	if (*image.format != *o->image.format) return false;
  }

  if (scale  &&  o->scale)
  {
	if (o->scale /    scale - 1 > toleranceScaleRatio) return true;
	if (   scale / o->scale - 1 > toleranceScaleRatio) return false;
  }

  // image.width is in descending order
  if (image.width  &&  o->image.width  &&  image.width > o->image.width) return true;
  return false;
}

float
EntryPyramid::distance (const ImageCacheEntry & that) const
{
  const EntryPyramid * o = dynamic_cast<const EntryPyramid *> (&that);
  if (! o) return INFINITY;

  if (*image.format != *o->image.format) return INFINITY;

  return ratioDistance (scale, o->scale) * 4 + ratioDistance (image.width, o->image.width);
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
  const float sourceScale   = source->scale;
  const int   sourceWidth   = source->image.width;
  const float targetScale   = scale       != 0 ? scale       : sourceScale;
  const int   targetWidth   = image.width != 0 ? image.width : this->targetWidth (targetScale, sourceWidth, sourceScale);

  // Early-out if only a format change
  if (targetWidth == sourceWidth  &&  targetScale == sourceScale)
  {
	image = source->image * *image.format;
	return;
  }

  float ratio = (float) sourceWidth / targetWidth;  // downsampling ratio; values < 1 mean upsampling
  float decimal = abs (ratio - roundp (ratio));

  double a = targetScale * targetWidth / originalWidth;  // native scale after
  double b = sourceScale * sourceWidth / originalWidth;  // native scale before

  // BlurDecimate if applicable
  if (fast  &&  ratio > 2 - FLT_EPSILON  &&  decimal < FLT_EPSILON)  // integer downsample, so decimate is appropriate
  {
	image = source->image * BlurDecimate ((int) roundp (ratio), b, a) * *image.format;
	return;
  }

  Image work = source->image;

  // Blur if needed
  a *= ratio;
  double s = sqrt (a * a - b * b);
  if (s > FLT_EPSILON)
  {
	Gaussian1D blur (s, Boost);
	blur.direction = Horizontal;
	work *= blur;
	blur.direction = Vertical;
	work *= blur;
  }

  // Resample if needed
  if (abs (ratio - 1) > FLT_EPSILON)
  {
	if (fast)
	{
	  float ratio1 = (float) source->image.width / (image.width - 1);
	  if (abs (ratio - 0.5) < FLT_EPSILON  ||  abs (ratio1 - 0.5) < FLT_EPSILON)
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
