/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


08/2005 Fred Rothganger -- Compilability fix for GCC 3.4.4
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
