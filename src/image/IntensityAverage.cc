#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class IntensityAverage ---------------------------------------------------

IntensityAverage::IntensityAverage (bool ignoreZeros)
{
  this->ignoreZeros = ignoreZeros;
}

Image
IntensityAverage::filter (const Image & image)
{
  average = 0;
  int count = 0;

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
