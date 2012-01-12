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


using namespace fl;
using namespace std;


// class DescriptorContrast ---------------------------------------------------

DescriptorContrast::DescriptorContrast (float supportRadial, int supportPixel)
{
  dimension           = 1;
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;
}

Vector<float>
DescriptorContrast::value (const Image & image, const PointAffine & point)
{
  int patchSize = 2 * supportPixel;
  double scale = supportPixel / supportRadial;
  Point center (supportPixel, supportPixel);  // should be supportPixel - 0.5, but will be rounded up anyway, so don't bother.

  Matrix<double> S = ! point.rectification ();
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;

  Image patch;
  if (scale >= point.scale)
  {
	Transform rectify (S, scale);
	rectify.setWindow (0, 0, patchSize, patchSize);
	patch = image * rectify;
  }
  else
  {
	TransformGauss rectify (S, scale);
	rectify.setWindow (0, 0, patchSize, patchSize);
	patch = image * rectify;
  }
  patch *= GrayFloat;
  ImageOf<float> I_x = patch * FiniteDifference (Horizontal);
  ImageOf<float> I_y = patch * FiniteDifference (Vertical);

  float average = 0;
  for (int y = 0; y < patch.height; y++)
  {
	for (int x = 0; x < patch.width; x++)
	{
	  float dx = I_x(x,y);
	  float dy = I_y(x,y);
	  //average += sqrtf (dx * dx + dy * dy);
	  average += dx * dx + dy * dy;
	}
  }
  average /= (patch.width * patch.height);

  Vector<float> result (1);
  result[0] = average;
  return result;
}

/**
   \todo Actually implement this method.
 **/
Image
DescriptorContrast::patch (const Vector<float> & value)
{
  Image result;
  return result;
}

Comparison *
DescriptorContrast::comparison ()
{
  return new MetricEuclidean;
}

void
DescriptorContrast::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & supportRadial;
  archive & supportPixel;
}
