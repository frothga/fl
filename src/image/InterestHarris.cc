/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


5/2005 Fred Rothganger -- Changed interface to return a collection of pointers.
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
