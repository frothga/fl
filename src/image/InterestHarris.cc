/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3 and 1.4 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/09 05:29:35  Fred
Update revision history and add Sandia copyright notice.

Revision 1.3  2005/06/07 03:56:50  Fred
Change interface to run() to accomodate polymorphism in returned interest
points.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/interest.h"

#include <math.h>

// Include for debugging
//#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestHarris -------------------------------------------------------

InterestHarris::InterestHarris (int neighborhood, int maxPoints, float thresholdFactor)
: nms (neighborhood),
  filter (1.0, 1.4, GrayFloat)
{
  this->maxPoints       = maxPoints;
  this->thresholdFactor = thresholdFactor;
}

void
InterestHarris::run (const Image & image, InterestPointSet & result)
{
  int offset = filter.offset;

  ImageOf<float> i = image * filter;
  i *= nms;
  float threshold = nms.average * thresholdFactor;

  multiset<PointInterest> sorted;

  for (int y = 0; y < i.height; y++)
  {
	for (int x = 0; x < i.width; x++)
	{
	  float pixel = i(x,y);
	  if (pixel > threshold)
	  {
		PointInterest p;
		p.x = x + offset;
		p.y = y + offset;
		p.weight = pixel;
		p.detector = PointInterest::Corner;
		sorted.insert (p);
		if (sorted.size () > maxPoints)
		{
		  sorted.erase (sorted.begin ());
		}
	  }
	}
  }

  result.add (sorted);
}

void
InterestHarris::read (istream & stream)
{
  stream.read ((char *) &maxPoints,       sizeof (maxPoints));
  stream.read ((char *) &thresholdFactor, sizeof (thresholdFactor));
  int neighborhood = 1;
  stream.read ((char *) &neighborhood,    sizeof (neighborhood));
  nms.half = neighborhood;
}

void
InterestHarris::write (ostream & stream) const
{
  stream.write ((char *) &maxPoints,       sizeof (maxPoints));
  stream.write ((char *) &thresholdFactor, sizeof (thresholdFactor));
  int neighborhood = nms.half;
  stream.write ((char *) &neighborhood,    sizeof (neighborhood));
}
