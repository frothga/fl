/*
Author: Fred Rothganger
Created 02/06/2005


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/imagecache.h"


using namespace fl;
using namespace std;


// class PyramidImage ---------------------------------------------------------

PyramidImage::PyramidImage (const Image & that, float scale)
: Image (that)
{
  this->scale = scale;
}


// class Pyramid --------------------------------------------------------------

Pyramid::~Pyramid ()
{
  clear ();
}

void
Pyramid::clear ()
{
  for (int i = 0; i < images.size (); i++)
  {
	delete images[i];
  }

  images.clear ();
}

void
Pyramid::insert (PyramidImage * image)
{
  int i = findIndex (image->scale);
  if (i < 0)
  {
	images.insert (images.begin (), image);
  }
  else
  {
	if (images[i]->scale == image->scale)
	{
	  delete images[i];
	  images[i] = image;
	}
	else  // i points to entry just before the desired insertion point
	{
	  images.insert (images.begin () + (i + 1), image);
	}
  }
}

PyramidImage *
Pyramid::find (float scale)
{
  if (images.size () == 0)
  {
	return 0;
  }

  int i = findIndex (scale);
  if (i < 0)
  {
	return images[0];
  }
  if (i == images.size () - 1)
  {
	return images[i];
  }
  float r1 = images[i]->scale / scale;
  float r2 = scale / images[i+1]->scale;
  if (r1 > r2)
  {
	return images[i];
  }
  else
  {
	return images[i+1];
  }
}

PyramidImage *
Pyramid::findLE (float scale)
{
  if (images.size () == 0)
  {
	return 0;
  }

  int i = findIndex (scale);
  if (i < 0)
  {
	return 0;
  }
  return images[i];
}

/**
   Binary search for entry with closest scale level <= the requested scale.
   Returns -1 if no entries exist or if the smallest entry is larger than
   the requested scale.
**/
int
Pyramid::findIndex (float scale)
{
  int lo = 0;
  int hi = images.size () - 1;

  if (hi < 0) return -1;

  while (true)
  {
	int mid = (hi + lo) / 2;
	float s = images[mid]->scale;
	if (s == scale)  // mid points to exact record
	{
	  return mid;
	}
	else if (s > scale)  // mid is higher than target
	{
	  if (mid == lo)  // target is lower than lo
	  {
		return lo - 1;
	  }
	  hi = mid;
	}
	else  // mid is lower than target
	{
	  if (mid == hi)  // target is higher than hi
	  {
		return hi;
	  }
	  if (lo < mid)
	  {
		lo = mid;
	  }
	  else  // lo == mid, which implies hi - lo == 1
	  {
		lo = hi;
	  }
	}
  }
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
  if (original) delete original;
  original = 0;

  IDmap::iterator i;
  for (i = cache.begin (); i != cache.end (); i++)
  {
	delete i->second;
  }
  cache.clear ();
}

PyramidImage *
ImageCache::add (const Image & image, int id, float scale)
{
  if (id == primary)
  {
	if (original)
	{
	  if (*original == image) return original;
	  clear ();
	}
	return original = new PyramidImage (image, scale);
  }

  PyramidImage * result = new PyramidImage (image, scale);
  getPyramid (id)->insert (result);
  return result;
}

PyramidImage *
ImageCache::get (int id, float scale)
{
  if (id == primary)
  {
	return original;
  }

  return getPyramid (id)->find (scale);
}

PyramidImage *
ImageCache::getLE (int id, float scale)
{
  if (id == primary)
  {
	return original;
  }

  return getPyramid (id)->findLE (scale);
}

Pyramid *
ImageCache::getPyramid (int id)
{
  IDmap::iterator i = cache.find (id);
  if (i == cache.end ())
  {
	i = cache.insert (make_pair (id, new Pyramid)).first;
  }
  return i->second;
}


// stream operators for all ImageCache related classes -----------------------

ostream &
fl::operator << (ostream & stream, const PyramidImage & data)
{
  stream << data.scale << " " << data.width << " " << data.height;
  return stream;
}

ostream &
fl::operator << (ostream & stream, const Pyramid & data)
{
  for (int i = 0; i < data.images.size (); i++)
  {
	stream << "  " << i << ": " << * data.images[i] << endl;
  }
  return stream;
}

ostream &
fl::operator << (ostream & stream, ImageCache & data)
{
  ImageCache::IDmap::iterator i;
  for (i = data.cache.begin (); i != data.cache.end (); i++)
  {
	stream << "id = " << i->first << endl;
	stream << * i->second << endl;
  }
  return stream;
}
