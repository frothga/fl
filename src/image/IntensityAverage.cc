#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class IntensityAverage ---------------------------------------------------

IntensityAverage::IntensityAverage (bool ignoreZeros)
{
  this->ignoreZeros = ignoreZeros;

  average = 0;
  count = 0;
  minimum = INFINITY;
  maximum = -INFINITY;
}

Image
IntensityAverage::filter (const Image & image)
{
  average = 0;
  count = 0;
  minimum = INFINITY;
  maximum = -INFINITY;

  #define addup(size) \
  { \
	size * pixel = (size *) image.buffer; \
	size * end   = pixel + image.width * image.height; \
	if (ignoreZeros) \
	{ \
	  while (pixel < end) \
	  { \
		if (*pixel != 0) \
		{ \
		  minimum = min (minimum, (float) *pixel); \
		  maximum = max (maximum, (float) *pixel); \
		  average += *pixel; \
		  count++; \
		} \
		pixel++; \
	  } \
	} \
	else \
	{ \
	  while (pixel < end) \
	  { \
		minimum = min (minimum, (float) *pixel); \
		maximum = max (maximum, (float) *pixel); \
		average += *pixel++; \
	  } \
	  count = image.width * image.height; \
	} \
  }

  if (*image.format == GrayFloat)
  {
	addup (float);
  }
  else if (*image.format == GrayDouble)
  {
	addup (double);
  }
  else if (*image.format == GrayChar)
  {
	addup (unsigned char);
  }
  else
  {
	filter (image * GrayFloat);
  }

  average /= count;

  return image;
}
