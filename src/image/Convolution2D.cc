#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Convolution2D --------------------------------------------------------

Convolution2D::Convolution2D (const PixelFormat & format, const BorderMode mode)
: Image (format)
{
  this->mode = mode;
}

Convolution2D::Convolution2D (const Image & image, const BorderMode mode)
: Image (image)
{
  this->mode = mode;
}

Image
Convolution2D::filter (const Image & image)
{
  if (*format != *image.format)
  {
	if (format->precedence <= image.format->precedence)
	{
	  Convolution2D temp (*image.format, mode);
	  (Image &) temp = (*this) * (*image.format);
	  return image * temp;
	}
	return filter (image * (*format));
  }

  int lastH = width - 1;
  int lastV = height - 1;
  int midX = width / 2;
  int midY = height / 2;

  switch (mode)
  {
    case ZeroFill:
	{
	  if (*format == GrayFloat)
	  {
		ImageOf<float> result (image.width, image.height, *format);
		result.clear ();
		ImageOf<float> that (image);

		for (int y = midY; y < result.height - midY; y++)
		{
		  for (int x = midX; x < result.width - midX; x++)
		  {
			float sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += ((float *) buffer)[v * width + h] * that (x - h + midX, y - v + midY);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else if (*format == GrayDouble)
	  {
		ImageOf<double> result (image.width, image.height, *format);
		result.clear ();
		ImageOf<double> that (image);

		for (int y = midY; y < result.height - midY; y++)
		{
		  for (int x = midX; x < result.width - midX; x++)
		  {
			double sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += ((double *) buffer)[v * width + h] * that (x - h + midX, y - v + midY);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else
	  {
		throw "Convolution2D::filter: unimplemented format";
	  }
	}
    case UseZeros:
	{
	  if (*format == GrayFloat)
	  {
		ImageOf<float> result (image.width, image.height, *format);
		ImageOf<float> that (image);

		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			int hh = min (lastH, x + midX);
			int hl = max (0,     x + midX - (image.width - 1));
			int vh = min (lastV, y + midY);
			int vl = max (0,     y + midY - (image.height - 1));

			float sum = 0;
			for (int v = vl; v <= vh; v++)
			{
			  for (int h = hl; h <= hh; h++)
			  {
				sum += ((float *) buffer)[v * width + h] * that (x - h + midX, y - v + midY);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else if (*format == GrayDouble)
	  {
		ImageOf<double> result (image.width, image.height, *format);
		ImageOf<double> that (image);

		for (int y = midY; y < result.height - midY; y++)
		{
		  for (int x = midX; x < result.width - midX; x++)
		  {
			int hh = min (lastH, x + midX);
			int hl = max (0,     x + midX - (image.width - 1));
			int vh = min (lastV, y + midY);
			int vl = max (0,     y + midY - (image.height - 1));

			double sum = 0;
			for (int v = vl; v <= vh; v++)
			{
			  for (int h = hl; h <= hh; h++)
			  {
				sum += ((double *) buffer)[v * width + h] * that (x - h + midX, y - v + midY);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else
	  {
		throw "Convolution2D::filter: unimplemented format";
	  }
	}
    case Boost:
	{
	  if (*format == GrayFloat)
	  {
		ImageOf<float> result (image.width, image.height, *format);
		ImageOf<float> that (image);

		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			int hh = min (lastH, x + midX);
			int hl = max (0,     x + midX - (image.width - 1));
			int vh = min (lastV, y + midY);
			int vl = max (0,     y + midY - (image.height - 1));

			float sum = 0;
			float weight = 0;
			for (int v = vl; v <= vh; v++)
			{
			  for (int h = hl; h <= hh; h++)
			  {
				float value = ((float *) buffer)[v * width + h];
				sum += value * that (x - h + midX, y - v + midY);
				weight += value;
			  }
			}
			result (x, y) = sum / weight;
		  }
		}

		return result;
	  }
	  else if (*format == GrayDouble)
	  {
		ImageOf<double> result (image.width, image.height, *format);
		ImageOf<double> that (image);

		for (int y = midY; y < result.height - midY; y++)
		{
		  for (int x = midX; x < result.width - midX; x++)
		  {
			int hh = min (lastH, x + midX);
			int hl = max (0,     x + midX - (image.width - 1));
			int vh = min (lastV, y + midY);
			int vl = max (0,     y + midY - (image.height - 1));

			double sum = 0;
			double weight = 0;
			for (int v = vl; v <= vh; v++)
			{
			  for (int h = hl; h <= hh; h++)
			  {
				double value = ((double *) buffer)[v * width + h];
				sum += value * that (x - h + midX, y - v + midY);
				weight += value;
			  }
			}
			result (x, y) = sum / weight;
		  }
		}

		return result;
	  }
	  else
	  {
		throw "Convolution2D::filter: unimplemented format";
	  }
	}
    case Crop:
    default:
	{
	  if (image.width < width  ||  image.height < height)
	  {
		return Image (*format);
	  }

	  if (*format == GrayFloat)
	  {
		ImageOf<float> result (image.width - lastH, image.height - lastV, *format);
		ImageOf<float> that (image);

		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			float sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += ((float *) buffer)[v * width + h] * that (x - h + lastH, y - v + lastV);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else if (*format == GrayDouble)
	  {
		ImageOf<double> result (image.width - lastH, image.height - lastV, *format);
		ImageOf<double> that (image);

		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			double sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += ((double *) buffer)[v * width + h] * that (x - h + lastH, y - v + lastV);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else
	  {
		throw "Convolution2D::filter: unimplemented format";
	  }
	}
  }
}

double
Convolution2D::response (const Image & image, const Point & p) const
{
  if (*format != *image.format)
  {
	if (format->precedence <= image.format->precedence)
	{
	  Convolution2D temp (*image.format, mode);
	  (Image &) temp = (*this) * (*image.format);
	  return temp.response (image, p);
	}
	return response (image * (*format), p);
  }

  int lastH = width - 1;
  int lastV = height - 1;
  int midX = width / 2;
  int midY = height / 2;

  int x = (int) rint (p.x);
  int y = (int) rint (p.y);

  int hh = min (lastH, x + midX);
  int hl = max (0,     x + midX - (image.width - 1));
  int vh = min (lastV, y + midY);
  int vl = max (0,     y + midY - (image.height - 1));

  if (hl > hh  ||  vl > vh)
  {
	return 0;
  }

  if (   (mode == Crop  ||  mode == ZeroFill)
      && (hl > 0  ||  hh < lastH  ||  vl > 0  ||  vh < lastV))
  {
	return 0;  // Should really return nan if mode == Crop, but this is more friendly.
  }

  if (mode == Boost)
  {
	if (*format == GrayFloat)
	{
	  ImageOf<float> that (image);
	  float result = 0;
	  float weight = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  float value = ((float *) buffer)[v * width + h];
		  result += value * that (x + midX - h, y + midY - v);
		  weight += value;
		}
	  }
	  return result / weight;
	}
	else if (*format == GrayDouble)
	{
	  ImageOf<double> that (image);
	  double result = 0;
	  double weight = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  double value = ((double *) buffer)[v * width + h];
		  result += value * that (x + midX - h, y + midY - v);
		  weight += value;
		}
	  }
	  return result / weight;
	}
	else
	{
	  throw "Convolution2D::response: unimplemented format";
	}
  }
  else
  {
	if (*format == GrayFloat)
	{
	  ImageOf<float> that (image);
	  float result = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  result += ((float *) buffer)[v * width + h] * that (x + midX - h, y + midY - v);
		}
	  }
	  return result;
	}
	else if (*format == GrayDouble)
	{
	  ImageOf<double> that (image);
	  double result = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  result += ((double *) buffer)[v * width + h] * that (x + midX - h, y + midY - v);
		}
	  }
	  return result;
	}
	else
	{
	  throw "Convolution2D::response: unimplemented format";
	}
  }
}
