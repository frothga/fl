/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.7, 1.9 and 1.10 Copyright 2005 Sandia Corporation.
Revisions 1.12 thru 1.13    Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.13  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.12  2006/02/05 22:23:20  Fred
Break dependency numeric and image libraries:  Created a new class called
Metric that resides in the numeric library and does a job similar to but more
general than Comparison.  Derive Comparison from Metric.  This forces a change
in the semantics of the value() function.  Moved preprocessing flag out of the
function prototype and made it a member of the class.  In the future, it may
prove useful to have preprocessing in Metric itself, in which case relevant
members will move up.

Revision 1.11  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.10  2005/10/09 05:11:55  Fred
Add detail to revision history.  Add Sandia copyright notice.

Compilability fix for GCC 3.4.4: force instantiation of template static member.

Revision 1.9  2005/08/27 13:17:37  Fred
Compilation fix for GCC 3.4.4

Revision 1.8  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.7  2005/01/22 21:06:10  Fred
MSVC compilability fix:  Use fl/math.h

Revision 1.6  2004/09/15 21:55:02  rothgang
Add preprocess() to ChiSquared.  Based on assumptions of preprocess(),
intermediate result will always be in [0,2], so change squashing function to an
affine transformation.

Revision 1.5  2004/09/08 17:17:44  rothgang
Add ability to do linear mapping in MetricEuclidean.

Add call to superclass in NormalizedCorrelation::read().

Revision 1.4  2004/05/03 18:54:10  rothgang
Add Factory.

Revision 1.3  2004/03/22 20:28:44  rothgang
Remove most probability transformations (ie: manipulations of the resulting
value so that they approximate the probability of a true match).

Revision 1.2  2004/02/15 18:07:55  rothgang
Change interface of Comparison to return a value in [0,1].  Original idea was
to give probability of correct match, but this varies with data so it should be
handled at the application level.  Will remove the probability idea but leave
the output in [0,1].  The application can then reshape the value into a
probability.

Added several new Comparisons: HistogramIntersection, ChiSquared.  Added
DescriptorCombo and ComparisonCombo.  Added DescriptorContrast.  Enhanced
Descriptor interface to provide some descriptive information: dimension and
chromaticity.

Revision 1.1  2004/01/13 21:14:24  rothgang
Create Comparison class.  Encapsulates the process of comparing two feature
vectors.
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/factory.h"
#include "fl/math.h"


using namespace fl;
using namespace std;


// class Comparison -----------------------------------------------------------

Comparison::Comparison ()
{
  needPreprocess = true;
}

Vector<float>
Comparison::preprocess (const Vector<float> & value) const
{
  return Vector<float> (value);
}

void
Comparison::read (istream & stream)
{
}

void
Comparison::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}

/**
   Since at present all Comparisons (other than ComparisonCombo) are
   lightweight classes implemented in this single source file, it makes sense
   to just register them all at once.  Another alternative would be to hard
   code a factory just for Comparisons.  However, the Factory template is
   more flexible.
 **/
void
Comparison::addProducts ()
{
  Product<Metric, NormalizedCorrelation>::add ();
  Product<Metric, MetricEuclidean>::add ();
  Product<Metric, HistogramIntersection>::add ();
  Product<Metric, ChiSquared>::add ();
}


// class NormalizedCorrelation ------------------------------------------------

NormalizedCorrelation::NormalizedCorrelation (bool subtractMean)
{
  this->subtractMean = subtractMean;
}

NormalizedCorrelation::NormalizedCorrelation (istream & stream)
{
  read (stream);
}

Vector<float>
NormalizedCorrelation::preprocess (const Vector<float> & value) const
{
  if (subtractMean)
  {
	const int n = value.rows ();
	Vector<float> result (n);
	float norm = 0;
	float mean = value.frob (1) / n;
	for (int r = 0; r < n; r++)
	{
	  float t = value[r] - mean;
	  result[r] = t;
	  norm += t * t;
	}
	result /= sqrtf (norm);
	return result;
  }
  else
  {
	return value / value.frob (2);
  }
}

float
NormalizedCorrelation::value (const Vector<float> & value1, const Vector<float> & value2) const
{
  float result;
  if (! needPreprocess)
  {
	result = value1.dot (value2);
  }
  else if (subtractMean)
  {
	const int n = value1.rows ();

	Vector<float> v1 (n);
	Vector<float> v2 (n);

	float mean1 = value1.frob (1) / n;
	float mean2 = value2.frob (1) / n;

	float norm1 = 0;
	float norm2 = 0;
	for (int r = 0; r < n; r++)
	{
	  float t1 = value1[r] - mean1;
	  float t2 = value2[r] - mean2;
	  v1[r] = t1;
	  v2[r] = t2;
	  norm1 += t1 * t1;
	  norm2 += t2 * t2;
	}
	norm1 = sqrt (norm1);
	norm2 = sqrt (norm2);
	result = v1.dot (v2) / (norm1 * norm2);
  }
  else
  {
	result = value1.dot (value2) / (value1.frob (2) * value2.frob (2));
  }

  return 1.0f - max (0.0f, result);  // Using max() because default mode is to consider negative correlation as zero.
}

void
NormalizedCorrelation::read (istream & stream)
{
  Comparison::read (stream);
  stream.read ((char *) &subtractMean, sizeof (subtractMean));
}

void
NormalizedCorrelation::write (ostream & stream, bool withName)
{
  Comparison::write (stream, withName);
  stream.write ((char *) &subtractMean, sizeof (subtractMean));
}


// class MetricEuclidean ------------------------------------------------------

/**
   Select between alternate squashing methods.  There are two possible methods
   for converting a Euclidean distance [0,inf) to [0,1].  One is a hyperbolic
   squashing function that maps a distance of inf to a value of 1.  The other
   method, usable when the Euclidean distance is known to have an upper bound,
   is to map the distance to the output value with a linear transformation.
 **/
MetricEuclidean::MetricEuclidean (float upperBound)
{
  this->upperBound = upperBound;
}

MetricEuclidean::MetricEuclidean (istream & stream)
{
  read (stream);
}

float
MetricEuclidean::value (const Vector<float> & value1, const Vector<float> & value2) const
{
  if (isinf (upperBound))
  {
	return 1.0f - 1.0f / coshf ((value1 - value2).frob (2));
  }
  else
  {
	return (value1 - value2).frob (2) / upperBound;
  }
}

void
MetricEuclidean::read (istream & stream)
{
  Comparison::read (stream);
  stream.read ((char *) &upperBound, sizeof (upperBound));
}

void
MetricEuclidean::write (ostream & stream, bool withName)
{
  Comparison::write (stream, withName);
  stream.write ((char *) &upperBound, sizeof (upperBound));
}


// class HistogramIntersection ------------------------------------------------

HistogramIntersection::HistogramIntersection (istream & stream)
{
  read (stream);
}

float
HistogramIntersection::value (const Vector<float> & value1, const Vector<float> & value2) const
{
  float result = 0;
  const int m = value1.rows ();
  int count1 = 0;
  int count2 = 0;
  for (int i = 0; i < m; i++)
  {
	float minimum = min (value1[i], value2[i]);
	float maximum = max (value1[i], value2[i]);
	if (maximum != 0.0f)
	{
	  result += minimum / maximum;
	}
	if (value1[i] >= 0)
	{
	  count1++;
	}
	if (value2[i] >= 0)
	{
	  count2++;
	}
  }
  return 1.0f - result / max (count1, count2);
}


// class ChiSquared -----------------------------------------------------------

ChiSquared::ChiSquared (istream & stream)
{
  read (stream);
}

Vector<float>
ChiSquared::preprocess (const Vector<float> & value) const
{
  return value / value.frob (1);  // probability distribution over bins
}

float
ChiSquared::value (const Vector<float> & value1, const Vector<float> & value2) const
{
  float result = 0;
  const int m = value1.rows ();
  if (! needPreprocess)
  {
	for (int i = 0; i < m; i++)
	{
	  float sum = value1[i] + value2[i];
	  if (sum != 0.0f)
	  {
		float d = value1[i] - value2[i];
		result += d * d / sum;
	  }
	}
  }
  else
  {
	float s1 = value1.frob (1);
	float s2 = value2.frob (1);
	for (int i = 0; i < m; i++)
	{
	  float a = value1[i] / s1;
	  float b = value2[i] / s2;
	  float sum = a + b;
	  if (sum != 0.0f)
	  {
		float d = a - b;
		result += d * d / sum;
	  }
	}
  }

  // result is always in [0, 2].
  return result / 2.0f;
}
