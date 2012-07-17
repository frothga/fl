/*
Author: Fred Rothganger
Created 03/13/2012

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


// class EntryDOG -------------------------------------------------------------

EntryDOG::EntryDOG (float sigmaPlus, float sigmaMinus, int width)
: sigmaPlus (sigmaPlus),
  sigmaMinus (sigmaMinus)
{
  image.width = width;
  scale = DifferenceOfGaussians::crossover (sigmaPlus, sigmaMinus);
}

void
EntryDOG::generate (ImageCache & cache)
{
  Image imageMinus = cache.get (new EntryPyramid (GrayFloat, sigmaMinus, image.width))->image;
  int w = imageMinus.width;
  int h = imageMinus.height;
  Image imagePlus  = cache.get (new EntryPyramid (GrayFloat, sigmaPlus,  w          ))->image;  // imagePlus *must* match width of imageMinus.

  image.format = &GrayFloat;
  image.resize (w, h);

  float * plus  = (float *) ((PixelBufferPacked *) imagePlus .buffer)->base ();
  float * minus = (float *) ((PixelBufferPacked *) imageMinus.buffer)->base ();
  float * dest  = (float *) ((PixelBufferPacked *) image     .buffer)->base ();
  float * end   = dest + w * h;
  while (dest < end)
  {
	*dest++ = *plus++ - *minus++;
  }
}

bool
EntryDOG::compare (const ImageCacheEntry & that) const
{
  if (typeid (*this).before (typeid (that))) return true;
  const EntryDOG * o = dynamic_cast<const EntryDOG *> (&that);
  if (! o) return false;

  // Scale always matters for DOGs
  if (o->scale /    scale - 1 > EntryPyramid::toleranceScaleRatio) return true;
  if (   scale / o->scale - 1 > EntryPyramid::toleranceScaleRatio) return false;

  if (image.width  &&  o->image.width  &&  image.width > o->image.width) return true;
  return false;
}

float
EntryDOG::distance (const ImageCacheEntry & that) const
{
  if (typeid (*this) != typeid (that)) return INFINITY;
  const EntryDOG & o = (const EntryDOG &) that;

  float result = 0;
  result += EntryPyramid::ratioDistance (scale,       o.scale)       * 1000;
  result += EntryPyramid::ratioDistance (image.width, o.image.width) * 10;
  result += EntryPyramid::ratioDistance (sigmaPlus,   o.sigmaPlus);
  result += EntryPyramid::ratioDistance (sigmaMinus,  o.sigmaMinus);
  return result;
}

void
EntryDOG::print (ostream & stream) const
{
  stream << "EntryDOG(" << sigmaPlus << " " << sigmaMinus << " " << image.width << ")";
}
