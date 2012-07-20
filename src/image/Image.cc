/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/image.h"
#include "fl/time.h"

#include <assert.h>
#include <fstream>

// Include for tracing
//#include <iostream>

using namespace std;
using namespace fl;


// class Image ----------------------------------------------------------------

Image::Image ()
{
  timestamp = getTimestamp ();
  format    = &GrayChar;
  width     = 0;
  height    = 0;
}

Image::Image (const PixelFormat & format)
{
  timestamp    = getTimestamp ();
  this->format = &format;
  width        = 0;
  height       = 0;
}

Image::Image (int width, int height)
{
  timestamp = getTimestamp ();
  format    = &GrayChar;
  resize (width, height);
}

Image::Image (int width, int height, const PixelFormat & format)
{
  timestamp    = getTimestamp ();
  this->format = &format;
  resize (width, height);
}

Image::Image (const Image & that)
{
  buffer    = that.buffer;
  format    = that.format;
  timestamp = that.timestamp;
  width     = that.width;
  height    = that.height;
}

Image::Image (void * block, int width, int height, const PixelFormat & format)
{
  attach (block, width, height, format);
}

Image::Image (const MatrixAbstract<float> & A)
{
  attach (MatrixStrided<float> (A), GrayFloat);
}

Image::Image (const MatrixAbstract<double> & A)
{
  attach (MatrixStrided<double> (A), GrayDouble);
}

Image::Image (const std::string & fileName)
{
  read (fileName);
}

void
Image::read (const std::string & fileName)
{
  ImageFile file (fileName, "r");
  file.read (*this);
}

void
Image::read (istream & stream)
{
  ImageFile file (stream);
  file.read (*this);
  timestamp = getTimestamp ();
}

void
Image::write (const std::string & fileName, const std::string & formatName) const
{
  ImageFile file (fileName, "w", formatName);
  file.write (*this);
}

void
Image::write (ostream & stream, const std::string & formatName) const
{
  ImageFile file (stream, formatName);
  file.write (*this);
}

void
Image::copyFrom (const Image & that)
{
  if (that.buffer == 0) buffer = 0;
  else buffer = that.buffer->duplicate ();

  format    = that.format;
  width     = that.width;
  height    = that.height;
  timestamp = that.timestamp;
}

void
Image::copyFrom (void * block, int width, int height, const PixelFormat & format)
{
  timestamp    = getTimestamp ();
  this->width  = max (0, width);
  this->height = max (0, height);
  buffer       = format.attach (block, this->width, this->height, true);
  this->format = &format;
}

void
Image::attach (void * block, int width, int height, const PixelFormat & format)
{
  timestamp    = getTimestamp ();
  this->width  = max (0, width);
  this->height = max (0, height);
  buffer       = format.attach (block, this->width, this->height);
  this->format = &format;
}

void
Image::attach (const MatrixStrided<float> & A)
{
  attach (A, GrayFloat);
}

void
Image::attach (const MatrixStrided<double> & A)
{
  attach (A, GrayDouble);
}

void
Image::detach ()
{
  buffer = 0;
}

/**
   @todo Deeper support would come from creating a format->roi() function
   similar to format->attach().
 **/
Image
Image::roi (int fromX, int fromY, int width, int height)
{
  PixelBufferPacked * pbp = (PixelBufferPacked *) buffer;
  if (! pbp) throw "ROI requires packed buffer";
  fromX = max (0,                fromX);
  fromY = max (0,                fromY);
  fromX = min (this->width  - 1, fromX);
  fromY = min (this->height - 1, fromY);
  if (width  < 0) width  = this->width  - fromX;
  if (height < 0) height = this->height - fromY;
  Image result (*format);
  result.width  = width;
  result.height = height;
  result.buffer = new PixelBufferPacked
  (
    pbp->memory,
	pbp->stride,
	pbp->depth,
	pbp->offset + pbp->stride * fromY + pbp->depth * fromX
  );
  return result;
}

/**
   Changes image to new size.
   /param preserve If true, then any pixels that are still visible are
   aligned correctly and any newly exposed pixels are set to black.
   If false, then the content of the buffer is undefined.
**/
void
Image::resize (int width, int height, bool preserve)
{
  width  = max (width,  0);
  height = max (height, 0);
  if (! buffer.memory  ||  buffer->planes != format->planes)
  {
	buffer = format->buffer ();
  }
  buffer->resize (width, height, *format, preserve);
  this->width  = width;
  this->height = height;
}

void
Image::bitblt (const Image & that, int toX, int toY, int fromX, int fromY, int width, int height)
{
  // Adjust parameters
  if (fromX >= that.width  ||  fromY >= that.height)
  {
	return;
  }
  if (width < 0)
  {
	width = that.width;
  }
  if (height < 0)
  {
	height = that.height;
  }
  if (toX < 0)
  {
	width += toX;
	fromX -= toX;
	toX   -= toX;  // same as toX = 0, but this makes it clear what we are doing.
  }
  if (toY < 0)
  {
	height += toY;
	fromY  -= toY;
	toY    -= toY;
  }
  if (fromX < 0)
  {
	width += fromX;
	toX   -= fromX;
	fromX -= fromX;
  }
  if (fromY < 0)
  {
	height += fromY;
	toY    -= fromY;
	fromY  -= fromY;
  }
  width = min (fromX + width, that.width) - fromX;
  height = min (fromY + height, that.height) - fromY;
  if (width <= 0  ||  height <= 0)
  {
	return;
  }

  // Adjust size of target Image (ie: this)
  int needWidth = toX + width;
  int needHeight = toY + height;
  if (needWidth > this->width  ||  needHeight > this->height)
  {
	this->width  = max (this->width,  needWidth);
	this->height = max (this->height, needHeight);
	resize (this->width, this->height, true);  // also brings buffer into existence if it is null
  }

  // Get buffers
  Image source = that * (*(this->format));  // should also force conversion to packed format
  PixelBufferPacked * fromBuffer = (PixelBufferPacked *) source.buffer;
  PixelBufferPacked * toBuffer   = (PixelBufferPacked *) this->buffer;

  // Transfer the block
  int offsetX = fromX - toX;
  int offsetY = fromY - toY;
  if (fromBuffer  &&  toBuffer)  // Both buffers are packed, so we can use direct memory copy.
  {
	char * toByte;
	char * fromByte;
	char * rowEnd;
	char * end;
	int toStep     = toBuffer  ->stride - width * toBuffer  ->depth;
	int fromStep   = fromBuffer->stride - width * fromBuffer->depth;
	int fromStride = fromBuffer->stride;
	int Xstep;

    if (offsetX < 0)
    {
      if (offsetY < 0)  // backward x and y
	  {
		toByte   = (char *) toBuffer  ->base () + (toY   + height - 1) * toBuffer  ->stride + (toX   + width) * toBuffer  ->depth - 1;
		fromByte = (char *) fromBuffer->base () + (fromY + height - 1) * fromBuffer->stride + (fromX + width) * fromBuffer->depth - 1;
		end      = fromByte - height * fromBuffer->stride;
		toStep     *= -1;
		fromStep   *= -1;
		fromStride *= -1;
      }
      else  // backward x forward y
      {
		toByte   = (char *) toBuffer  ->base () + toY   * toBuffer  ->stride + (toX   + width) * toBuffer  ->depth - 1;
		fromByte = (char *) fromBuffer->base () + fromY * fromBuffer->stride + (fromX + width) * fromBuffer->depth - 1;
		end      = fromByte + height * fromBuffer->stride;
		toStep   = toBuffer->stride   + width * toBuffer  ->depth;
		fromStep = fromBuffer->stride + width * fromBuffer->depth;
      }
	  rowEnd = fromByte - width  * fromBuffer->depth;
	  Xstep = -1;
	}
    else
    {
	  if (offsetY < 0)  // forward x backward y
	  {
		toByte   = (char *) toBuffer  ->base () + (toY   + height - 1) * toBuffer  ->stride + toX   * toBuffer  ->depth;
		fromByte = (char *) fromBuffer->base () + (fromY + height - 1) * fromBuffer->stride + fromX * fromBuffer->depth;
		end      = fromByte - height * fromBuffer->stride;
		toStep   -= 2 * toBuffer  ->stride;
		fromStep -= 2 * fromBuffer->stride;
		fromStride *= -1;
	  }
      else  // forward x forward y
      {
		toByte   = (char *) toBuffer  ->base () + toY   * toBuffer  ->stride + toX   * toBuffer  ->depth;
		fromByte = (char *) fromBuffer->base () + fromY * fromBuffer->stride + fromX * fromBuffer->depth;
		end      = fromByte + height * fromBuffer->stride;
      }
	  rowEnd = fromByte + width  * fromBuffer->depth;
	  Xstep = 1;
    }

	while (fromByte != end)
	{
	  while (fromByte != rowEnd)
	  {
		*toByte = *fromByte;
		toByte   += Xstep;
		fromByte += Xstep;
	  }
	  toByte   += toStep;
	  fromByte += fromStep;
	  rowEnd   += fromStride;
	}
  }
  else  // Must use indirect pixel read and write
  {
    if (offsetX < 0)
    {
      if (offsetY < 0)
	  {
		for (int y = needHeight - 1; y >= toY; y--)
        {
		  for (int x = needWidth - 1; x >= toX; x--)
	      {
			setRGBA (x, y, source.getRGBA (x + offsetX, y + offsetY));
	      }
	    }
      }
      else
      {
		for (int y = toY; y < needHeight; y++)
        {
		  for (int x = needWidth - 1; x >= toX; x--)
	      {
			setRGBA (x, y, source.getRGBA (x + offsetX, y + offsetY));
	      }
	    }
      }
	}
    else
    {
	  if (offsetY < 0)
	  {
		for (int y = needHeight - 1; y >= toY; y--)
        {
		  for (int x = toX; x < needWidth; x++)
	      {
			setRGBA (x, y, source.getRGBA (x + offsetX, y + offsetY));
	      }
	    }
	  }
      else
      {
		for (int y = toY; y < needHeight; y++)
        {
		  for (int x = toX; x < needWidth; x++)
	      {
			setRGBA (x, y, source.getRGBA (x + offsetX, y + offsetY));
	      }
	    }
      }
    }
  }
}

void
Image::clear (unsigned int rgba)
{
  if (rgba)
  {
	for (int y = 0; y < height; y++)
	{
	  for (int x = 0; x < width; x++)
	  {
		format->setRGBA (buffer->pixel (x, y), rgba);
	  }
	}
  }
  else
  {
	buffer->clear ();
  }
}

Image
Image::operator + (const Image & that)
{
  const PixelFormat * outputFormat = &GrayFloat;
  if (max (format->precedence, that.format->precedence) > GrayFloat.precedence)
  {
	outputFormat = &GrayDouble;
  }

  Image result;
  int maxWidth  = max (width,  that.width);
  int maxHeight = max (height, that.height);

  #define add(size) \
  { \
	Matrix<size> A = (*this * *outputFormat).toMatrix<size> (); \
	Matrix<size> B = ( that * *outputFormat).toMatrix<size> (); \
	Matrix<size> C (maxWidth, maxHeight); \
	C.clear (); \
	C.region ((maxWidth -      width) / 2, (maxHeight -      height) / 2) += A; \
	C.region ((maxWidth - that.width) / 2, (maxHeight - that.height) / 2) += B; \
	result.attach (C); \
  }

  if (outputFormat == &GrayFloat) add (float)
  else add (double)

  return result;
}

Image
Image::operator - (const Image & that)
{
  const PixelFormat * outputFormat = &GrayFloat;
  if (max (format->precedence, that.format->precedence) > GrayFloat.precedence)
  {
	outputFormat = &GrayDouble;
  }

  Image result;
  int maxWidth  = max (width,  that.width);
  int maxHeight = max (height, that.height);

  #define subtract(size) \
  { \
	Matrix<size> A = (*this * *outputFormat).toMatrix<size> (); \
	Matrix<size> B = ( that * *outputFormat).toMatrix<size> (); \
	Matrix<size> C (maxWidth, maxHeight); \
	C.clear (); \
	C.region ((maxWidth -      width) / 2, (maxHeight -      height) / 2) += A; \
	C.region ((maxWidth - that.width) / 2, (maxHeight - that.height) / 2) -= B; \
	result.attach (C); \
  }

  if (outputFormat == &GrayFloat) subtract (float)
  else subtract (double)

  return result;
}

Image
Image::operator * (double factor)
{
  PixelBufferPacked * p = (PixelBufferPacked *) buffer;
  if (! p) throw "Can't multiply non-packed buffer yet.";

  Image result (width, height, *format);
  PixelBufferPacked * q = (PixelBufferPacked *) result.buffer;

  if (*format == GrayFloat)
  {
	float * fromPixel = (float *) p->base ();
	float * toPixel   = (float *) q->base ();
	float * end       = toPixel + width * height;
	while (fromPixel < end)
	{
	  *toPixel++ = *fromPixel++ * factor;
	}
  }
  else if (*format == GrayDouble)
  {
	double * fromPixel = (double *) p->base ();
	double * toPixel   = (double *) q->base ();
	double * end       = toPixel + width * height;
	while (fromPixel < end)
	{
	  *toPixel++ = *fromPixel++ * factor;
	}
  }
  else if (*format == GrayChar)
  {
	int ifactor = (int) (factor * (1 << 16));
	unsigned char * fromPixel = (unsigned char *) p->base ();
	unsigned char * toPixel   = (unsigned char *) q->base ();
	unsigned char * end       = toPixel + width * height;
	while (fromPixel < end)
	{
	  *toPixel++ = (*fromPixel * ifactor) >> 16;
	}
  }
  else
  {
	throw "Image::operator * : unimplemented format";
  }

  return result;
}

Image &
Image::operator *= (double factor)
{
  PixelBufferPacked * p = (PixelBufferPacked *) buffer;
  if (! p) throw "Can't multiply non-packed buffer yet.";

  if (*format == GrayFloat)
  {
	this->toMatrix<float> () *= factor;
  }
  else if (*format == GrayDouble)
  {
	this->toMatrix<double> () *= factor;
  }
  else
  {
	throw "Image::operator *= : unimplemented format";
  }

  return *this;
}

Image &
Image::operator += (double value)
{
  PixelBufferPacked * p = (PixelBufferPacked *) buffer;
  if (! p) throw "Can't multiply non-packed buffer yet.";

  if (*format == GrayFloat)
  {
	this->toMatrix<float> () += value;
  }
  else if (*format == GrayDouble)
  {
	this->toMatrix<double> () += value;
  }
  else
  {
	throw "Image::operator += : unimplemented format";
  }

  return *this;
}
