/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/interest.h"


using namespace std;
using namespace fl;


// class InterestPointSet -----------------------------------------------------

InterestPointSet::InterestPointSet ()
{
  ownPoints = true;
}

InterestPointSet::~InterestPointSet ()
{
  if (ownPoints)
  {
	iterator i = begin ();
	while (i < end ())
	{
	  delete *i++;
	}
  }
}

void
InterestPointSet::add (const multiset<PointInterest> & points)
{
  int pointSize = points.size ();
  if (! pointSize) return;

  int rsize = size ();
  resize (rsize + pointSize);
  PointInterest ** r = & at (rsize);
  multiset<PointInterest>::const_iterator s = points.begin ();
  while (s != points.end ())
  {
	*r++ = new PointInterest (*s++);
  }

  ownPoints = true;  // WARNING: Don't mix owned and non-owned points!
}


// class InterestOperator -----------------------------------------------------

InterestOperator::~InterestOperator ()
{
}

void
InterestOperator::read (istream & stream)
{
}

void
InterestOperator::write (ostream & stream) const
{
}
