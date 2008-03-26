/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 and 1.6 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 05:06:44  Fred
Remove lapack.h, as it is no longer necessary to obtain matrix inversion
operator.

Revision 1.5  2005/10/09 04:45:29  Fred
Rename lapack?.h to lapack.h.  Add Sandia copyright notice.

Revision 1.4  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.2  2004/05/03 18:57:42  rothgang
Add Factory.

Revision 1.1  2004/02/22 00:14:33  rothgang
Add descriptor the measures the contrast in a region.
-------------------------------------------------------------------------------
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

DescriptorContrast::DescriptorContrast (istream & stream)
{
  dimension = 1;
  read (stream);
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
  ImageOf<float> I_x = patch * FiniteDifferenceX ();
  ImageOf<float> I_y = patch * FiniteDifferenceY ();

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
DescriptorContrast::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
}

void
DescriptorContrast::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
}
