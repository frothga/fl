/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.6 and 1.8    Copyright 2005 Sandia Corporation.
Revisions 1.10 thru 1.21 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.21  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.20  2006/11/12 15:11:56  Fred
Allocate buffer if needed in bitblt().

Revision 1.19  2006/03/20 05:34:51  Fred
Push work of constructing buffer into resize().

Revision 1.18  2006/03/05 23:58:49  Fred
Guard against negative image size.

Revision 1.17  2006/03/02 03:27:14  Fred
Create new class ImageFileDelegate to do the actual work of the image codec. 
Make ImageFile a tool for accessing image files, including metadata and image
contents.  ImageFileDelegate is now like a strategy object which implements the
specifics, while ImageFile presents a uniform inteface to the programmer.

Use ImageFile to service to read() and write() methods.

Revision 1.16  2006/02/27 03:32:44  Fred
Get rid of ImageFileFormat::open() for files, and change interface of open()
for streams to optionally specify that the ImageFile should take responsibility
for destroying the stream.  Update Image::read() and write() file methods to
set the flag appropriately.  Guard against zero probability codec selection.

Revision 1.15  2006/02/26 14:03:47  Fred
Add omitted return value.

Revision 1.14  2006/02/26 03:09:12  Fred
Create a new class called ImageFile which does the actual work of reading or
writing Images, and separate it from ImageFileFormat.  The job of
ImageFileFormat is now just to reify the format and act as a factory to
ImageFiles.  The purpose of this change is to move toward supporting big
images, which require a file to be open over the lifespan of the Image.

Revision 1.13  2006/02/26 00:14:10  Fred
Switch to probabilistic selection of image file format.

Revision 1.12  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.11  2006/01/15 05:35:12  Fred
Rewrite operator + to use pointers, similar to operator -.

Revision 1.10  2006/01/15 03:23:43  Fred
Rewrite operator - to use pointers, and factor out if tests.  Code is much more
complicated, but should be more efficient.

Revision 1.9  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.8  2005/10/09 04:11:48  Fred
Add detail to revision history.

Revision 1.7  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.6  2005/01/22 21:19:58  Fred
MSVC compilability fix:  Use memset() rather than bzero(), since it is not
always available.  Probably should look for more efficient function to clear
memory.

Revision 1.5  2004/01/08 21:23:58  rothgang
Add method to multiply image by scalar that returs a new image rather than
modifying the existing one.

Revision 1.4  2003/09/07 22:06:26  rothgang
Fixed error messages for operators.

Revision 1.3  2003/08/06 22:19:56  rothgang
Prevent width and height from going negative.

Revision 1.2  2003/07/13 19:23:58  rothgang
Update resize() to handle 3-byte formats.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
  timestamp    = getTimestamp ();
  this->width  = max (0, width);
  this->height = max (0, height);
  buffer       = new PixelBufferPacked (block, this->width, this->height, format.depth);
  this->format = &format;
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

Image &
Image::operator <<= (const Image & that)
{
  buffer    = that.buffer;
  format    = that.format;
  width     = that.width;
  height    = that.height;
  timestamp = that.timestamp;

  return *this;
}

void
Image::copyFrom (const Image & that)
{
  buffer    = that.buffer->duplicate ();
  format    = that.format;
  width     = that.width;
  height    = that.height;
  timestamp = that.timestamp;
}

void
Image::copyFrom (void * block, int width, int height, const PixelFormat & format)
{
  PixelBufferPacked * p = (PixelBufferPacked *) buffer;
  if (! p)
  {
	buffer = p = new PixelBufferPacked ();
  }
  if (p->memory.memory != block)
  {
	timestamp    = getTimestamp ();  // Actually, we don't know the timestamp on a bare buffer, but this guess is as good as any.
	this->width  = max (0, width);
	this->height = max (0, height);
	p->copyFrom (block, this->width, this->height, format.depth);
	this->format = &format;
  }
}

void
Image::attach (void * block, int width, int height, const PixelFormat & format)
{
  timestamp    = getTimestamp ();
  this->width  = max (0, width);
  this->height = max (0, height);
  buffer       = new PixelBufferPacked (block, this->width, this->height, format.depth);
  this->format = &format;
}

void
Image::detach ()
{
  buffer = 0;
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

struct triad
{
  unsigned char channel[3];
};

/**
   /todo Use line transfers rather than pixel oriented loops.
   /todo Handle reading or writing non-packed buffers.
 **/
void
Image::bitblt (const Image & that, int toX, int toY, int fromX, int fromY, int width, int height)
{
  // Temporary guard against non-packed buffers.
  PixelBufferPacked * toBuffer = (PixelBufferPacked *) this->buffer;
  if (! toBuffer)
  {
	buffer = toBuffer = new PixelBufferPacked;
	this->width  = 0;
	this->height = 0;
  }

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
  Image source = that * (*(this->format));  // should also force conversion to packed format
  PixelBufferPacked * fromBuffer = (PixelBufferPacked *) source.buffer;
  if (! fromBuffer) throw "Non-packed buffers not yet implemented in bitblt.";

  // Adjust size of target Image (ie: this)
  int needWidth = toX + width;
  int needHeight = toY + height;
  if (needWidth > this->width  ||  needHeight > this->height)
  {
	this->width  = max (this->width,  needWidth);
	this->height = max (this->height, needHeight);
	resize (this->width, this->height, true);
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
            ((size *) toBuffer->memory)[y * this->width + x] = ((size *) fromBuffer->memory)[(y + offsetY) * that.width + (x + offsetX)]; \
	      } \
	    } \
      } \
      else \
      { \
        for (int x = needWidth - 1; x >= toX; x--) \
        { \
	      for (int y = toY; y < needHeight; y++) \
	      { \
            ((size *) toBuffer->memory)[y * this->width + x] = ((size *) fromBuffer->memory)[(y + offsetY) * that.width + (x + offsetX)]; \
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
            ((size *) toBuffer->memory)[y * this->width + x] = ((size *) fromBuffer->memory)[(y + offsetY) * that.width + (x + offsetX)]; \
	      } \
	    } \
	  } \
      else \
      { \
        for (int x = toX; x < needWidth; x++) \
        { \
	      for (int y = toY; y < needHeight; y++) \
	      { \
            ((size *) toBuffer->memory)[y * this->width + x] = ((size *) fromBuffer->memory)[(y + offsetY) * that.width + (x + offsetX)]; \
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

  PixelBufferPacked * buffer1 = (PixelBufferPacked *) buffer;
  PixelBufferPacked * buffer2 = (PixelBufferPacked *) that.buffer;
  if (! buffer1  ||  ! buffer2) throw "Non-packed buffers not yet implemented in operator +.";

  Image result (max (width, that.width), max (height, that.height), *format);

  int offsetX = abs (width  - that.width)  / 2;
  int offsetY = abs (height - that.height) / 2;
  int sharedWidth = min (width, that.width);
  int sharedHeight = min (height, that.height);
  assert (offsetX * 2 == abs (width - that.width)  &&  offsetY * 2 == abs (height - that.height));  // avoid odd-even mismatch

  #define add(size) \
  { \
	size * start1 = (size *) buffer1->memory; \
	size * start2 = (size *) buffer2->memory; \
	size * start  = (size *) ((PixelBufferPacked *) result.buffer)->memory; \
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

  PixelBufferPacked * buffer1 = (PixelBufferPacked *) buffer;
  PixelBufferPacked * buffer2 = (PixelBufferPacked *) that.buffer;
  if (! buffer1  ||  ! buffer2) throw "Non-packed buffers not yet implemented in operator +.";

  Image result (max (width, that.width), max (height, that.height), *format);

  int offsetX = abs (width  - that.width)  / 2;
  int offsetY = abs (height - that.height) / 2;
  int sharedWidth = min (width, that.width);
  assert (offsetX * 2 == abs (width - that.width)  &&  offsetY * 2 == abs (height - that.height));

  #define subtract(size) \
  { \
	size * start1 = (size *) buffer1->memory; \
	size * start2 = (size *) buffer2->memory; \
	size * start  = (size *) ((PixelBufferPacked *) result.buffer)->memory; \
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
  PixelBufferPacked * p = (PixelBufferPacked *) buffer;
  if (! p) throw "Can't multiply non-packed buffer yet.";

  Image result (width, height, *format);
  PixelBufferPacked * q = (PixelBufferPacked *) result.buffer;

  if (*format == GrayFloat)
  {
	float * fromPixel = (float *) p->memory;
	float * toPixel   = (float *) q->memory;
	float * end       = fromPixel + width * height;
	while (fromPixel < end)
	{
	  *toPixel++ = *fromPixel++ * factor;
	}
  }
  else if (*format == GrayDouble)
  {
	double * fromPixel = (double *) p->memory;
	double * toPixel   = (double *) q->memory;
	double * end       = fromPixel + width * height;
	while (fromPixel < end)
	{
	  *toPixel++ = *fromPixel++ * factor;
	}
  }
  else if (*format == GrayChar)
  {
	int ifactor = (int) (factor * (1 << 16));
	unsigned char * fromPixel = (unsigned char *) p->memory;
	unsigned char * toPixel   = (unsigned char *) q->memory;
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
  PixelBufferPacked * p = (PixelBufferPacked *) buffer;
  if (! p) throw "Can't multiply non-packed buffer yet.";

  if (*format == GrayFloat)
  {
	float * pixel = (float *) p->memory;
	float * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ *= factor;
	}
  }
  else if (*format == GrayDouble)
  {
	double * pixel = (double *) p->memory;
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
	unsigned char * pixel = (unsigned char *) p->memory;
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
  PixelBufferPacked * p = (PixelBufferPacked *) buffer;
  if (! p) throw "Can't multiply non-packed buffer yet.";

  if (*format == GrayFloat)
  {
	float * pixel = (float *) p->memory;
	float * end   = pixel + width * height;
	while (pixel < end)
	{
	  *pixel++ += value;
	}
  }
  else if (*format == GrayDouble)
  {
	double * pixel = (double *) p->memory;
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
	unsigned char * pixel = (unsigned char *) p->memory;
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
