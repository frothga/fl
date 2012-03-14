/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorSchmid -----------------------------------------------------

DescriptorSchmid::DescriptorSchmid (int scaleCount, float scaleStep)
{
  if (scaleStep < 1)
  {
	scaleStep = sqrt (2.0);
  }

  this->scaleStep = scaleStep;

  initialize (scaleCount);
}

void
DescriptorSchmid::initialize (int scaleCount)
{
  for (int i = 0; i < descriptors.size (); i++)
  {
	delete descriptors[i];
  }
  descriptors.clear ();

  for (int s = 0; s < scaleCount; s++)
  {
    float scale = pow (scaleStep, s);
    DescriptorSchmidScale * d = new DescriptorSchmidScale (scale);
    descriptors.push_back (d);
  }

  dimension = descriptors[0]->dimension;
}

DescriptorSchmid::~DescriptorSchmid ()
{
  for (int i = 0; i < descriptors.size (); i++)
  {
	delete descriptors[i];
  }
}

Vector<float>
DescriptorSchmid::value (ImageCache & cache, const PointAffine & point)
{
  return findScale (point.scale)->value (cache, point);
}

Image
DescriptorSchmid::patch (const Vector<float> & value)
{
  // Pick an entry with nice scale and use it to reconstruct the patch
  return findScale (2.0)->patch (value);
}

DescriptorSchmidScale *
DescriptorSchmid::findScale (float sigma)
{
  // Just a linear search.  Could be a binary search, but for a list this small
  // it doesn't make much difference
  DescriptorSchmidScale * result = NULL;
  float smallestDistance = 1e30f;
  for (int i = 0; i < descriptors.size (); i++)
  {
	float distance = fabs (descriptors[i]->sigma - sigma);
	if (distance < smallestDistance)
	{
	  result = descriptors[i];
	  smallestDistance = distance;
	}
  }

  return result;
}

void
DescriptorSchmid::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  int scaleCount = descriptors.size ();
  archive & scaleCount;
  archive & scaleStep;

  if (archive.in) initialize (scaleCount);
}
