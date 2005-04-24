/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/reduce.h"
#include "fl/factory.h"


using namespace std;
using namespace fl;


// DimensionalityReduction ----------------------------------------------------

Factory<DimensionalityReduction>::productMap Factory<DimensionalityReduction>::products;

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
