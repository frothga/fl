#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


using namespace std;
using namespace fl;


/*
  Notes:

  At small scales, Hessian finds a large number of points.  On a histogram of
  count versus response value, most of the points appear close to zero response.
  There is actually a hump (fast rise, fast fall) near (but not at) zero.
  After that, it appears to fall off exponentially until it reaches the maximum
  response value, at which point (of course) there are no more points.  :)

  As scale gets larger, the hump gradually dissappears, and the curve gets
  more level.  Also, the number of points goes down.  The fact that we are
  zooming in a piece of texture predicts both of these effects.  If you look
  at just the "hill", the amount of area at each response level becomes more
  equal; whereas if you look at the hill and the plains surrounding it, most
  of the response values will be at the low end.  Also, as you zoom in there
  will be fewer maxima.

  I tried writing code that would histogram the points and cut off the hump,
  on the assumption that they were noise.  This approach does not work any
  better than using standard deviation in some form (either relative to zero
  or to the average).

  The problem is that a threshold found on std goes up with scale, and if the
  multiple is high enough (say 2 or greater) it eventually exceeds the maximum,
  and so allows no points.  A hack to work around this allows *all* points
  once the threshold exceeds the maximum.  This works because at large scales
  there are very few points at any response level.
*/


// class InterestHessian ------------------------------------------------------

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
  float lastThreshold = 0.2f;  // some reasonable default, in case no good threshold is found before we need this variable.

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

	IntensityDeviation std (0, true);
	filtered * std;
	float threshold = max (0.0f, std.deviation * thresholdFactor);

	// Hack for large scales
	if (nms.count < 20)
	{
	  threshold = 0;
	}
	else if (nms.count < 100)  // Approaching flat distribution
	{
	  threshold = lastThreshold * nms.count / 100;  // Gradually ease back to zero.
	}
	else
	{
	  lastThreshold = threshold;
	}
cerr << " " << nms.count << " " << std.deviation << " " << threshold;

int added = 0;
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
added++;
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
cerr << " " << added << " " << getTimestamp () - startTime << endl;
  }
}
