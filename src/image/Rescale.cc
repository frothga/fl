#include "fl/convolve.h"


using namespace fl;
using namespace std;


// class Rescale --------------------------------------------------------------

Rescale::Rescale (double a, double b)
{
  this->a = a;
  this->b = b;
}

Rescale::Rescale (const Image & image, bool useFullRange)
{
  double lo = INFINITY;
  double hi = -INFINITY;

  if (*image.format == GrayFloat)
  {
	float * i = (float *) image.buffer;
	float * end = i + image.width * image.height;
	while (i < end)
	{
	  hi = max ((double) *i,   hi);
	  lo = min ((double) *i++, lo);
	}
  }
  else if (*image.format == GrayDouble)
  {
	double * i = (double *) image.buffer;
	double * end = i + image.width * image.height;
	while (i < end)
	{
	  hi = max (*i,   hi);
	  lo = min (*i++, lo);
	}
  }
  else
  {
	a = 1;
	b = 0;
	return;
  }

  if (! useFullRange  &&  hi <= 1)
  {
	if (lo >= 0)
	{
	  a = 1;
	  b = 0;
	  return;
	}
	else if (lo >= -1)
	{
	  a = 0.5;
	  b = 0.5;
	  return;
	}
  }

  a = 1.0 / (hi - lo);
  b = -lo / (hi - lo);
}

Image
Rescale::filter (const Image & image)
{
  if (*image.format == GrayFloat)
  {
	Image result (image.width, image.height, *image.format);
	float * r = (float *) result.buffer;
	float * i = (float *) image.buffer;
	float * end = i + image.width * image.height;
	while (i < end)
	{
	  *r++ = *i++ * a + b;
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	Image result (image.width, image.height, *image.format);
	double * r = (double *) result.buffer;
	double * i = (double *) image.buffer;
	double * end = i + image.width * image.height;
	while (i < end)
	{
	  *r++ = *i++ * a + b;
	}
	return result;
  }
  else
  {
	return Image (image);
  }
}
