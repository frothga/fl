/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.8 and 1.9 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.11  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.10  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.9  2005/10/09 05:07:06  Fred
Remove lapack.h, as it is no longer necessary to obtain matrix inversion
operator.

Revision 1.8  2005/10/09 04:46:19  Fred
Rename lapack?.h to lapack.h.  Add Sandia copyright notice.

Revision 1.7  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.6  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.5  2004/05/03 20:16:15  rothgang
Rearrange parameters for Gaussians so border mode comes before format.

Revision 1.4  2004/05/03 19:00:07  rothgang
Add Factory.

Revision 1.3  2004/03/22 20:34:35  rothgang
Changed setGray parameter to use reference rather than pointer.  Allows simpler
code in this file.

Revision 1.2  2004/02/15 18:40:56  rothgang
Change convolution kernel parameters so they still work when clipped.  Ensure
that patch is gray.

Revision 1.1  2003/12/30 21:06:55  rothgang
Create orientation descriptor.
-------------------------------------------------------------------------------
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
DescriptorOrientation::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &kernelSize,    sizeof (kernelSize));
}
