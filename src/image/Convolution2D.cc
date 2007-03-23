/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.6  Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 04:08:49  Fred
Add detail to revision history.

Revision 1.5  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.4  2005/01/12 05:10:12  rothgang
Use math.h to adapt to environments (such as cygwin) that lack a definition of
fpclassify().

Revision 1.3  2004/05/03 20:14:12  rothgang
Rearrange parameters so border mode comes before format.  Add function to zero
out subnormal floats in kernel (improves speed).

Revision 1.2  2003/09/07 22:00:10  rothgang
Rename convolution base classes to allow for other methods of computation
besides discrete kernel.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class ConvolutionDiscrete2D ------------------------------------------------

ConvolutionDiscrete2D::ConvolutionDiscrete2D (const BorderMode mode, const PixelFormat & format)
: Image (format)
{
  this->mode = mode;
}

ConvolutionDiscrete2D::ConvolutionDiscrete2D (const Image & image, const BorderMode mode)
: Image (image)
{
  this->mode = mode;
}

Image
ConvolutionDiscrete2D::filter (const Image & image)
{
  if (*format != *image.format)
  {
	if (format->precedence <= image.format->precedence)
	{
	  ConvolutionDiscrete2D temp (*image.format, mode);
	  (Image &) temp = (*this) * (*image.format);
	  return image * temp;
	}
	return filter (image * (*format));
  }

  PixelBufferPacked * pbp = (PixelBufferPacked *) buffer;
  if (! pbp) throw "Convolution kernel must be packed";

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
		float * buffer = (float *) pbp->memory;

		for (int y = midY; y < result.height - midY; y++)
		{
		  for (int x = midX; x < result.width - midX; x++)
		  {
			float sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += buffer[v * width + h] * that (x - h + midX, y - v + midY);
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
		double * buffer = (double *) pbp->memory;

		for (int y = midY; y < result.height - midY; y++)
		{
		  for (int x = midX; x < result.width - midX; x++)
		  {
			double sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += buffer[v * width + h] * that (x - h + midX, y - v + midY);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else
	  {
		throw "ConvolutionDiscrete2D::filter: unimplemented format";
	  }
	}
    case UseZeros:
	{
	  if (*format == GrayFloat)
	  {
		ImageOf<float> result (image.width, image.height, *format);
		ImageOf<float> that (image);
		float * buffer = (float *) pbp->memory;

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
				sum += buffer[v * width + h] * that (x - h + midX, y - v + midY);
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
		double * buffer = (double *) pbp->memory;

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
				sum += buffer[v * width + h] * that (x - h + midX, y - v + midY);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else
	  {
		throw "ConvolutionDiscrete2D::filter: unimplemented format";
	  }
	}
    case Boost:
	{
	  if (*format == GrayFloat)
	  {
		ImageOf<float> result (image.width, image.height, *format);
		ImageOf<float> that (image);
		float * buffer = (float *) pbp->memory;

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
				float value = buffer[v * width + h];
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
		double * buffer = (double *) pbp->memory;

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
				double value = buffer[v * width + h];
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
		throw "ConvolutionDiscrete2D::filter: unimplemented format";
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
		float * buffer = (float *) pbp->memory;

		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			float sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += buffer[v * width + h] * that (x - h + lastH, y - v + lastV);
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
		double * buffer = (double *) pbp->memory;

		for (int y = 0; y < result.height; y++)
		{
		  for (int x = 0; x < result.width; x++)
		  {
			double sum = 0;
			for (int v = 0; v <= lastV; v++)
			{
			  for (int h = 0; h <= lastH; h++)
			  {
				sum += buffer[v * width + h] * that (x - h + lastH, y - v + lastV);
			  }
			}
			result (x, y) = sum;
		  }
		}

		return result;
	  }
	  else
	  {
		throw "ConvolutionDiscrete2D::filter: unimplemented format";
	  }
	}
  }
}

double
ConvolutionDiscrete2D::response (const Image & image, const Point & p) const
{
  if (*format != *image.format)
  {
	if (format->precedence <= image.format->precedence)
	{
	  ConvolutionDiscrete2D temp (*image.format, mode);
	  (Image &) temp = (*this) * (*image.format);
	  return temp.response (image, p);
	}
	return response (image * (*format), p);
  }

  PixelBufferPacked * pbp = (PixelBufferPacked *) buffer;
  if (! pbp) throw "Convolution kernel must be packed";

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
	  float * buffer = (float *) pbp->memory;
	  float result = 0;
	  float weight = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  float value = buffer[v * width + h];
		  result += value * that (x + midX - h, y + midY - v);
		  weight += value;
		}
	  }
	  return result / weight;
	}
	else if (*format == GrayDouble)
	{
	  ImageOf<double> that (image);
	  double * buffer = (double *) pbp->memory;
	  double result = 0;
	  double weight = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  double value = buffer[v * width + h];
		  result += value * that (x + midX - h, y + midY - v);
		  weight += value;
		}
	  }
	  return result / weight;
	}
	else
	{
	  throw "ConvolutionDiscrete2D::response: unimplemented format";
	}
  }
  else
  {
	if (*format == GrayFloat)
	{
	  ImageOf<float> that (image);
	  float * buffer = (float *) pbp->memory;
	  float result = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  result += buffer[v * width + h] * that (x + midX - h, y + midY - v);
		}
	  }
	  return result;
	}
	else if (*format == GrayDouble)
	{
	  ImageOf<double> that (image);
	  double * buffer = (double *) pbp->memory;
	  double result = 0;
	  for (int v = vl; v <= vh; v++)
	  {
		for (int h = hl; h <= hh; h++)
		{
		  result += buffer[v * width + h] * that (x + midX - h, y + midY - v);
		}
	  }
	  return result;
	}
	else
	{
	  throw "ConvolutionDiscrete2D::response: unimplemented format";
	}
  }
}

void
ConvolutionDiscrete2D::normalFloats ()
{
  PixelBufferPacked * pbp = (PixelBufferPacked *) buffer;
  if (! pbp) throw "Convolution kernel must be packed";

  if (*format == GrayFloat)
  {
	float * a   = (float *) pbp->memory;
	float * end = a + width * height;
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
	double * a   = (double *) pbp->memory;
	double * end = a + width * height;
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
