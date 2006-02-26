/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC.
02/2006 Fred Rothganger -- Change Image structure.
*/


#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class ConvolutionDiscrete1D ------------------------------------------------

ConvolutionDiscrete1D::ConvolutionDiscrete1D (const BorderMode mode, const PixelFormat & format, const Direction direction)
: Image (format)
{
  this->direction = direction;
  this->mode = mode;
}

ConvolutionDiscrete1D::ConvolutionDiscrete1D (const Image & image, const BorderMode mode, const Direction direction)
: Image (image)
{
  this->direction = direction;
  this->mode = mode;
}

Image
ConvolutionDiscrete1D::filter (const Image & image)
{
  // This code is essentially the same as Convolution2D::filter ().  However, it
  // removes one layer of looping, which saves a little bit of overhead.

  if (*format != *image.format)
  {
	if (format->precedence <= image.format->precedence)
	{
	  ConvolutionDiscrete1D temp (mode, *image.format, direction);
	  (Image &) temp = (*this) * (*image.format);
	  return image * temp;
	}
	return filter (image * (*format));
  }

  PixelBufferPacked * kernelBuffer = (PixelBufferPacked *) buffer;
  if (! kernelBuffer) throw "kernel must be a packed buffer";
  Pointer kernel = kernelBuffer->memory;
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "Convolution1D only handles packed buffers for now";
  Pointer input = imageBuffer->memory;

  int last = width - 1;
  int mid  = width / 2;
  int stride = direction == Horizontal ? 1 : image.width;

  Image result (*format);

  switch (mode)
  {
    case ZeroFill:
	{
	  result.resize (image.width, image.height);
	  result.clear ();

	  int incx;
	  int incy;
	  if (direction == Horizontal)
	  {
		incx = mid;
		incy = 0;
	  }
	  else  // direction == Vertical
	  {
		incx = 0;
		incy = mid;
	  }

	  if (*format == GrayFloat)
	  {
		float * r = (float *) ((PixelBufferPacked *) result.buffer)->memory;
		r += incy * image.width + incx;
		for (int y = incy; y < result.height - incy; y++)
		{
		  for (int x = incx; x < result.width - incx; x++)
		  {
			float * a = (float *) kernel;
			float * b = (float *) input + (y * image.width + x);
			b += mid * stride;

			float sum = 0;
			for (int i = 0; i <= last; i++)
			{
			  sum += *a++ * *b;
			  b -= stride;
			}
			*r++ = sum;
		  }
		  r += incx + incx;
		}
	  }
	  else if (*format == GrayDouble)
	  {
		double * r = (double *) ((PixelBufferPacked *) result.buffer)->memory;
		r += incy * image.width + incx;
		for (int y = incy; y < result.height - incy; y++)
		{
		  for (int x = incx; x < result.width - incx; x++)
		  {
			double * a = (double *) kernel;
			double * b = (double *) input + (y * image.width + x);
			b += mid * stride;

			double sum = 0;
			for (int i = 0; i <= last; i++)
			{
			  sum += *a++ * *b;
			  b -= stride;
			}
			*r++ = sum;
		  }
		  r += incx + incx;
		}
	  }
	  else
	  {
		throw "ConvolutionDiscrete1D::filter: unimplemented format";
	  }

	  break;
	}
    case UseZeros:
	{
	  result.resize (image.width, image.height);

	  if (*format == GrayFloat)
	  {
		float * r = (float *) ((PixelBufferPacked *) result.buffer)->memory;
		if (direction == Horizontal)
		{
		  for (int y = 0; y < result.height; y++)
		  {
			for (int x = 0; x < result.width; x++)
			{
			  int low  = max (0,    x + mid - (image.width - 1));	
			  int high = min (last, x + mid);

			  float * a = (float *) kernel + low;
			  float * b = (float *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  float sum = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a++ * *b;
				b -= stride;
			  }
			  *r++ = sum;
			}
		  }
		}
		else  // direction == Vertical
		{
		  for (int y = 0; y < result.height; y++)
		  {
			int low  = max (0,    y + mid - (image.height - 1));	
			int high = min (last, y + mid);

			for (int x = 0; x < result.width; x++)
			{
			  float * a = (float *) kernel + low;
			  float * b = (float *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  float sum = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a++ * *b;
				b -= stride;
			  }
			  *r++ = sum;
			}
		  }
		}
	  }
	  else if (*format == GrayDouble)
	  {
		double * r = (double *) ((PixelBufferPacked *) result.buffer)->memory;
		if (direction == Horizontal)
		{
		  for (int y = 0; y < result.height; y++)
		  {
			for (int x = 0; x < result.width; x++)
			{
			  int low  = max (0,    x + mid - (image.width - 1));	
			  int high = min (last, x + mid);

			  double * a = (double *) kernel + low;
			  double * b = (double *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  double sum = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a++ * *b;
				b -= stride;
			  }
			  *r++ = sum;
			}
		  }
		}
		else  // direction == Vertical
		{
		  for (int y = 0; y < result.height; y++)
		  {
			int low  = max (0,    y + mid - (image.height - 1));	
			int high = min (last, y + mid);

			for (int x = 0; x < result.width; x++)
			{
			  double * a = (double *) kernel + low;
			  double * b = (double *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  double sum = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a++ * *b;
				b -= stride;
			  }
			  *r++ = sum;
			}
		  }
		}
	  }
	  else
	  {
		throw "ConvolutionDiscrete1D::filter: unimplemented format";
	  }

	  break;
	}
    case Boost:
	{
	  result.resize (image.width, image.height);

	  if (*format == GrayFloat)
	  {
		float * r = (float *) ((PixelBufferPacked *) result.buffer)->memory;
		if (direction == Horizontal)
		{
		  for (int y = 0; y < result.height; y++)
		  {
			for (int x = 0; x < result.width; x++)
			{
			  int low  = max (0,    x + mid - (image.width - 1));	
			  int high = min (last, x + mid);

			  float * a = (float *) kernel + low;
			  float * b = (float *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  float sum = 0;
			  float weight = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a * *b;
				weight += *a++;
				b -= stride;
			  }
			  *r++ = sum / weight;
			}
		  }
		}
		else  // direction == Vertical
		{
		  for (int y = 0; y < result.height; y++)
		  {
			int low  = max (0,    y + mid - (image.height - 1));	
			int high = min (last, y + mid);

			for (int x = 0; x < result.width; x++)
			{
			  float * a = (float *) kernel + low;
			  float * b = (float *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  float sum = 0;
			  float weight = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a * *b;
				weight += *a++;
				b -= stride;
			  }
			  *r++ = sum / weight;
			}
		  }
		}
	  }
	  else if (*format == GrayDouble)
	  {
		double * r = (double *) ((PixelBufferPacked *) result.buffer)->memory;
		if (direction == Horizontal)
		{
		  for (int y = 0; y < result.height; y++)
		  {
			for (int x = 0; x < result.width; x++)
			{
			  int low  = max (0,    x + mid - (image.width - 1));	
			  int high = min (last, x + mid);

			  double * a = (double *) kernel + low;
			  double * b = (double *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  double sum = 0;
			  double weight = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a * *b;
				weight += *a++;
				b -= stride;
			  }
			  *r++ = sum / weight;
			}
		  }
		}
		else  // direction == Vertical
		{
		  for (int y = 0; y < result.height; y++)
		  {
			int low  = max (0,    y + mid - (image.height - 1));	
			int high = min (last, y + mid);

			for (int x = 0; x < result.width; x++)
			{
			  double * a = (double *) kernel + low;
			  double * b = (double *) input + (y * image.width + x);
			  b += (mid - low) * stride;

			  double sum = 0;
			  double weight = 0;
			  for (int i = low; i <= high; i++)
			  {
				sum += *a * *b;
				weight += *a++;
				b -= stride;
			  }
			  *r++ = sum / weight;
			}
		  }
		}
	  }
	  else
	  {
		throw "ConvolutionDiscrete1D::filter: unimplemented format";
	  }

	  break;
	}
    case Crop:
    default:
	{
	  if (direction == Horizontal)
	  {
		if (image.width < width)
		{
		  break;
		}
		result.resize (image.width - last, image.height);
	  }
	  else  // direction == Vertical
	  {
		if (image.height < width)
		{
		  break;
		}
		result.resize (image.width, image.height - last);
	  }

	  if (*format == GrayFloat)
	  {
		float * r = (float *) ((PixelBufferPacked *) result.buffer)->memory;
		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			float * a = (float *) kernel;
			float * b = (float *) input + (y * image.width + x);
			b += last * stride;

			float sum = 0;
			for (int i = 0; i <= last; i++)
			{
			  sum += *a++ * *b;
			  b -= stride;
			}
			*r++ = sum;
		  }
		}
	  }
	  else if (*format == GrayDouble)
	  {
		double * r = (double *) ((PixelBufferPacked *) result.buffer)->memory;
		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			double * a = (double *) kernel;
			double * b = (double *) input + (y * image.width + x);
			b += last * stride;

			double sum = 0;
			for (int i = 0; i <= last; i++)
			{
			  sum += *a++ * *b;
			  b -= stride;
			}
			*r++ = sum;
		  }
		}
	  }
	  else
	  {
		throw "ConvolutionDiscrete1D::filter: unimplemented format";
	  }

	  break;
	}
  }

  return result;
}

double
ConvolutionDiscrete1D::response (const Image & image, const Point & p) const
{
  if (*format != *image.format)
  {
	if (format->precedence <= image.format->precedence)
	{
	  ConvolutionDiscrete1D temp (mode, *image.format, direction);
	  (Image &) temp = (*this) * (*image.format);
	  return temp.response (image, p);
	}
	return response (image * (*format), p);
  }

  PixelBufferPacked * kernelBuffer = (PixelBufferPacked *) buffer;
  if (! kernelBuffer) throw "kernel must be a packed buffer";
  Pointer kernel = kernelBuffer->memory;
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "Convolution1D only handles packed buffers for now";
  Pointer input = imageBuffer->memory;

  int last = width - 1;
  int mid = width / 2;

  int x = (int) rint (p.x);
  int y = (int) rint (p.y);

  int low;
  int high;
  int stride;
  if (direction == Horizontal)
  {
	low  = max (0,    x + mid - (image.width - 1));	
	high = min (last, x + mid);
	stride = 1;
  }
  else  // direction == Vertical
  {
	low  = max (0,    y + mid - (image.height - 1));
	high = min (last, y + mid);
	stride = image.width;
  }

  if (low > high)
  {
	return 0;
  }

  if (   (mode == Crop  ||  mode == ZeroFill)
      && (low > 0  ||  high < last))
  {
	return 0;
  }

  if (mode == Boost)
  {
	if (*format == GrayFloat)
	{
	  float * a = (float *) kernel + low;
	  float * b = (float *) input + (y * image.width + x);
	  b += (mid - low) * stride;

	  float result = 0;
	  float weight = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a * *b;
		weight += *a++;
		b -= stride;
	  }
	  return result / weight;
	}
	else if (*format == GrayDouble)
	{
	  double * a = (double *) kernel + low;
	  double * b = (double *) input + (y * image.width + x);
	  b += (mid - low) * stride;

	  double result = 0;
	  double weight = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a * *b;
		weight += *a++;
		b -= stride;
	  }
	  return result / weight;
	}
	else
	{
	  throw "ConvolutionDiscrete1D::response: unimplemented format";
	}
  }
  else  // mode in {Crop, ZeroFill, UseZeros}
  {
	if (*format == GrayFloat)
	{
	  float * a = (float *) kernel + low;
	  float * b = (float *) input + (y * image.width + x);
	  b += (mid - low) * stride;

	  float result = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a++ * *b;
		b -= stride;
	  }
	  return result;
	}
	else if (*format == GrayDouble)
	{
	  double * a = (double *) kernel + low;
	  double * b = (double *) input + (y * image.width + x);
	  b += (mid - low) * stride;

	  double result = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a++ * *b;
		b -= stride;
	  }
	  return result;
	}
	else
	{
	  throw "ConvolutionDiscrete1D::response: unimplemented format";
	}
  }
}

void
ConvolutionDiscrete1D::normalFloats ()
{
  if (*format == GrayFloat)
  {
	float * a   = (float *) ((PixelBufferPacked *) buffer)->memory;
	float * end = a + width;
	while (a < end)
	{
	  if (issubnormal (*a))
	  {
		*a = 0;
	  }
	  a++;
	}
  }
  else if (*format == GrayDouble)
  {
	double * a   = (double *) ((PixelBufferPacked *) buffer)->memory;
	double * end = a + width;
	while (a < end)
	{
	  if (issubnormal (*a))
	  {
		*a = 0;
	  }
	  a++;
	}
  }
}
