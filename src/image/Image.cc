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
	  bzero ((char *) buffer + count, buffer.size () - count);
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
  if (*format != *that.format)
  {
	if (format->precedence >= that.format->precedence)
	{
	  Image temp = that * (*format);
	  return *this + temp;
	}
	else
	{
	  Image temp = (*this) * (*that.format);
	  return temp + that;
	}
  }

  Image result (max (width, that.width), max (height, that.height), *format);

  int offsetX1 = 0;
  int offsetX2 = 0;
  if (width > that.width)
  {
	offsetX2 = (width - that.width) / 2;
  }
  else
  {
	offsetX1 = (that.width - width) / 2;
  }
  int offsetY1 = 0;
  int offsetY2 = 0;
  if (height > that.height)
  {
	offsetY2 = (height - that.height) / 2;
  }
  else
  {
	offsetY1 = (that.height - height) / 2;
  }

  if (*format == GrayFloat)
  {
	ImageOf<float> image1 (*this);
	ImageOf<float> image2 (that);
	ImageOf<float> imageR (result);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		float pixel1 = 0;
		if (x - offsetX1 >= 0  &&  x - offsetX1 < width  &&  y - offsetY1 >= 0  &&  y - offsetY1 < height)
		{
		  pixel1 = image1 (x - offsetX1, y - offsetY1);
		}
		float pixel2 = 0;
		if (x - offsetX2 >= 0  &&  x - offsetX2 < that.width  &&  y - offsetY2 >= 0  &&  y - offsetY2 < that.height)
		{
		  pixel2 = image2 (x - offsetX2, y - offsetY2);
		}
		imageR (x, y) = pixel1 + pixel2;
	  }
	}
  }
  else if (*format == GrayDouble)
  {
	ImageOf<double> image1 (*this);
	ImageOf<double> image2 (that);
	ImageOf<double> imageR (result);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		double pixel1 = 0;
		if (x - offsetX1 >= 0  &&  x - offsetX1 < width  &&  y - offsetY1 >= 0  &&  y - offsetY1 < height)
		{
		  pixel1 = image1 (x - offsetX1, y - offsetY1);
		}
		double pixel2 = 0;
		if (x - offsetX2 >= 0  &&  x - offsetX2 < that.width  &&  y - offsetY2 >= 0  &&  y - offsetY2 < that.height)
		{
		  pixel2 = image2 (x - offsetX2, y - offsetY2);
		}
		imageR (x, y) = pixel1 + pixel2;
	  }
	}
  }
  else
  {
	throw "add unimplemented for general formats";
	// Use Pixel class to encapsulate format, and code similar to above
  }

  return result;
}

Image
Image::operator - (const Image & that)
{
  if (*format != *that.format)
  {
	if (format->precedence >= that.format->precedence)
	{
	  Image temp = that * (*format);
	  return *this - temp;
	}
	else
	{
	  Image temp = (*this) * (*that.format);
	  return temp - that;
	}
  }

  Image result (max (width, that.width), max (height, that.height), *format);

  int offsetX1 = 0;
  int offsetX2 = 0;
  if (width > that.width)
  {
	offsetX2 = (width - that.width) / 2;
  }
  else
  {
	offsetX1 = (that.width - width) / 2;
  }
  int offsetY1 = 0;
  int offsetY2 = 0;
  if (height > that.height)
  {
	offsetY2 = (height - that.height) / 2;
  }
  else
  {
	offsetY1 = (that.height - height) / 2;
  }

  if (*format == GrayFloat)
  {
	ImageOf<float> image1 (*this);
	ImageOf<float> image2 (that);
	ImageOf<float> imageR (result);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		float pixel1 = 0;
		if (x - offsetX1 >= 0  &&  x - offsetX1 < width  &&  y - offsetY1 >= 0  &&  y - offsetY1 < height)
		{
		  pixel1 = image1 (x - offsetX1, y - offsetY1);
		}
		float pixel2 = 0;
		if (x - offsetX2 >= 0  &&  x - offsetX2 < that.width  &&  y - offsetY2 >= 0  &&  y - offsetY2 < that.height)
		{
		  pixel2 = image2 (x - offsetX2, y - offsetY2);
		}
		imageR (x, y) = pixel1 - pixel2;
	  }
	}
  }
  else if (*format == GrayDouble)
  {
	ImageOf<double> image1 (*this);
	ImageOf<double> image2 (that);
	ImageOf<double> imageR (result);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		double pixel1 = 0;
		if (x - offsetX1 >= 0  &&  x - offsetX1 < width  &&  y - offsetY1 >= 0  &&  y - offsetY1 < height)
		{
		  pixel1 = image1 (x - offsetX1, y - offsetY1);
		}
		double pixel2 = 0;
		if (x - offsetX2 >= 0  &&  x - offsetX2 < that.width  &&  y - offsetY2 >= 0  &&  y - offsetY2 < that.height)
		{
		  pixel2 = image2 (x - offsetX2, y - offsetY2);
		}
		imageR (x, y) = pixel1 - pixel2;
	  }
	}
  }
  else
  {
	throw "subtract unimplemented for general formats";
  }

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
