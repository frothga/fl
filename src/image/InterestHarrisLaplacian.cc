#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestHarrisLaplacian ----------------------------------------------

InterestHarrisLaplacian::InterestHarrisLaplacian (int maxPoints, float thresholdFactor, float neighborhood, float firstScale, float lastScale, int extraSteps, float stepSize)
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

  // Generate Harris filters
  for (int s = firstStep + extraSteps; s <= lastStep - extraSteps; s += extraSteps)
  {
	float scale = powf (stepSize, s);
	cerr << "harris scale = " << scale << endl;
	FilterHarris h (scale * 0.7125, scale);  // sigmaI seems to be the truer representative of characteristic scale
	filters.push_back (h);
  }
}

inline void
InterestHarrisLaplacian::findScale (const Image & image, PointInterest & p)
{
  float harrisWeight = p.weight;
  p.weight = 0;

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

  // p.weight could either be max Laplacian response or max Harris response
  // but probably Harris response is more meaningful since this is a Harris
  // detector.  :)
  p.weight = harrisWeight;
}

void
InterestHarrisLaplacian::run (const Image & image, std::multiset<PointInterest> & result)
{
  ImageOf<float> work = image * GrayFloat;
  result.clear ();

  for (int i = 0; i < filters.size (); i++)
  {
cerr << "filter " << i;
double startTime = getTimestamp ();

    int offset = filters[i].offset;

	ImageOf<float> filtered = work * filters[i];

	int nmsSize;
	if (neighborhood < 0)
	{
	  nmsSize = (int) ceilf (-neighborhood * filters[i].sigmaI);
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

		  float r0 = fabsf (laplacians[ i      * extraSteps].response (work, p));  // one step down
		  float r1 = fabsf (laplacians[(i + 1) * extraSteps].response (work, p));  // Current scale
		  float r2 = fabsf (laplacians[(i + 2) * extraSteps].response (work, p));  // one step up
		  if (r1 > r0  &&  r1 > r2)
		  {
			p.scale = filters[i].sigmaI;
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
