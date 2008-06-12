/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4, 1.6 and 1.7 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.8  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.7  2005/10/09 05:30:09  Fred
Update revision history and add Sandia copyright notice.

Revision 1.6  2005/06/07 03:56:50  Fred
Change interface to run() to accomodate polymorphism in returned interest
points.

Revision 1.5  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.4  2005/01/22 21:15:36  Fred
MSVC compilability fix:  Be explicit about float constant.

Revision 1.3  2003/12/29 23:32:11  rothgang
Spelling error in comments.

Revision 1.2  2003/09/07 22:11:23  rothgang
Hack to compute threshold for very large scales.

Revision 1.1  2003/07/30 14:09:03  rothgang
Add new interest operator for the trace of Hessian function.  This is
equivalent to the Laplacian function, but can be computed with seperable
filters, so much more efficient.
-------------------------------------------------------------------------------
*/


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
InterestHessian::run (const Image & image, InterestPointSet & result)
{
  ImageOf<float> work = image * GrayFloat;
  multiset<PointInterest> sorted;

  AbsoluteValue abs;
  float lastThreshold = 0.2f;  // some reasonable default, in case no good threshold is found before we need this variable.

  for (int i = 0; i < filters.size (); i++)
  {
cerr << "filter " << i;
Stopwatch timer;

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
		if (pixel > threshold  &&  (sorted.size () < maxPoints  ||  pixel > sorted.begin ()->weight))
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
			p.detector = PointInterest::Blob;
			sorted.insert (p);
			if (sorted.size () > maxPoints)
			{
			  sorted.erase (sorted.begin ());
			}
		  }
		}
	  }
	}
cerr << " " << added << " " << timer << endl;
  }

  result.add (sorted);
}

void
InterestHessian::read (istream & stream)
{
}

void
InterestHessian::write (ostream & stream) const
{
}
