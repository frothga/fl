#include "fl/descriptor.h"


using namespace fl;
using namespace std;


// class Comparison -----------------------------------------------------------

Comparison::~Comparison ()
{
}

Vector<float>
Comparison::preprocess (const Vector<float> & value) const
{
  return Vector<float> (value);
}


// class NormalizedCorrelation ------------------------------------------------

NormalizedCorrelation::NormalizedCorrelation (bool subtractMean, float gamma)
{
  this->subtractMean = subtractMean;
  this->gamma        = gamma;
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

  return powf (result, gamma);
}


// class MetricEuclidean ------------------------------------------------------

MetricEuclidean::MetricEuclidean (float scale, float offset)
{
  this->scale = scale;
  this->offset = offset;
}

float
MetricEuclidean::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  //return 1.0f - tanhf ((value1 - value2).frob (2) * scale);
  return 1.0f / coshf ((value1 - value2).frob (2) * scale + offset);
}


// class HistogramIntersection ------------------------------------------------

HistogramIntersection::HistogramIntersection (float gamma)
{
  this->gamma = gamma;
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
  result = powf (result, gamma);
  return result;
}


// class ChiSquared -----------------------------------------------------------

ChiSquared::ChiSquared (float gamma)
{
  this->gamma = gamma;
}

float
ChiSquared::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  float result = 0;
  const int m = value1.rows ();
  for (int i = 0; i < m; i++)
  {
	float sum = value1[i] + value2[i];
	if (sum != 0.0f)
	{
	  float d = value1[i] - value2[i];
	  result += d * d / sum;
	}
  }
  result = (m - result) / m;
  return powf (result, gamma);
}
