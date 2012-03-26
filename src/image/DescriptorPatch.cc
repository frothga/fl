/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorPatch ------------------------------------------------------

DescriptorPatch::DescriptorPatch (int width, float supportRadial)
{
  this->width = width;
  if (supportRadial == 0) this->supportRadial = width;  // causes width to be in natural pixel units at point.scale
  else                    this->supportRadial = supportRadial;
  dimension = width * width;
}

DescriptorPatch::~DescriptorPatch ()
{
}

Vector<float>
DescriptorPatch::value (ImageCache & cache, const PointAffine & point)
{
  // Find or generate gray image at appropriate blur level
  EntryPyramid * entry = (EntryPyramid *) cache.getLE (new EntryPyramid (GrayFloat, point.scale));
  float originalScale = cache.original->scale;
  float octave = originalScale * pow (2.0, EntryPyramid::octave (point.scale, originalScale));
  if (! entry  ||  entry->scale < octave)
  {
	entry = (EntryPyramid *) cache.get (new EntryPyramid (GrayFloat, octave));
  }

  // Adjust point position to scale of selected image
  octave = (float) cache.original->image.width / entry->image.width;
  PointAffine p = point;
  p.x = (p.x + 0.5f) / octave - 0.5f;
  p.y = (p.y + 0.5f) / octave - 0.5f;
  float half = width / 2.0;
  p.scale *= supportRadial / (octave * width);  // apply all three scale adjustments at once

  // Extract patch
  Transform t (p.projection (), true);
  t.setWindow (0, 0, width, width);
  Image patch = entry->image * t;
  Vector<float> result (((PixelBufferPacked *) patch.buffer)->memory, width * width);

  return result;
}

Image
DescriptorPatch::patch (const Vector<float> & value)
{
  Image result (width, width, GrayFloat);
  ((PixelBufferPacked *) result.buffer)->memory = value.data;

  return result;
}

Comparison *
DescriptorPatch::comparison ()
{
  return new NormalizedCorrelation;
}

void
DescriptorPatch::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & width;
  archive & supportRadial;

  dimension = width * width;
}
