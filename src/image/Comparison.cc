#include "fl/descriptor.h"


using namespace fl;


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

Vector<float>
NormalizedCorrelation::preprocess (const Vector<float> & value) const
{
  const int n = value.rows ();

  Vector<float> result (n);

  float mean = value.frob (1) / n;
  float norm = 0;
  for (int r = 0; r < n; r++)
  {
	float t = value[r] - mean;
	result[r] = t;
	norm += t * t;
  }
  result /= sqrtf (norm);

  return result;
}

float
NormalizedCorrelation::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  if (preprocessed)
  {
	return value1.dot (value2);
  }

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

  return v1.dot (v2) / (norm1 * norm2);
}


// class MetricEuclidean ------------------------------------------------------

float
MetricEuclidean::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  return - (value1 - value2).frob (2);
}
