#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"
//#include "fl/slideshow.h"


using namespace std;
using namespace fl;


// class InterestLaplacian ----------------------------------------------------

InterestLaplacian::InterestLaplacian (int maxPoints, float thresholdFactor, int neighborhood, int firstStep, int lastStep, float stepSize, int extraSteps)
: nms (neighborhood)
{
  this->maxPoints       = maxPoints;
  this->thresholdFactor = thresholdFactor;
  this->extraSteps      = extraSteps;

  if (stepSize < 0)
  {
	stepSize = sqrtf (2.0);
  }

  // Factor in extraSteps for scale searching
  stepSize = powf (stepSize, 1.0f / extraSteps);
  firstStep *= extraSteps;
  lastStep *= extraSteps;

  // Generate Laplacian filters
  for (int s = firstStep; s <= lastStep; s++)
  {
	float scale = powf (stepSize, s);
	Laplacian l (scale);
	l *= scale * scale;
	laplacians.push_back (l);
  }
}

inline void
InterestLaplacian::findScale (const Image & image, PointInterest & p)
{
  p.weight = 0;
  p.scale = 1.0;
  vector<Laplacian>::iterator i;
  for (i = laplacians.begin (); i < laplacians.end (); i++)
  {
	float response = fabsf (i->response (image, p));
	if (response > p.weight)
	{
	  p.weight = response;
	  p.scale = i->sigma;
	}
  }

  /*  Technically, a scale at either end of our range is not a local extreme,
	  because we haven't measured the value beyond it to see if it drops off
	  again.  However, suppressing the scale back to 1 doesn't make any more
	  sense than using the end scale...
  if (p.scale <= laplacians.front ().sigma  ||  p.scale == laplacians.back ().sigma)
  {
	p.scale = 1.0;
	p.weight = 0;
  }
  */
}

void
InterestLaplacian::run (const Image & image, multiset<PointInterest> & result)
{
  ImageOf<float> work = image * GrayFloat;

  ImageOf<float> maxImage (work.width, work.height, GrayFloat);
  ImageOf<float> scales (work.width, work.height, GrayFloat);
  maxImage.clear ();
  scales.clear ();

  AbsoluteValue abs;

double startTime;
  for (int i = 0; i < laplacians.size (); i += extraSteps)
  {
cerr << "filter " << i << endl;
startTime = getTimestamp ();
    int offset = laplacians[i].width / 2;

	ImageOf<float> filtered = work * laplacians[i] * abs;
	filtered *= nms;
cerr << "  took " << getTimestamp () - startTime << endl;
startTime = getTimestamp ();

    for (int y = 0; y < filtered.height; y++)
	{
	  for (int x = 0; x < filtered.width; x++)
	  {
		float & target = maxImage (x + offset, y + offset);
		float & source = filtered (x, y);
		if (source > target)
		{
		  target = source;
		  scales (x + offset, y + offset) = laplacians[i].sigma;
		}
	  }
	}
cerr << "  max " << getTimestamp () - startTime << endl;
  }

startTime = getTimestamp ();
  maxImage *= nms;
cerr << "nms " << getTimestamp () - startTime << endl;
  float threshold = nms.average * thresholdFactor;

  result.clear ();

startTime = getTimestamp ();
  for (int y = 0; y < maxImage.height; y++)
  {
	for (int x = 0; x < maxImage.width; x++)
	{
	  float pixel = maxImage (x, y);
	  if (pixel > threshold)
	  {
		PointInterest p;
		p.x = x;
		p.y = y;
		p.weight = pixel;
		p.scale = scales (x, y);
		p.detector = PointInterest::Laplacian;
		result.insert (p);
		if (result.size () > maxPoints)
		{
		  result.erase (result.begin ());
		}
	  }
	}
  }
cerr << "thresholding " << getTimestamp () - startTime << endl;

startTime = getTimestamp ();
  multiset<PointInterest>::iterator it;
  for (it = result.begin (); it != result.end (); it++)
  {
	findScale (work, const_cast<PointInterest &> (*it));
  }
cerr << "find scale " << getTimestamp () - startTime << endl;
}
