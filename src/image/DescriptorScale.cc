/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/math.h"
#include "fl/descriptor.h"
#include "fl/canvas.h"


using namespace fl;
using namespace std;


// class DescriptorScale ------------------------------------------------------

DescriptorScale::DescriptorScale (float firstScale, float lastScale, int interQuanta, float quantum)
{
  this->firstScale = max (1.0f, firstScale);
  this->lastScale = max (this->firstScale, lastScale);
  stepSize = powf (quantum, 1.0f / interQuanta);
  dimension = 1;
}

DescriptorScale::~DescriptorScale ()
{
  for (int i = 0; i < laplacians.size (); i++) delete laplacians[i];
}

void
DescriptorScale::initialize ()
{
  int s = 0;
  while (true)
  {
	float scale = firstScale * powf (stepSize, s++);
	if (scale > lastScale) break;
	Laplacian * l = new Laplacian (scale);
	(*l) *= scale * scale;
	laplacians.push_back (l);
  }
}

Vector<float>
DescriptorScale::value (ImageCache & cache, const PointAffine & point)
{
  if (laplacians.size () == 0) initialize ();
  Image image = cache.get (new EntryPyramid (GrayFloat))->image;

  Vector<float> result (1);
  result[0] = 1;

  float bestResponse = 0;
  vector<Laplacian *>::iterator l;
  for (l = laplacians.begin (); l < laplacians.end (); l++)
  {
	float response = fabsf ((*l)->response (image, point));
	if (response > bestResponse)
	{
	  bestResponse = response;
	  result[0] = (*l)->sigma * M_SQRT2;
	}
  }

  return result;
}

Image
DescriptorScale::patch (const Vector<float> & value)
{
  return Laplacian (value[0] / M_SQRT2);
}

void
DescriptorScale::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & firstScale;
  archive & lastScale;
  archive & stepSize;
}
