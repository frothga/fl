/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.9 and 1.10  Copyright 2005 Sandia Corporation.
Revisions 1.12 and 1.13 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.13  2007/03/25 14:51:05  Fred
Fix template instantiation for Factory.

Revision 1.12  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.11  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.10  2005/10/09 05:12:33  Fred
Compilability fix for GCC 3.4.4: force instantiation of template static member.
Add Sandia copyright notice.

Revision 1.9  2005/08/27 13:17:52  Fred
Compilation fix for GCC 3.4.4

Revision 1.8  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.7  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.6  2004/05/03 18:54:56  rothgang
Add Factory.  Remove conversions to PointAffine.

Revision 1.5  2004/02/15 18:07:55  rothgang
Change interface of Comparison to return a value in [0,1].  Original idea was
to give probability of correct match, but this varies with data so it should be
handled at the application level.  Will remove the probability idea but leave
the output in [0,1].  The application can then reshape the value into a
probability.

Added several new Comparisons: HistogramIntersection, ChiSquared.  Added
DescriptorCombo and ComparisonCombo.  Added DescriptorContrast.  Enhanced
Descriptor interface to provide some descriptive information: dimension and
chromaticity.

Revision 1.4  2004/01/14 18:11:57  rothgang
Add read(), write(), and monochrome attribute.

Revision 1.3  2004/01/13 21:15:51  rothgang
Get rid of factory method.  The plan is to replace it by a template class that
creates a factory.  Add comparison() method.

Revision 1.2  2004/01/08 21:26:28  rothgang
Add support for regions selected by alpha channel.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/serialize.h"


using namespace std;
using namespace fl;


// class Descriptor -----------------------------------------------------------

template class Factory<Descriptor>;

Descriptor::Descriptor ()
{
  monochrome = true;
  dimension = 0;
  supportRadial = 0;
}

Descriptor::~Descriptor ()
{
}

Vector<float>
Descriptor::value (const Image & image)
{
  throw "alpha selected regions not implemented";
}

Comparison *
Descriptor::comparison ()
{
  return new NormalizedCorrelation;
}

void
Descriptor::read (istream & stream)
{
}

void
Descriptor::write (ostream & stream) const
{
}
