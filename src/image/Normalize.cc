#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class Normalize ------------------------------------------------------------

Normalize::Normalize (double length)
{
  this->length = length;
}

Image
Normalize::filter (const Image & image)
{
  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (image.width, image.height, GrayFloat);
	result.timestamp = image.timestamp;
	ImageOf<float> that (image);
	float sum = 0;
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		sum += that (x, y) * that (x, y);
	  }
	}
	sum = sqrt (sum);
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		result (x, y) = that (x, y) * length / sum;
	  }
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> result (image.width, image.height, GrayDouble);
	result.timestamp = image.timestamp;
	ImageOf<double> that (image);
	double sum = 0;
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		sum += that (x, y) * that (x, y);
	  }
	}
	sum = sqrt (sum);
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		result (x, y) = that (x, y) * length / sum;
	  }
	}
	return result;
  }
  else
  {
	throw "Normalize::filter: unimplemented format";
  }
}
