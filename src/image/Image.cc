/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
*/


#include "fl/image.h"
#include "fl/time.h"

#include <stdio.h>
#include <sys/stat.h>
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
  this->format = &GrayChar;
  width = 0;
  height = 0;
}

Image::Image (const PixelFormat & format)
{
  timestamp = getTimestamp ();
  this->format = &format;
  width = 0;
  height = 0;
}

Image::Image (int width, int height)
: buffer (width * height * GrayChar.depth)
{
  timestamp = getTimestamp ();
  this->format = &GrayChar;
  this->width  = max (0, width);  // Note that if a negative width or height was specified, the Pointer class would handle it correctly when constructing the buffer.
  this->height = max (0, height);
}

Image::Image (int width, int height, const PixelFormat & format)
: buffer (width * height * format.depth)
{
  timestamp = getTimestamp ();
  this->format = &format;
  this->width  = max (0, width);
  this->height = max (0, height);
}

Image::Image (const Image & that)
: buffer (that.buffer)
{
  format    = that.format;
  width     = that.width;
  height    = that.height;
  timestamp = that.timestamp;
}

Image::Image (unsigned char * buffer, int width, int height, const PixelFormat & format)
: buffer (buffer, width * height * format.depth)
{
  timestamp = getTimestamp ();
  this->format = &format;
  this->width  = max (0, width);
  this->height = max (0, height);
}

Image::Image (const std::string & fileName)
{
  // Set these so resize() will work right.
  width = 0;
  height = 0;

  read (fileName);
}

void
Image::read (const std::string & fileName)
{
  ImageFileFormat * f = ImageFileFormat::find (fileName);
  if (! f)
  {
	throw "Unrecognized file format for image.";
  }

  f->read (fileName, *this);

  // Use stat () to determine timestamp.
  struct stat info;
  stat (fileName.c_str (), &info);
  timestamp = info.st_mtime;  // Does this need more work to align it with getTimestamp () values?
}

void
Image::read (istream & stream)
{
  if (stream.good ())
  {
	ImageFileFormat * f = ImageFileFormat::find (stream);
	if (! f)
	{
	  throw "Unrecognized file format for image.";
	}

	f->read (stream, *this);
  }

  timestamp = getTimestamp ();
}

void
Image::write (const std::string & fileName, const std::string & formatName) const
{
  ImageFileFormat * f = ImageFileFormat::findName (formatName);
  if (! f)
  {
	throw "Unrecognized file format for image.";
  }

  f->write (fileName, *this);
}

void
Image::write (ostream & stream, const std::string & formatName) const
{
  ImageFileFormat * f = ImageFileFormat::findName (formatName);
  if (! f)
  {
	throw "Unrecognized file format for image.";
  }

  f->write (stream, *this);
}

void
Image::copyFrom (const Image & that)
{
  buffer.copyFrom (that.buffer);
  format    = that.format;
  width     = that.width;
  height    = that.height;
  timestamp = that.timestamp;
}

void
Image::copyFrom (unsigned char * buffer, int width, int height, const PixelFormat & format)
{
  if (this->buffer.memory != buffer)
  {
	timestamp = getTimestamp ();  // Actually, we don't know the timestamp on a bare buffer, but this guess is as good as any.

	this->format = &format;
	this->width  = max (0, width);
	this->height = max (0, height);

	this->buffer.copyFrom (buffer, width * height * format.depth);
  }
}

void
Image::attach (unsigned char * buffer, int width, int height, const PixelFormat & format)
{
  this->buffer.attach (buffer, width * height * format.depth);
  timestamp = getTimestamp ();
  this->format = &format;
  this->width  = max (0, width);
  this->height = max (0, height);
}

void
Image::detach ()
{
  buffer.detach ();
  width = 0;
  height = 0;
}

struct triad
{
  unsigned char channel[3];
};

void
Image::resize (const int width, const int height)
{
  if (width <= 0  ||  height <= 0)
  {
	this->width  = 0;
	this->height = 0;
	buffer.detach ();
	return;
  }

  int needWidth = min (this->width, width);
  int needHeight = min (this->height, height);

  if (width == this->width)
  {
	if (height > this->height)
	{
	  Pointer temp (buffer);
	  buffer.detach ();
	  buffer.grow (width * height * format->depth);
	  int count = width * needHeight * format->depth;
	  memcpy (buffer.memory, temp.memory, count);
	  assert (count >= 0  &&  count < buffer.size ());
	  memset ((char *) buffer + count, 0, buffer.size () - count);
	}
	this->height = height;
  }
  else
  {
    int oldwidth = this->width;
	this->width = width;
	this->height = height;
	Pointer temp (buffer);
	buffer.detach ();
	buffer.grow (width * height * format->depth);
	buffer.clear ();

    #define reshuffle(size) \
    { \
	  for (int x = 0; x < needWidth; x++) \
	  { \
	    for (int y = 0; y < needHeight; y++) \
	    { \
		  ((size *) buffer)[y * width + x] = ((size *) temp)[y * oldwidth + x]; \
	    } \
	  } \
	}
	switch (format->depth)
	{
      case 2:
		reshuffle (unsigned short);
		break;
      case 3:
		reshuffle (triad);
		break;
      case 4:
		reshuffle (unsigned int);
		break;
      case 8:
		reshuffle (double);
		break;
      case 1:
      default:
		reshuffle (unsigned char);
	}
  }
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

  // Get source image in same format we are.
  Image source = that * (*(this->format));

  // Adjust size of target Image (ie: this)
  int needWidth = toX + width;
  int needHeight = toY + height;
  if (needWidth > this->width  ||  needHeight > this->height)
  {
	resize (max (this->width, needWidth), max (this->height, needHeight));
  }

  // Transfer the block
  int offsetX = fromX - toX;
  int offsetY = fromY - toY;

  #define transfer(size) \
  { \
    if (offsetX < 0) \
    { \
      if (offsetY < 0) \
	  { \
        for (int x = needWidth - 1; x >= toX; x--) \
        { \
	      for (int y = needHeight - 1; y >= toY; y--) \
	      { \
            ((size *) buffer)[y * this->width + x] = ((size *) source.buffer)[(y + offsetY) * source.width + (x + offsetX)]; \
	      } \
	    } \
      } \
      else \
      { \
        for (int x = needWidth - 1; x >= toX; x--) \
        { \
	      for (int y = toY; y < needHeight; y++) \
	      { \
            ((size *) buffer)[y * this->width + x] = ((size *) source.buffer)[(y + offsetY) * source.width + (x + offsetX)]; \
	      } \
	    } \
      } \
	} \
    else \
    { \
	  if (offsetY < 0) \
	  { \
        for (int x = toX; x < needWidth; x++) \
        { \
	      for (int y = needHeight - 1; y >= toY; y--) \
	      { \
            ((size *) buffer)[y * this->width + x] = ((size *) source.buffer)[(y + offsetY) * source.width + (x + offsetX)]; \
	      } \
	    } \
	  } \
      else \
      { \
        for (int x = toX; x < needWidth; x++) \
        { \
	      for (int y = toY; y < needHeight; y++) \
	      { \
            ((size *) buffer)[y * this->width + x] = ((size *) source.buffer)[(y + offsetY) * source.width + (x + offsetX)]; \
	      } \
	    } \
      } \
    } \
  }

  switch (format->depth)
  {
    case 2:
	  transfer (unsigned short);
	  break;
    case 3:
	  transfer (triad);
	  break;
    case 4:
	  transfer (unsigned int);
	  break;
    case 8:
	  transfer (double);
	  break;
    case 1:
    default:
	  transfer (unsigned char);
  }
}

void
Image::clear (unsigned int rgba)
{
  if (rgba)
  {
	unsigned char * pixel = (unsigned char *) buffer;
	unsigned char * end = pixel + width * height * format->depth;
	while (pixel < end)
	{
	  format->setRGBA (pixel, rgba);
	  pixel += format->depth;
	}
  }
  else
  {
	buffer.clear ();
  }
}


Image
Image::operator + (const Image & that)
{
  if (max (format->precedence, that.format->precedence) <= GrayFloat.precedence)
  {
	if (*format != GrayFloat  ||  *that.format != GrayFloat)
	{
	  return (*this * GrayFloat) + (that * GrayFloat);
	}
  }
  else
  {
	if (*format != GrayDouble  ||  *that.format != GrayDouble)
	{
	  return (*this * GrayDouble) + (that * GrayDouble);
	}
  }

  Image result (max (width, that.width), max (height, that.height), *format);

  int offsetX = abs (width  - that.width)  / 2;
  int offsetY = abs (height - that.height) / 2;
  int sharedWidth = min (width, that.width);
  int sharedHeight = min (height, that.height);
  assert (offsetX * 2 == abs (width - that.width)  &&  offsetY * 2 == abs (height - that.height));  // avoid odd-even mismatch

  #define add(size) \
  { \
	size * start1 = (size *) buffer; \
	size * start2 = (size *) that.buffer; \
	size * start  = (size *) result.buffer; \
    \
	size * pixel1 = start1; \
	size * pixel2 = start2; \
	if (width > that.width) pixel1 += offsetX; \
	else                    pixel2 += offsetX; \
	if (height > that.height) pixel1 += offsetY * width; \
	else                      pixel2 += offsetY * that.width; \
	size * pixel  = start + (offsetX + offsetY * result.width); \
    \
	int advance1 = width        - sharedWidth; \
	int advance2 = that.width   - sharedWidth; \
	int advance  = result.width - sharedWidth; \
    \
	size * end = start + (result.width * (result.height - offsetY) - offsetX); \
	while (pixel < end) \
	{ \
	  size * rowEnd = pixel + sharedWidth; \
	  while (pixel < rowEnd) \
	  { \
		*pixel++ = *pixel1++ + *pixel2++; \
	  } \
	  pixel1 += advance1; \
	  pixel2 += advance2; \
	  pixel  += advance; \
	} \
    \
	if (height != that.height) \
	{ \
	  size * sourceT; \
	  size * sourceB; \
	  int sourceWidth; \
	  if (height > that.height) \
	  { \
		sourceT = start1; \
		sourceB = start1 + ((height - offsetY) * width); \
		sourceWidth = width; \
	  } \
	  else \
	  { \
		sourceT = start2; \
		sourceB = start2 + ((that.height - offsetY) * that.width); \
		sourceWidth = that.width; \
	  } \
	  size * pixelT = start; \
	  if (sourceWidth < result.width) pixelT += offsetX; \
	  size * pixelB = pixelT + ((result.height - offsetY) * result.width); \
      \
	  int advance = result.width - sourceWidth; \
      \
	  size * end = sourceT + offsetY * sourceWidth; \
	  while (sourceT < end) \
	  { \
		size * rowEnd = sourceT + sourceWidth; \
		while (sourceT < rowEnd) \
		{ \
		  *pixelT++ = *sourceT++; \
		  *pixelB++ = *sourceB++; \
		} \
		pixelT += advance; \
		pixelB += advance; \
	  } \
	} \
    \
	if (width != that.width) \
	{ \
	  size * sourceL; \
	  size * end; \
	  int sourceHeight; \
	  if (width > that.width) \
	  { \
		sourceL = start1; \
		end     = start1 + width * height; \
		sourceHeight = height; \
	  } \
	  else \
	  { \
		sourceL = start2; \
		end     = start2 + that.width * that.height; \
		sourceHeight = that.height; \
	  } \
	  if (sourceHeight > sharedHeight) \
	  { \
		sourceL += offsetY * result.width; \
		end     -= offsetY * result.width; \
	  } \
	  size * sourceR = sourceL + (result.width - offsetX); \
	  size * pixelL  = start + offsetY * result.width; \
	  size * pixelR  = pixelL + (result.width - offsetX); \
      \
	  int advance = result.width - offsetX; \
      \
	  while (sourceR < end) \
	  { \
		size * rowEnd = sourceR + offsetX; \
		while (sourceR < rowEnd) \
		{ \
		  *pixelL++ = *sourceL++; \
		  *pixelR++ = *sourceR++; \
		} \
		sourceL = sourceR; \
		sourceR += advance; \
		pixelL = pixelR; \
		pixelR += advance; \
	  } \
	} \
    \
	if ((width - that.width) * (height - that.height) < 0) \
	{ \
	  size * pixelTL = start; \
	  size * pixelTR = start + (result.width - offsetX); \
	  size * pixelBL = pixelTL + ((result.height - offsetY) * result.width); \
	  size * pixelBR = pixelTR + ((result.height - offsetY) * result.width); \
      \
	  int advance = result.width - offsetX; \
      \
	  size * end = start + result.width * result.height; \
	  while (pixelBR < end) \
	  { \
		size * rowEnd = pixelBR + offsetX; \
		while (pixelBR < rowEnd) \
		{ \
		  *pixelTL++ = 0.0f; \
		  *pixelTR++ = 0.0f; \
		  *pixelBL++ = 0.0f; \
		  *pixelBR++ = 0.0f; \
		} \
		pixelTL = pixelTR; \
		pixelBL = pixelBR; \
		pixelTR += advance; \
		pixelBR += advance; \
	  } \
	} \
  }

  if (*format == GrayFloat) add (float)
  else add (double)

  return result;
}

Image
Image::operator - (const Image & that)
{
  if (max (format->precedence, that.format->precedence) <= GrayFloat.precedence)
  {
	if (*format != GrayFloat  ||  *that.format != GrayFloat)
	{
	  return (*this * GrayFloat) - (that * GrayFloat);
	}
  }
  else
  {
	if (*format != GrayDouble  ||  *that.format != GrayDouble)
	{
	  return (*this * GrayDouble) - (that * GrayDouble);
	}
  }

  Image result (max (width, that.width), max (height, that.height), *format);

  int offsetX = abs (width  - that.width)  / 2;
  int offsetY = abs (height - that.height) / 2;
  int sharedWidth = min (width, that.width);
  assert (offsetX * 2 == abs (width - that.width)  &&  offsetY * 2 == abs (height - that.height));

  #define subtract(size) \
  { \
	size * start1 = (size *) buffer; \
	size * start2 = (size *) that.buffer; \
	size * start  = (size *) result.buffer; \
    \
	size * pixel1 = start1; \
	size * pixel2 = start2; \
	if (width > that.width) pixel1 += offsetX; \
	else                    pixel2 += offsetX; \
	if (height > that.height) pixel1 += offsetY * width; \
	else                      pixel2 += offsetY * that.width; \
	size * pixel  = start + (offsetX + offsetY * result.width); \
    \
	int advance1 = width        - sharedWidth; \
	int advance2 = that.width   - sharedWidth; \
	int advance  = result.width - sharedWidth; \
    \
	size * end = start + (result.width * (result.height - offsetY) - offsetX); \
	while (pixel < end) \
	{ \
	  size * rowEnd = pixel + sharedWidth; \
	  while (pixel < rowEnd) \
	  { \
		*pixel++ = *pixel1++ - *pixel2++; \
	  } \
	  pixel1 += advance1; \
	  pixel2 += advance2; \
	  pixel  += advance; \
	} \
    \
	if (height > that.height) \
	{ \
	  size * pixel1T = start1; \
	  size * pixel1B = start1 + ((height - offsetY) * width); \
	  size * pixelT  = start; \
	  if (width < result.width) pixelT += offsetX; \
	  size * pixelB  = pixelT + ((result.height - offsetY) * result.width); \
      \
	  int advance = result.width - width; \
      \
	  size * end = pixel1T + offsetY * width; \
	  while (pixel1T < end) \
	  { \
		size * rowEnd = pixel1T + width; \
		while (pixel1T < rowEnd) \
		{ \
		  *pixelT++ = *pixel1T++; \
		  *pixelB++ = *pixel1B++; \
		} \
		pixelT += advance; \
		pixelB += advance; \
	  } \
	} \
	else if (height < that.height) \
	{ \
	  size * pixel2T = start2; \
	  size * pixel2B = start2 + ((that.height - offsetY) * that.width); \
	  size * pixelT  = start; \
	  if (that.width < result.width) pixelT += offsetX; \
	  size * pixelB  = pixelT + ((result.height - offsetY) * result.width); \
      \
	  int advance = result.width - that.width; \
      \
	  size * end = pixel2T + offsetY * that.width; \
	  while (pixel2T < end) \
	  { \
		size * rowEnd = pixel2T + that.width; \
		while (pixel2T < rowEnd) \
		{ \
		  *pixelT++ = - *pixel2T++; \
		  *pixelB++ = - *pixel2B++; \
		} \
		pixelT += advance; \
		pixelB += advance; \
	  } \
	} \
    \
	if (width > that.width) \
	{ \
	  size * pixel1L = start1; \
	  size * end     = start1 + width * height; \
	  if (height > that.height) \
	  { \
		pixel1L += offsetY * width; \
		end -= offsetY * width; \
	  } \
	  size * pixel1R = pixel1L + (width - offsetX); \
	  size * pixelL  = start + offsetY * result.width; \
	  size * pixelR  = pixelL + (result.width - offsetX); \
      \
	  int advance = width - offsetX; \
      \
	  while (pixel1R < end) \
	  { \
		size * rowEnd = pixel1R + offsetX; \
		while (pixel1R < rowEnd) \
		{ \
		  *pixelL++ = *pixel1L++; \
		  *pixelR++ = *pixel1R++; \
		} \
		pixel1L = pixel1R; \
		pixel1R += advance; \
		pixelL = pixelR; \
		pixelR += advance; \
	  } \
	} \
	else if (width < that.width) \
	{ \
	  size * pixel2L = start2; \
	  size * end     = start2 + that.width * that.height; \
	  if (that.height > height) \
	  { \
		pixel2L += offsetY * that.width; \
		end -= offsetY * that.width; \
	  } \
	  size * pixel2R = pixel2L + (that.width - offsetX); \
	  size * pixelL  = start + offsetY * result.width; \
	  size * pixelR  = pixelL + (result.width - offsetX); \
      \
	  int advance = that.width - offsetX; \
      \
	  while (pixel2R < end) \
	  { \
		size * rowEnd = pixel2R + offsetX; \
		while (pixel2R < rowEnd) \
		{ \
		  *pixelL++ = - *pixel2L++; \
		  *pixelR++ = - *pixel2R++; \
		} \
		pixel2L = pixel2R; \
		pixel2R += advance; \
		pixelL = pixelR; \
		pixelR += advance; \
	  } \
	} \
    \
	if ((width - that.width) * (height - that.height) < 0) \
	{ \
	  size * pixelTL = start; \
	  size * pixelTR = start + (result.width - offsetX); \
	  size * pixelBL = pixelTL + ((result.height - offsetY) * result.width); \
	  size * pixelBR = pixelTR + ((result.height - offsetY) * result.width); \
      \
	  int advance = result.width - offsetX; \
      \
	  size * end = start + result.width * result.height; \
	  while (pixelBR < end) \
	  { \
		size * rowEnd = pixelBR + offsetX; \
		while (pixelBR < rowEnd) \
		{ \
		  *pixelTL++ = 0.0f; \
		  *pixelTR++ = 0.0f; \
		  *pixelBL++ = 0.0f; \
		  *pixelBR++ = 0.0f; \
		} \
		pixelTL = pixelTR; \
		pixelBL = pixelBR; \
		pixelTR += advance; \
		pixelBR += advance; \
	  } \
	} \
  }

  if (*format == GrayFloat) subtract (float)
  else subtract (double)

  return result;
}

Image
Image::operator * (double factor)
{
  Image result (width, height, *format);

  if (*format == GrayFloat)
  {
	float * fromPixel = (float *) buffer;
	float * toPixel   = (float *) result.buffer;
	float * end       = fromPixel + width * height;
	while (fromPixel < end)
	{
	  *toPixel++ = *fromPixel++ * factor;
	}
  }
  else if (*format == GrayDouble)
  {
	double * fromPixel = (double *) buffer;
	double * toPixel   = (double *) result.buffer;
	double * end       = fromPixel + width * height;
	while (fromPixel < end)
	{
	  *toPixel++ = *fromPixel++ * factor;
	}
  }
  else if (*format == GrayChar)
  {
	int ifactor = (int) (factor * (1 << 16));
	unsigned char * fromPixel = (unsigned char *) buffer;
	unsigned char * toPixel   = (unsigned char *) result.buffer;
	unsigned char * end       = fromPixel + width * height;
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
  if (*format == GrayFloat)
  {
	float * pixel = (float *) buffer;
	float * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ *= factor;
	}
  }
  else if (*format == GrayDouble)
  {
	double * pixel = (double *) buffer;
	double * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ *= factor;
	}
  }
  else if (*format == GrayChar)
  {
	// This does not explicitly handle negative factors.
	int ifactor = (int) (factor * (1 << 16));
	unsigned char * pixel = (unsigned char *) buffer;
	unsigned char * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ = (*pixel * ifactor) >> 16;
	}
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
  if (*format == GrayFloat)
  {
	float * pixel = (float *) buffer;
	float * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ += value;
	}
  }
  else if (*format == GrayDouble)
  {
	double * pixel = (double *) buffer;
	double * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ += value;
	}
  }
  else if (*format == GrayChar)
  {
	// This does not explicitly handle negative values.
	int ivalue = (int) (value * (1 << 16));
	unsigned char * pixel = (unsigned char *) buffer;
	unsigned char * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ = (*pixel + ivalue) >> 16;
	}
  }
  else
  {
	throw "Image::operator += : unimplemented format";
  }

  return *this;
}

unsigned int
Image::getRGBA (const int x, const int y) const
{
  if (x < 0  ||  x >= width  ||  y < 0  ||  y >= height)
  {
	// We could throw an exception, which may help expose errors of logic in the caller.
	// However, by being tolerant, we enable more flexible programming.  The caller can
	// view the image as existing but being zero outside its bounds.
	return 0;
  }

  return format->getRGBA (((char *) buffer) + (y * width + x) * format->depth);
}

void
Image::setRGBA (const int x, const int y, const unsigned int rgba)
{
  if (x < 0  ||  x >= width  ||  y < 0  ||  y >= height)
  {
	// Silently ignore out of bounds pixels.
	return;
  }

  return format->setRGBA (((char *) buffer) + (y * width + x) * format->depth, rgba);
}
