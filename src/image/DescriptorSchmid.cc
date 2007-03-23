/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3 and 1.5 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/09 04:10:02  Fred
Add detail to revision history.

Revision 1.4  2005/04/23 19:36:45  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.3  2005/01/22 21:14:12  Fred
MSVC compilability fix:  Be explicit about float constant.

Revision 1.2  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

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

DescriptorSchmid::DescriptorSchmid (std::istream & stream)
{
  read (stream);
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
DescriptorSchmid::value (const Image & image, const PointAffine & point)
{
  return findScale (point.scale)->value (image, point);
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
DescriptorSchmid::read (std::istream & stream)
{
  Descriptor::read (stream);

  int scaleCount;
  stream.read ((char *) &scaleCount, sizeof (scaleCount));
  stream.read ((char *) &scaleStep, sizeof (scaleStep));

  initialize (scaleCount);
}

void
DescriptorSchmid::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  int scaleCount = descriptors.size ();
  stream.write ((char *) &scaleCount, sizeof (scaleCount));
  stream.write ((char *) &scaleStep, sizeof (scaleStep));
}

