/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.6   Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.11 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.11  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.10  2006/04/15 18:56:59  Fred
Rewrite filter() to compute the well-defined portion of the result separately
from the borders.  This allows fewer tests in the inner loop, making the common
case more efficient.

Revision 1.9  2006/03/20 05:32:55  Fred
Image now has null PixelBuffer if it is empty, so trap this case.

Revision 1.8  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 04:08:20  Fred
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

/**
   \todo This code has latent bugs if the kernel is wider than the image.
 **/
template<class T>
static inline void convolveH (T * kernel, T * image, T * result,
							  BorderMode mode,
							  int kernelWidth, int width, int height,
							  int fromStride, int toStride)
{
  assert (kernelWidth <= width);

  // Main convolution
  int mid       = kernelWidth / 2;
  int last      = kernelWidth - 1;
  int rowWidth  = width - last;
  int fromStep  = fromStride - rowWidth;
  int toStep    = toStride   - rowWidth;
  T * kernelEnd = kernel + last;
  T * fromPixel = image;
  T * toPixel   = result;
  if (mode != Crop) toPixel += mid;
  T * end = toPixel + height * toStride;
  while (toPixel < end)
  {
	T * rowEnd = toPixel + rowWidth;
	while (toPixel < rowEnd)
	{
	  register T sum = 0;
	  T * k = kernelEnd;
	  while (k >= kernel)
	  {
		sum += *k-- * *fromPixel++;
	  }
	  *toPixel++ = sum;
	  fromPixel -= last;  // net advance of 1 pixel
	}
	toPixel += toStep;
	fromPixel += fromStep;
  }

  // Edge cases
  switch (mode)
  {
	case ZeroFill:
	{
	  toStep    = toStride - mid;
	  T * left  = result;
	  T * right = result + mid + rowWidth;
	  T * end   = result + height * toStride;
	  while (right < end)
	  {
		T * rowEnd = right + mid;
		while (right < rowEnd)
		{
		  *left++  = (T) 0.0;
		  *right++ = (T) 0.0;
		}
		left += toStep;
		right += toStep;
	  }
	  break;
	}
	case UseZeros:
	{
	  int toLeftStep  = toStride - mid;
	  int toRightStep = toStride + mid;
	  T * kernelStart = kernel + mid;
	  T * fromLeft  = image;
	  T * fromRight = image + mid + rowWidth + mid - 1;
	  T * toLeft  = result;
	  T * toRight = result + mid + rowWidth + mid - 1;
	  T * end     = result + height * toStride;
	  while (toRight < end)
	  {
		T * kernelLeft  = kernelStart;
		T * kernelRight = kernelStart;
		while (kernelRight > kernel)
		{
		  T * kl = kernelLeft++;
		  T * kr = kernelRight--;
		  T * fl = fromLeft;
		  T * fr = fromRight;
		  register T sumLeft = 0;
		  register T sumRight = 0;
		  while (kl >= kernel)
		  {
			sumLeft  += *kl-- * *fl++;
			sumRight += *kr++ * *fr--;
		  }
		  *toLeft++  = sumLeft;
		  *toRight-- = sumRight;
		}
		fromLeft  += fromStride;
		fromRight += fromStride;
		toLeft  += toLeftStep;
		toRight += toRightStep;
	  }
	  break;
	}
	case Boost:
	{
	  // Pre-compute partial sums of the kernel.
	  // Need separate left and right totals in case kernel is not symmetric.
	  T * leftTotal  = (T *) alloca (kernelWidth * sizeof (T));
	  T * rightTotal = (T *) alloca (kernelWidth * sizeof (T));

	  double total = 0;
	  T * k = kernelEnd;
	  T * t = rightTotal + last;
	  while (k >= kernel)
	  {
		*t-- = (T) (total += *k--);
	  }

	  total = 0;
	  k = kernel;
	  t = leftTotal;
	  while (k <= kernelEnd)
	  {
		*t++ = (T) (total += *k++);
	  }

	  // Convolve
	  int toLeftStep  = toStride - mid;
	  int toRightStep = toStride + mid;
	  T * kernelStart = kernel + mid;
	  T * leftTotalStart = leftTotal + mid;
	  T * rightTotalStart = rightTotal + mid;
	  T * fromLeft  = image;
	  T * fromRight = image + mid + rowWidth + mid - 1;
	  T * toLeft  = result;
	  T * toRight = result + mid + rowWidth + mid - 1;
	  T * end   = result + height * toStride;
	  while (toRight < end)
	  {
		T * kernelLeft  = kernelStart;
		T * kernelRight = kernelStart;
		T * lt          = leftTotalStart;
		T * rt          = rightTotalStart;
		while (kernelRight > kernel)
		{
		  T * kl = kernelLeft++;
		  T * kr = kernelRight--;
		  T * fl = fromLeft;
		  T * fr = fromRight;
		  register T sumLeft = 0;
		  register T sumRight = 0;
		  while (kl >= kernel)
		  {
			sumLeft  += *kl-- * *fl++;
			sumRight += *kr++ * *fr--;
		  }
		  *toLeft++  = sumLeft  / *lt++;
		  *toRight-- = sumRight / *rt--;
		}
		fromLeft  += fromStride;
		fromRight += fromStride;
		toLeft  += toLeftStep;
		toRight += toRightStep;
	  }
	  break;
	}
	case Copy:
	{
	  fromStep      = fromStride - mid;
	  toStep        = toStride   - mid;
	  T * fromLeft  = image;
	  T * fromRight = image + (mid + rowWidth);
	  T * toLeft    = result;
	  T * toRight   = result + (mid + rowWidth);
	  T * end       = result + height * toStride;
	  while (toRight < end)
	  {
		T * rowEnd = toRight + mid;
		while (toRight < rowEnd)
		{
		  *toLeft++  = *fromLeft++;
		  *toRight++ = *fromRight++;
		}
		fromLeft += fromStep;
		fromRight += fromStep;
		toLeft += toStep;
		toRight += toStep;
	  }
	}
  }
}

/**
   \todo This code has latent bugs if the kernel is taller than the image.
 **/
template<class T>
static inline void convolveV (T * kernel, T * image, T * result,
							  BorderMode mode,
							  int kernelHeight, int width, int height,
							  int fromStride, int toStride)
{
  assert (kernelHeight <= height);

  // Main convolution
  int mid        = kernelHeight / 2;
  int last       = kernelHeight - 1;
  int cropHeight = height - last;
  int rowStep    = 1 - fromStride * kernelHeight;
  int fromStep   = fromStride - width;
  int toStep     = toStride   - width;
  T * kernelEnd  = kernel + last;
  T * fromPixel  = image;
  T * toPixel    = result;
  if (mode != Crop) toPixel += mid * toStride;
  T * end = toPixel + cropHeight * toStride;
  while (toPixel < end)
  {
	T * rowEnd = toPixel + width;
	while (toPixel < rowEnd)
	{
	  register T sum = 0;
	  T * k = kernelEnd;
	  while (k >= kernel)
	  {
		sum += *k-- * *fromPixel;
		fromPixel += fromStride;
	  }
	  *toPixel++ = sum;
	  fromPixel += rowStep;
	}
	toPixel += toStep;
	fromPixel += fromStep;
  }

  // Edge cases
  switch (mode)
  {
	case ZeroFill:
	{
	  int count = toStride * mid * sizeof (T);
	  memset (result, 0, count);
	  memset (end,    0, count);
	  break;
	}
	case UseZeros:
	{
	  T * kernelStart = kernel + mid;
	  T * fromTop    = image;
	  T * fromBottom = image + (height - 1) * fromStride;
	  T * toTopStart    = result;
	  T * toBottomStart = result + (height - 1) * toStride;
	  T * end           = toBottomStart + width;
	  while (toBottomStart < end)
	  {
		T * kernelTop    = kernelStart;
		T * kernelBottom = kernelStart;
		T * toTop    = toTopStart++;
		T * toBottom = toBottomStart++;
		while (kernelBottom > kernel)
		{
		  T * kt = kernelTop++;
		  T * kb = kernelBottom--;
		  T * ft = fromTop;
		  T * fb = fromBottom;
		  register T sumTop    = 0;
		  register T sumBottom = 0;
		  while (kt >= kernel)
		  {
			sumTop    += *kt-- * *ft;
			sumBottom += *kb++ * *fb;
			ft += fromStride;
			fb -= fromStride;
		  }
		  *toTop    = sumTop;
		  *toBottom = sumBottom;
		  toTop    += toStride;
		  toBottom -= toStride;
		}
		fromTop++;
		fromBottom++;
	  }
	  break;
	}
	case Boost:
	{
	  // Pre-compute partial sums of the kernel.
	  T * topTotal    = (T *) alloca (kernelHeight * sizeof (T));
	  T * bottomTotal = (T *) alloca (kernelHeight * sizeof (T));

	  double total = 0;
	  T * k = kernelEnd;
	  T * t = bottomTotal + last;
	  while (k >= kernel)
	  {
		*t-- = (T) (total += *k--);
	  }

	  total = 0;
	  k = kernel;
	  t = topTotal;
	  while (k <= kernelEnd)
	  {
		*t++ = (T) (total += *k++);
	  }

	  // Convolve
	  T * kernelStart      = kernel      + mid;
	  T * topTotalStart    = topTotal    + mid;
	  T * bottomTotalStart = bottomTotal + mid;
	  T * fromTop    = image;
	  T * fromBottom = image + (height - 1) * fromStride;
	  T * toTopStart    = result;
	  T * toBottomStart = result + (height - 1) * toStride;
	  T * end           = toBottomStart + width;
	  while (toBottomStart < end)
	  {
		T * kernelTop    = kernelStart;
		T * kernelBottom = kernelStart;
		T * tt           = topTotalStart;
		T * bt           = bottomTotalStart;
		T * toTop    = toTopStart++;
		T * toBottom = toBottomStart++;
		while (kernelBottom > kernel)
		{
		  T * kt = kernelTop++;
		  T * kb = kernelBottom--;
		  T * ft = fromTop;
		  T * fb = fromBottom;
		  register T sumTop    = 0;
		  register T sumBottom = 0;
		  while (kt >= kernel)
		  {
			sumTop    += *kt-- * *ft;
			sumBottom += *kb++ * *fb;
			ft += fromStride;
			fb -= fromStride;
		  }
		  *toTop    = sumTop    / *tt++;
		  *toBottom = sumBottom / *bt--;
		  toTop    += toStride;
		  toBottom -= toStride;
		}
		fromTop++;
		fromBottom++;
	  }
	  break;
	}
	case Copy:
	{
	  int count = toStride * mid * sizeof (T);
	  memcpy (result, image,                                   count);
	  memcpy (end,    image + (mid + cropHeight) * fromStride, count);
	}
  }
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

  PixelBufferPacked * k = (PixelBufferPacked *) buffer;
  if (! k) throw "kernel must be a packed buffer";

  Image result (*format);
  if (mode == Crop)
  {
	if (direction == Horizontal)
	{
	  result.resize (image.width - (width - 1), image.height);
	}
	else  // direction == Vertical
	{
	  result.resize (image.width, image.height - (width - 1));
	}
  }
  else
  {
	result.resize (image.width, image.height);
  }
  if (result.width == 0  ||  result.height == 0) return result;

  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  if (! i) throw "Convolution1D only handles packed buffers for now";

  if (direction == Horizontal)
  {
	if (*format == GrayFloat)
	{
	  convolveH ((float *) k->memory, (float *) i->memory, (float *) o->memory, mode, width, image.width, image.height, i->stride, o->stride);
	}
	else if (*format == GrayDouble)
	{
	  convolveH ((double *) k->memory, (double *) i->memory, (double *) o->memory, mode, width, image.width, image.height, i->stride, o->stride);
	}
	else
	{
	  throw "ConvolutionDiscrete1D::filter: unimplemented format";
	}
  }
  else  // direction == Vertical
  {
	if (*format == GrayFloat)
	{
	  convolveV ((float *) k->memory, (float *) i->memory, (float *) o->memory, mode, width, image.width, image.height, i->stride, o->stride);
	}
	else if (*format == GrayDouble)
	{
	  convolveV ((double *) k->memory, (double *) i->memory, (double *) o->memory, mode, width, image.width, image.height, i->stride, o->stride);
	}
	else
	{
	  throw "ConvolutionDiscrete1D::filter: unimplemented format";
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

  if (image.width == 0  ||  image.height == 0) return 0;
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
