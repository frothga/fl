#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorSpinSimple -------------------------------------------------

static const float hsqrt2 = sqrt (2.0) / 2.0;  // "half square root of 2".

DescriptorSpinSimple::DescriptorSpinSimple (int binsRadial, int binsIntensity, float supportRadial, float supportIntensity)
{
  this->binsRadial       = binsRadial;
  this->binsIntensity    = binsIntensity;
  this->supportRadial    = supportRadial;
  this->supportIntensity = supportIntensity;
}

void
DescriptorSpinSimple::rangeMinMax (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum)
{
  DescriptorSpin::rangeMinMax (image, point, x1, y1, x2, y2, width - hsqrt2, minIntensity, quantum);
}

void
DescriptorSpinSimple::rangeMeanDeviation (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum)
{
  DescriptorSpin::rangeMeanDeviation (image, point, x1, y1, x2, y2, width - hsqrt2, minIntensity, quantum);
}

void
DescriptorSpinSimple::doBinning (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float minIntensity, float quantum, float binRadius, Vector<float> & result)
{
  width -= hsqrt2;

  ImageOf<float> that (image);
  result.resize (binsRadial * binsIntensity);
  result.clear ();

  for (int x = x1; x <= x2; x++)
  {
	float dx = x - point.x;
	for (int y = y1; y <= y2; y++)
	{
	  float dy = y - point.y;
	  float radius = sqrt (dx * dx + dy * dy);
	  if (radius < width)
	  {
		int d = (int) ((that (x, y) - minIntensity) / quantum);
		d = d >? 0;
		d = d <? binsIntensity - 1;

		int r = (int) (radius / binRadius);

		result[r * binsIntensity + d] += 1.0;
	  }
	}
  }
}

