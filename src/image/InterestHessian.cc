#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestHessian ----------------------------------------------

InterestHessian::InterestHessian (int maxPoints, float thresholdFactor, float neighborhood, float firstScale, float lastScale, int extraSteps, float stepSize)
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

  // Generate Hessian filters
  for (int s = firstStep + extraSteps; s <= lastStep - extraSteps; s += extraSteps)
  {
	float scale = powf (stepSize, s);
	cerr << "hessian scale = " << scale << endl;
	FilterHessian h (scale);
	filters.push_back (h);
  }
}

void
InterestHessian::run (const Image & image, std::multiset<PointInterest> & result)
{
  ImageOf<float> work = image * GrayFloat;
  result.clear ();

  AbsoluteValue abs;

  for (int i = 0; i < filters.size (); i++)
  {
cerr << "filter " << i;
double startTime = getTimestamp ();

    int offset = filters[i].offset;

	ImageOf<float> filtered = work * filters[i] * abs;

	int nmsSize;
	if (neighborhood < 0)
	{
	  nmsSize = (int) ceilf (-neighborhood * filters[i].sigma);
	}
	else
	{
	  nmsSize = (int) neighborhood;
	}
	NonMaxSuppress nms (nmsSize);
	filtered *= nms;

	IntensityDeviation std (nms.average, true);
	filtered * std;
	float threshold = nms.average + std.deviation * thresholdFactor;

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
}
