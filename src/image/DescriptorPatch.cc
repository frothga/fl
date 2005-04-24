/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
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

DescriptorPatch::DescriptorPatch (std::istream & stream)
{
  read (stream);
}

DescriptorPatch::~DescriptorPatch ()
{
}

Vector<float>
DescriptorPatch::value (const Image & image, const PointAffine & point)
{
  float half = width / 2.0;

  Matrix2x2<double> R;
  R(0,0) = cos (point.angle);
  R(1,0) = sin (point.angle);
  R(0,1) = -R(1,0);
  R(1,1) = R(0,0);

  TransformGauss reduce (point.A * R / (half / (supportRadial * point.scale)), true);
  reduce.setPeg (point.x, point.y, width, width);
  ImageOf<float> patch = image * GrayFloat * reduce;

  Vector<float> result (patch.buffer, width * width);

  return result;
}

Image
DescriptorPatch::patch (const Vector<float> & value)
{
  Image result (width, width, GrayFloat);
  result.buffer = value.data;

  return result;
}

Comparison *
DescriptorPatch::comparison ()
{
  return new NormalizedCorrelation;
}

void
DescriptorPatch::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &width, sizeof (width));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  dimension = width * width;
}

void
DescriptorPatch::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  stream.write ((char *) &width, sizeof (width));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
}

