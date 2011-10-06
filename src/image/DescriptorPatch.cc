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
  this->supportRadial = supportRadial;
  dimension = width * width;
}

DescriptorPatch::~DescriptorPatch ()
{
}

Vector<float>
DescriptorPatch::value (const Image & image, const PointAffine & point)
{
  float half = width / 2.0;

  MatrixFixed<double,2,2> R;
  R(0,0) = cos (point.angle);
  R(1,0) = sin (point.angle);
  R(0,1) = -R(1,0);
  R(1,1) = R(0,0);

  TransformGauss reduce (point.A * R / (half / (supportRadial * point.scale)), true);
  reduce.setPeg (point.x, point.y, width, width);
  ImageOf<float> patch = image * GrayFloat * reduce;

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
