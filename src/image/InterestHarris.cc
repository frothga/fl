#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


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
InterestHarris::run (const Image & image, multiset<PointInterest> & result)
{
  int offset = filter.offset;

  ImageOf<float> i = image * filter;
  i *= nms;
  float threshold = nms.average * thresholdFactor;

  result.clear ();

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
		p.detector = PointInterest::Harris;
		result.insert (p);
		if (result.size () > maxPoints)
		{
		  result.erase (result.begin ());
		}
	  }
	}
  }
}
