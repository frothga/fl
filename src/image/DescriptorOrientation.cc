/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"


using namespace fl;
using namespace std;


// class DescriptorOrientation ------------------------------------------------

DescriptorOrientation::DescriptorOrientation (float supportRadial, int supportPixel, float kernelSize)
{
  initialize (supportRadial, supportPixel, kernelSize);
}

DescriptorOrientation::DescriptorOrientation (istream & stream)
{
  read (stream);
}

static inline void
killRadius (float limit, Image & image)
{
  float cx = (image.width - 1) / 2.0f;
  float cy = (image.height - 1) / 2.0f;

  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  float dx = x - cx;
	  float dy = y - cy;
	  float d = sqrtf (dx * dx + dy * dy);
	  if (d > limit)
	  {
		image.setGray (x, y, 0.0f);
	  }
	}
  }
}

void
DescriptorOrientation::initialize (float supportRadial, int supportPixel, float kernelSize)
{
  dimension = 1;
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;
  this->kernelSize    = kernelSize;

  double filterScale = supportPixel / kernelSize;
  Gx = GaussianDerivativeFirst (0, filterScale, -1, 0, UseZeros);
  Gy = GaussianDerivativeFirst (1, filterScale, -1, 0, UseZeros);
  killRadius (supportPixel + 0.5, Gx);
  killRadius (supportPixel + 0.5, Gy);
}

Vector<float>
DescriptorOrientation::value (const Image & image, const PointAffine & point)
{
  int patchSize = 2 * supportPixel + 1;
  double scale = supportPixel / supportRadial;
  Point middle (supportPixel, supportPixel);

  Matrix<double> S = ! point.rectification ();
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;

  Transform rectify (S, scale);
  rectify.setWindow (0, 0, patchSize, patchSize);
  Image patch = image * rectify;
  patch *= *Gx.format;

  Vector<float> result (1);
  result[0] = atan2 (Gy.response (patch, middle), Gx.response (patch, middle));
  return result;
}

Image
DescriptorOrientation::patch (const Vector<float> & value)
{
  int patchSize = 2 * supportPixel + 1;
  double filterScale = supportPixel / kernelSize;
  GaussianDerivativeFirst G (0, filterScale, -1, value[0] + PI);
  killRadius (supportPixel + 1, G);
  Transform t (1, 1);
  t.setPeg (G.width / 2, G.height / 2, patchSize, patchSize);
  return G * t;  // TODO: not sure what the convention is for returning patches.  Should it be in [0,1] or can it be any range?
}

void
DescriptorOrientation::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
  stream.read ((char *) &kernelSize,    sizeof (kernelSize));

  initialize (supportRadial, supportPixel, kernelSize);
}

void
DescriptorOrientation::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &kernelSize,    sizeof (kernelSize));
}
