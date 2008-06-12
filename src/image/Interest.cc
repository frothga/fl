/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 thru 1.6 Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/02/25 22:43:40  Fred
Guard against adding an empty point set.  This is a disaster (segfaults) if the
receiving set is also empty.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 05:28:39  Fred
Update revision history and add Sandia copyright notice.

Revision 1.5  2005/08/27 13:20:00  Fred
Compilation fix for MSVC 7.1

Revision 1.4  2005/06/07 03:56:50  Fred
Change interface to run() to accomodate polymorphism in returned interest
points.

Revision 1.3  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.2  2004/08/29 16:01:58  rothgang
Simpler code for adding data of one collection to the other.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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

void
InterestOperator::read (istream & stream)
{
}

void
InterestOperator::write (ostream & stream) const
{
}
