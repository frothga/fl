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


using namespace std;
using namespace fl;


// class Descriptor -----------------------------------------------------------

uint32_t Descriptor::serializeVersion = 0;

Descriptor::Descriptor ()
{
  monochrome = true;
  dimension = 0;
  supportRadial = 0;
}

Descriptor::~Descriptor ()
{
}

Vector<float>
Descriptor::value (ImageCache & cache)
{
  throw "alpha selected regions not implemented";
}

Vector<float>
Descriptor::value (const Image & image, const PointAffine & point)
{
  ImageCache::shared.setOriginal (image);
  return value (ImageCache::shared, point);
}

Vector<float>
Descriptor::value (const Image & image)
{
  ImageCache::shared.setOriginal (image);
  return value (ImageCache::shared);
}

Comparison *
Descriptor::comparison ()
{
  return new NormalizedCorrelation;
}

void
Descriptor::serialize (Archive & archive, uint32_t version)
{
}
