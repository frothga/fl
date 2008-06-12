/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.7 and 1.8 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.10  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.9  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.8  2005/10/09 05:30:22  Fred
Update revision history and add Sandia copyright notice.

Revision 1.7  2005/06/07 03:56:50  Fred
Change interface to run() to accomodate polymorphism in returned interest
points.

Revision 1.6  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.5  2003/09/07 22:15:31  rothgang
Remove findScale().

Revision 1.4  2003/07/24 19:20:16  rothgang
Make InterestHarrisLaplacian and InterestLaplacian work in very similar manner.
Find local maximum rather than global maximum when determining scale.  Develop
appropriate statistic for determining threshold.  Deprecate findScale().  Need
to look more at the statistics.  May switch back to avg + std rather than just
std from zero.

Revision 1.3  2003/07/18 13:09:28  rothgang
Change method for generating scales.  Add "bookending", where the set of scales
is one quantum larger at each end than the requested range of filters.  Add
code to properly detect whether the point is a local maximum in scale space.

Revision 1.2  2003/07/17 02:07:07  rothgang
Add dynamicly sized nms neighborhood.  Limit points to local maxima in scale
space.  Add one quantum of padding at each end of scale range to accurately
find local scale maxima.

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
#include "fl/time.h"


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

void
InterestLaplacian::run (const Image & image, InterestPointSet & result)
{
  ImageOf<float> work = image * GrayFloat;
  multiset<PointInterest> sorted;

  AbsoluteValue abs;

  for (int i = extraSteps; i < laplacians.size () - extraSteps; i += extraSteps)
  {
cerr << "filter " << i;
Stopwatch timer;

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

	IntensityDeviation std (0, true);
	filtered * std;
	float threshold = max (0.0f, std.deviation * thresholdFactor);

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

		  int l = i - extraSteps;
		  int h = i + extraSteps;
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
cerr << " " << timer << endl;
  }

  result.add (sorted);
}

void
InterestLaplacian::read (istream & stream)
{
}

void
InterestLaplacian::write (ostream & stream) const
{
}
