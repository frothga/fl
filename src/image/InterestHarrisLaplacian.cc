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
  int s = (int) rint (logf (p.scale) / logf (stepSize)) - firstStep;
  int l = max (0, s - extraSteps);
  int h = min ((int) laplacians.size () - 1, s + extraSteps);

  vector<float> r (h - l + 1);
  for (int i = l; i <= h; i++)
  {
	r[i - l] = fabsf (laplacians[i].response (image, p));
  }

  float harrisWeight = p.weight;
  p.weight = 0;
  p.scale = 0;
  for (int i = 1; i < r.size () - 1; i++)
  {
	if (r[i] > r[i-1]  &&  r[i] > r[i+1]  &&  r[i] > p.weight)
	{
	  p.weight = r[i];
	  p.scale = laplacians[i + l].sigma;
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

	IntensityDeviation std (0, true);
	filtered * std;
	float threshold = std.deviation * thresholdFactor;

	for (int y = 0; y < filtered.height; y++)
	{
	  for (int x = 0; x < filtered.width; x++)
	  {
		float pixel = filtered (x, y);
		if (pixel > threshold  &&  (result.size () < maxPoints  ||  pixel > result.begin ()->weight))
		{
		  PointInterest p;
		  p.x = x + offset;
		  p.y = y + offset;

		  int l = i * extraSteps;
		  int h = l + 2 * extraSteps;
		  vector<float> r (h - l + 1);
		  for (int j = l; j <= h; j++)
		  {
			r[j - l] = fabsf (laplacians[j].response (work, p));
		  }

		  p.weight = 0;
		  p.scale = 0;
		  for (int j = 1; j < r.size () - 1; j++)
		  {
			if (r[j] > r[j-1]  &&  r[j] > r[j+1]  &&  r[j] > p.weight)
			{
			  p.weight = r[j];
			  p.scale = laplacians[j + l].sigma;
			}
		  }

		  if (p.scale > 0)
		  {
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
}
