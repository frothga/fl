/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
*/


#include "fl/descriptor.h"
#include "fl/factory.h"
#include "fl/math.h"


using namespace fl;
using namespace std;


// class Comparison -----------------------------------------------------------

Factory<Comparison>::productMap Factory<Comparison>::products;

Comparison::~Comparison ()
{
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
  Product<Comparison, NormalizedCorrelation>::add ();
  Product<Comparison, MetricEuclidean>::add ();
  Product<Comparison, HistogramIntersection>::add ();
  Product<Comparison, ChiSquared>::add ();
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
NormalizedCorrelation::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  float result;
  if (preprocessed)
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

  result = max (0.0f, result);  // Default mode is to consider everything below zero as zero probability.

  return result;
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
   squashing function that maps a distance of 0 to a value of 1 and a distance
   of inf to a value of 0.  The other method, usable when the Euclidean
   distance is known to have an upper bound, is to map the distance to the
   output value with a linear transformation.
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
MetricEuclidean::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  if (isinf (upperBound))
  {
	return 1.0f / coshf ((value1 - value2).frob (2));
  }
  else
  {
	return 1.0f - (value1 - value2).frob (2) / upperBound;
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
HistogramIntersection::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
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
  result /= max (count1, count2);
  return result;
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
ChiSquared::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  float result = 0;
  const int m = value1.rows ();
  if (preprocessed)
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
  return 1.0f - result / 2.0f;
}
