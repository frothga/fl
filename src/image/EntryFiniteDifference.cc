/*
Author: Fred Rothganger
Created 03/12/2012

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/imagecache.h"
#include "fl/convolve.h"


using namespace fl;
using namespace std;


// class EntryFiniteDifference ------------------------------------------------

EntryFiniteDifference::EntryFiniteDifference (Direction direction, float scale, int width)
: direction (direction),
  scale (scale)
{
  image.width = width;
}

void
EntryFiniteDifference::generate (ImageCache & cache)
{
  image = cache.get (new EntryPyramid (GrayFloat, scale, image.width))->image * FiniteDifference (direction);
}

bool
EntryFiniteDifference::compare (const ImageCacheEntry & that) const
{
  if (typeid (*this).before (typeid (that))) return true;
  const EntryFiniteDifference * o = dynamic_cast<const EntryFiniteDifference *> (&that);
  if (! o) return false;
  if (direction < o->direction) return true;
  if (scale        &&  o->scale        &&  scale       < o->scale      ) return true;
  if (image.width  &&  o->image.width  &&  image.width > o->image.width) return true;
  return false;
}

float
EntryFiniteDifference::distance (const ImageCacheEntry & that) const
{
  if (typeid (*this) != typeid (that)) return INFINITY;
  const EntryFiniteDifference & o = (const EntryFiniteDifference &) that;

  float result = abs (direction - o.direction) * 1000;
  result += EntryPyramid::ratioDistance (scale, o.scale) * 4 + EntryPyramid::ratioDistance (image.width, o.image.width);
  return result;
}

void
EntryFiniteDifference::print (ostream & stream) const
{
  stream << "EntryFiniteDifference(" << (direction == Horizontal ? "Horizontal" : "Vertical") << " " << scale << " " << image.width << ")";
}
