/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.5 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 10:57:30  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/13 04:14:26  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/13 03:31:28  Fred
Add Sandia distribution terms.

Revision 1.4  2005/09/12 03:23:15  Fred
Fix Factory template instantiation for GCC 3.4.4.

Add Sandia copyright notice.  Must add license information before release.

Revision 1.3  2005/04/23 19:40:06  Fred
Add UIUC copyright notice.

Revision 1.2  2004/04/19 17:10:35  rothgang
Provide default implementations for analyze().  Instantiate the productMap for
DimensionalityReduction.

Revision 1.1  2004/04/06 21:07:04  rothgang
Add class of algorithms for reducing dimensionality.  First instance is PCA.
-------------------------------------------------------------------------------
*/


#include "fl/reduce.h"
#include "fl/factory.h"


using namespace std;
using namespace fl;


// DimensionalityReduction ----------------------------------------------------

template class Factory<DimensionalityReduction>;
template <> Factory<DimensionalityReduction>::productMap Factory<DimensionalityReduction>::products;

void
DimensionalityReduction::analyze (const vector< Vector<float> > & data)
{
  vector<int> classAssignments (data.size (), 0);
  analyze (data, classAssignments);
}

void
DimensionalityReduction::analyze (const vector< Vector<float> > & data, const vector<int> & classAssignments)
{
  analyze (data);
}

void
DimensionalityReduction::read (istream & stream)
{
}

void
DimensionalityReduction::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
