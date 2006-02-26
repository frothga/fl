/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


05/2005 Fred Rothganger -- Revised interface.  Created InterestPointSet class
        to manage collections of pointers to InterestPoints.
08/2005 Fred Rothganger -- Fix compilation issue with MSVC 7.1
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


02/2006 Fred Rothganger -- Guard against empty point set.
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

