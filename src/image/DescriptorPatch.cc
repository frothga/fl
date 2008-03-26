/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.8 thru 1.9 Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.5  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.4  2004/03/22 20:35:08  rothgang
Remove probability transformation from comparison.

Revision 1.3  2004/02/15 18:36:52  rothgang
Add dimension() and comparison().

Revision 1.2  2003/07/13 19:23:05  rothgang
Update to use Vector constructor that directly takes a block of memory.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
DescriptorPatch::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &width, sizeof (width));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  dimension = width * width;
}

void
DescriptorPatch::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &width, sizeof (width));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
}

