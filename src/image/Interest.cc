/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


5/2005 Fred Rothganger -- Revised interface.  Created InterestPointSet class
       to manage collections of pointers to InterestPoints.
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
  int rsize = size ();
  resize (rsize + points.size ());
  PointInterest ** r = & at (rsize);
  multiset<PointInterest>::const_iterator s = points.begin ();
  while (s != points.end ())
  {
	*r++ = new PointInterest (*s++);
  }

  ownPoints = true;  // WARNING: Don't mix owned and non-owned points!
}

