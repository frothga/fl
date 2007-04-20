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
Revision 1.12  2007/04/20 13:34:12  Fred
First working version of assembly optimizations!  This is for horizontal case
only, but will add vertical later.  Also, it is only for GCC/x86.  This code
only improves GCC's output in -03 by about 5% to 10%.  To be worth it, this
should be written for SSE rather than x87.  Also, might be worth abondoning
the "reverse" kernel, since incrementing pointers is almost zero cost when
interleaved with x87 instructions.

Changed stride to be in bytes rather than pixels.

Fixed response() to handle new border modes.

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

// for debugging only
#include "fl/time.h"


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
   Generic template for horizontal main loop.
   Validated by test.cc w/ threshold = 3e-6
**/
template<class T>
static inline void
asmH (T * reverse, T * fromPixel, T * toPixel, T * end,
	  int rowWidth, int last, int fromStep, int toStep)
{
  T * reverseEnd = reverse + last;

  while (toPixel < end)
  {
	T * rowEnd = toPixel + rowWidth;
	while (toPixel < rowEnd)
	{
	  register T sum = 0;
	  T * r = reverse;
	  while (r <= reverseEnd)
	  {
		sum += *r++ * *fromPixel++;
	  }
	  *toPixel++ = sum;
	  fromPixel -= last;  // net advance of 1 pixel
	}
	toPixel   = (T *) ((char *) toPixel   + toStep);
	fromPixel = (T *) ((char *) fromPixel + fromStep);
  }
}

#if defined (__GNUC__)  &&  defined (__i386__)

/**
   Float specialization of horizontal main loop.
**/
template<>
static inline void
asmH (float * reverse, float * fromPixel, float * toPixel, float * end,
	  int rowWidth, int last, int fromStep, int toStep)
{
  reverse--;
  fromPixel--;
  last++;  // same as kernelWidth
  rowWidth *= sizeof (float);  // pre-scale for convenience

  __asm (
		 // Load registers  (don't trust gcc to do this right)
		 "movl   %0, %%eax\n"  // toPixel
		 "movl   %1, %%ebx\n"  // fromPixel
		 //  ecx is inner loop index
		 "movl   %2, %%edx\n"  // reverse
		 "movl   %3, %%esi\n"  // last
		 //  edi is rowEnd

		 // while (toPixel < end)
		 "jmp    5f\n"
		 "1:"
		 "movl   %%eax, %%edi\n"
		 "addl   %4, %%edi\n"  // rowEnd = toPixel + rowWidth

		 // while (toPixel < rowEnd)
		 "jmp    4f\n"
		 "2:"
		 "movl   %%esi, %%ecx\n"
		 "fldz\n"
		 "3:"
		 "flds   (%%ebx,%%ecx,4)\n"
		 "fmuls  (%%edx,%%ecx,4)\n"
		 "decl   %%ecx\n"
		 "faddp\n"
		 "jnz    3b\n"
		 "fstps  (%%eax)\n"

		 "addl   $4, %%eax\n"  // toPixel++
		 "addl   $4, %%ebx\n"  // fromPixel++
		 "4:"
		 "cmpl   %%edi, %%eax\n"
		 "jb     2b\n"

		 "addl   %5, %%eax\n"   // toPixel += toStep
		 "addl   %6, %%ebx\n"   // fromPixel += fromStep
		 "5:"
		 "cmpl   %7, %%eax\n"
		 "jb     1b\n"
		 
		 :
		 : "m" (toPixel), "m" (fromPixel), "m" (reverse), "m" (last), "m" (rowWidth), "m" (toStep), "m" (fromStep), "m" (end)
		 : "eax", "ebx", "ecx", "edx", "esi", "edi"
		 );
}

/**
   Double specialization of horizontal main loop.
**/
template<>
static inline void
asmH (double * reverse, double * fromPixel, double * toPixel, double * end,
	  int rowWidth, int last, int fromStep, int toStep)
{
  reverse--;
  fromPixel--;
  last++;
  rowWidth *= sizeof (double);

  __asm (
		 "movl   %0, %%eax\n"
		 "movl   %1, %%ebx\n"
		 "movl   %2, %%edx\n"
		 "movl   %3, %%esi\n"

		 "jmp    5f\n"
		 "1:"
		 "movl   %%eax, %%edi\n"
		 "addl   %4, %%edi\n"

		 "jmp    4f\n"
		 "2:"
		 "movl   %%esi, %%ecx\n"
		 "fldz\n"
		 "3:"
		 "fldl   (%%ebx,%%ecx,8)\n"
		 "fmull  (%%edx,%%ecx,8)\n"
		 "decl   %%ecx\n"
		 "faddp\n"
		 "jnz    3b\n"
		 "fstpl  (%%eax)\n"

		 "addl   $8, %%eax\n"
		 "addl   $8, %%ebx\n"
		 "4:"
		 "cmpl   %%edi, %%eax\n"
		 "jb     2b\n"

		 "addl   %5, %%eax\n"
		 "addl   %6, %%ebx\n"
		 "5:"
		 "cmpl   %7, %%eax\n"
		 "jb     1b\n"
		 :
		 : "m" (toPixel), "m" (fromPixel), "m" (reverse), "m" (last), "m" (rowWidth), "m" (toStep), "m" (fromStep), "m" (end)
		 : "eax", "ebx", "ecx", "edx", "esi", "edi"
		 );
}

#endif

/**
   \todo This code has latent bugs if the kernel is wider than the image.
 **/
template<class T>
static void
convolveH (T * kernel, T * image, T * result,
		   BorderMode mode,
		   int kernelWidth, int width, int height,
		   int fromStride, int toStride)
{
  assert (kernelWidth <= width);

  // Main convolution

  int mid       = kernelWidth / 2;
  int last      = kernelWidth - 1;
  int rowWidth  = width - last;
  int fromStep  = fromStride - rowWidth * sizeof (T);
  int toStep    = toStride   - rowWidth * sizeof (T);
  T * kernelEnd = kernel + last;
  T * fromPixel = image;
  T * toPixel   = result;
  if (mode != Crop) toPixel += mid;
  T * end = (T *) ((char *) toPixel + height * toStride);

  //   Reverse the kernel for use in assembly optimization
  T * reverse = (T *) alloca (kernelWidth * sizeof (T));
  T * r = reverse;
  T * k = kernelEnd;
  while (k >= kernel) *r++ = *k--;

  asmH (reverse, fromPixel, toPixel, end, rowWidth, last, fromStep, toStep);

  // Edge cases
  switch (mode)
  {
	case ZeroFill:
	{
	  toStep    = toStride - mid * sizeof (T);
	  T * left  = result;
	  T * right = result + mid + rowWidth;
	  T * end   = (T *) ((char *) result + height * toStride);
	  while (right < end)
	  {
		T * rowEnd = right + mid;
		while (right < rowEnd)
		{
		  *left++  = (T) 0.0;
		  *right++ = (T) 0.0;
		}
		left = (T *) ((char *) left + toStep);
		right = (T *) ((char *) right + toStep);
	  }
	  break;
	}
	case UseZeros:
	{
	  int toLeftStep  = toStride - mid * sizeof (T);
	  int toRightStep = toStride + mid * sizeof (T);
	  T * kernelStart = kernel + mid;
	  T * fromLeft  = image;
	  T * fromRight = image + mid + rowWidth + mid - 1;
	  T * toLeft  = result;
	  T * toRight = result + mid + rowWidth + mid - 1;
	  T * end     = (T *) ((char *) result + height * toStride);
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
		fromLeft  = (T *) ((char *) fromLeft + fromStride);
		fromRight = (T *) ((char *) fromRight + fromStride);
		toLeft  = (T *) ((char *) toLeft + toLeftStep);
		toRight = (T *) ((char *) toRight + toRightStep);
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
	  int toLeftStep  = toStride - mid * sizeof (T);
	  int toRightStep = toStride + mid * sizeof (T);
	  T * kernelStart = kernel + mid;
	  T * leftTotalStart = leftTotal + mid;
	  T * rightTotalStart = rightTotal + mid;
	  T * fromLeft  = image;
	  T * fromRight = image + mid + rowWidth + mid - 1;
	  T * toLeft  = result;
	  T * toRight = result + mid + rowWidth + mid - 1;
	  T * end   = (T *) ((char *) result + height * toStride);
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
		fromLeft  = (T *) ((char *) fromLeft  + fromStride);
		fromRight = (T *) ((char *) fromRight + fromStride);
		toLeft  = (T *) ((char *) toLeft  + toLeftStep);
		toRight = (T *) ((char *) toRight + toRightStep);
	  }
	  break;
	}
	case Copy:
	{
	  fromStep      = fromStride - mid * sizeof (T);
	  toStep        = toStride   - mid * sizeof (T);
	  T * fromLeft  = image;
	  T * fromRight = image + (mid + rowWidth);
	  T * toLeft    = result;
	  T * toRight   = result + (mid + rowWidth);
	  T * end       = (T *) ((char *) result + height * toStride);
	  while (toRight < end)
	  {
		T * rowEnd = toRight + mid;
		while (toRight < rowEnd)
		{
		  *toLeft++  = *fromLeft++;
		  *toRight++ = *fromRight++;
		}
		fromLeft  = (T *) ((char *) fromLeft  + fromStep);
		fromRight = (T *) ((char *) fromRight + fromStep);
		toLeft  = (T *) ((char *) toLeft  + toStep);
		toRight = (T *) ((char *) toRight + toStep);
	  }
	}
  }
}

/**
   \todo This code has latent bugs if the kernel is taller than the image.
 **/
template<class T>
static void
convolveV (T * kernel, T * image, T * result,
		   BorderMode mode,
		   int kernelHeight, int width, int height,
		   int fromStride, int toStride)
{
  assert (kernelHeight <= height);

  // Main convolution
  int mid        = kernelHeight / 2;
  int last       = kernelHeight - 1;
  int cropHeight = height - last;
  int rowStep    = sizeof (T) - fromStride * kernelHeight;
  int fromStep   = fromStride - width * sizeof (T);
  int toStep     = toStride   - width * sizeof (T);
  T * kernelEnd  = kernel + last;
  T * fromPixel  = image;
  T * toPixel    = result;
  if (mode != Crop) toPixel = (T *) ((char *) toPixel + mid * toStride);
  T * end = (T *) ((char *) toPixel + cropHeight * toStride);
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
		fromPixel = (T *) ((char *) fromPixel + fromStride);
	  }
	  *toPixel++ = sum;
	  fromPixel = (T *) ((char *) fromPixel + rowStep);
	}
	toPixel   = (T *) ((char *) toPixel   + toStep);
	fromPixel = (T *) ((char *) fromPixel + fromStep);
  }

  // Edge cases
  switch (mode)
  {
	case ZeroFill:
	{
	  int count = toStride * mid;
	  memset (result, 0, count);
	  memset (end,    0, count);
	  break;
	}
	case UseZeros:
	{
	  T * kernelStart = kernel + mid;
	  T * fromTop    = image;
	  T * fromBottom = (T *) ((char *) image + (height - 1) * fromStride);
	  T * toTopStart    = result;
	  T * toBottomStart = (T *) ((char *) result + (height - 1) * toStride);
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
			ft = (T *) ((char *) ft + fromStride);
			fb = (T *) ((char *) fb - fromStride);
		  }
		  *toTop    = sumTop;
		  *toBottom = sumBottom;
		  toTop    = (T *) ((char *) toTop    + toStride);
		  toBottom = (T *) ((char *) toBottom - toStride);
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
	  T * fromBottom = (T *) ((char *) image + (height - 1) * fromStride);
	  T * toTopStart    = result;
	  T * toBottomStart = (T *) ((char *) result + (height - 1) * toStride);
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
			ft = (T *) ((char *) ft + fromStride);
			fb = (T *) ((char *) fb - fromStride);
		  }
		  *toTop    = sumTop    / *tt++;
		  *toBottom = sumBottom / *bt--;
		  toTop    = (T *) ((char *) toTop    + toStride);
		  toBottom = (T *) ((char *) toBottom - toStride);
		}
		fromTop++;
		fromBottom++;
	  }
	  break;
	}
	case Copy:
	{
	  int count = toStride * mid;
	  memcpy (result, image,                                            count);
	  memcpy (end,    (char *) image + (mid + cropHeight) * fromStride, count);
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
  char * input = (char *) imageBuffer->memory;

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
	stride = (int) format->depth;
  }
  else  // direction == Vertical
  {
	low  = max (0,    y + mid - (image.height - 1));
	high = min (last, y + mid);
	stride = imageBuffer->stride;
  }

  if (low > 0  ||  high < last)
  {
	if (mode == Copy)
	{
	  if      (*format == GrayFloat)  return ((float  *) (input + y * imageBuffer->stride))[x];
	  else if (*format == GrayDouble) return ((double *) (input + y * imageBuffer->stride))[x];
	  else return 0;
	}
	if (mode == Crop  ||  mode == ZeroFill  ||  mode == Undefined) return 0;
  }

  if (mode == Boost)
  {
	if (*format == GrayFloat)
	{
	  float * a = (float *) kernel + low;
	  float * b = (float *) (input + y * imageBuffer->stride) + x;
	  b = (float *) ((char *) b + (mid - low) * stride);

	  float result = 0;
	  float weight = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a * *b;
		weight += *a++;
		b = (float *) ((char *) b - stride);
	  }
	  return result / weight;
	}
	else if (*format == GrayDouble)
	{
	  double * a = (double *) kernel + low;
	  double * b = (double *) (input + y * imageBuffer->stride) + x;
	  b = (double *) ((char *) b + (mid - low) * stride);

	  double result = 0;
	  double weight = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a * *b;
		weight += *a++;
		b = (double *) ((char *) b - stride);
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
	  float * b = (float *) (input + y * imageBuffer->stride) + x;
	  b = (float *) ((char *) b + (mid - low) * stride);

	  float result = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a++ * *b;
		b = (float *) ((char *) b - stride);
	  }
	  return result;
	}
	else if (*format == GrayDouble)
	{
	  double * a = (double *) kernel + low;
	  double * b = (double *) (input + y * imageBuffer->stride) + x;
	  b = (double *) ((char *) b + (mid - low) * stride);

	  double result = 0;
	  for (int i = low; i <= high; i++)
	  {
		result += *a++ * *b;
		b = (double *) ((char *) b - stride);
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
