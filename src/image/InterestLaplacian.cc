#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"
//#include "fl/slideshow.h"


using namespace std;
using namespace fl;


// class InterestLaplacian ----------------------------------------------------

InterestLaplacian::InterestLaplacian (int maxPoints, float thresholdFactor, float neighborhood, float firstScale, float lastScale, int extraSteps, float stepSize)
{
  this->maxPoints       = maxPoints;
  this->thresholdFactor = thresholdFactor;
  this->extraSteps      = extraSteps;

  if (neighborhood > 0)
  {
	neighborhood = ceilf (neighborhood);
  }
  else if (neighborhood == 0)
  {
	neighborhood = 1;
  }
  this->neighborhood = neighborhood;

  if (stepSize < 0)
  {
	stepSize = sqrtf (2.0);
  }
  stepSize = powf (stepSize, 1.0f / extraSteps);
  this->stepSize = stepSize;

  firstStep = max (0, (int) rint (logf (firstScale) / logf (stepSize)) - extraSteps);
  int lastStep = (int) ceil ((logf (lastScale) / logf (stepSize) - firstStep) / extraSteps) * extraSteps + firstStep;

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
  int s = (int) rint (logf (p.scale) / logf (stepSize)) - firstStep;
  int l = max (0, s - extraSteps);
  int h = min ((int) laplacians.size () - 1, s + extraSteps);
  for (int i = l + 1; i < h; i++)
  {
	float response = fabsf (laplacians[i].response (image, p));
	if (response > p.weight)
	{
	  p.weight = response;
	  p.scale = laplacians[i].sigma;
	}
  }
}

void
InterestLaplacian::run (const Image & image, std::multiset<PointInterest> & result)
{
  ImageOf<float> work = image * GrayFloat;
  result.clear ();

  AbsoluteValue abs;

  for (int i = extraSteps; i < laplacians.size () - extraSteps; i += extraSteps)
  {
cerr << "filter " << i;
double startTime = getTimestamp ();

    int offset = laplacians[i].width / 2;

	ImageOf<float> filtered = work * laplacians[i] * abs;

	int nmsSize;
	if (neighborhood < 0)
	{
	  nmsSize = (int) ceilf (-neighborhood * laplacians[i].sigma);
	}
	else
	{
	  nmsSize = (int) neighborhood;
	}
	NonMaxSuppress nms (nmsSize);
	filtered *= nms;
	float threshold = nms.average * thresholdFactor;

	for (int y = 0; y < filtered.height; y++)
	{
	  for (int x = 0; x < filtered.width; x++)
	  {
		float pixel = filtered (x, y);
		if (pixel > threshold)
		{
		  PointInterest p;
		  p.x = x + offset;
		  p.y = y + offset;

		  float r0 = fabsf (laplacians[i - extraSteps].response (work, p));
		  float r2 = fabsf (laplacians[i + extraSteps].response (work, p));
		  if (pixel > r0  &&  pixel > r2  &&  r0 > 0  &&  r2 > 0)
		  {
			p.scale = laplacians[i].sigma;
			p.weight = pixel;
			p.detector = PointInterest::Laplacian;
			result.insert (p);
			if (result.size () > maxPoints)
			{
			  result.erase (result.begin ());
			}
		  }
		}
	  }
	}
cerr << " " << getTimestamp () - startTime << endl;
  }

double startTime = getTimestamp ();
cerr << "refine scales ";
  multiset<PointInterest>::iterator it;
  for (it = result.begin (); it != result.end (); it++)
  {
	findScale (work, const_cast<PointInterest &> (*it));
  }
cerr << getTimestamp () - startTime << endl;
}
