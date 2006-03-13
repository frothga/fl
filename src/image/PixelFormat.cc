/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for Cygwin
01/2005 Fred Rothganger -- Compilability fix for MSVC
05/2005 Fred Rothganger -- Rework naming scheme to be consistent across
        endians.
10/2005 Fred Rothganger -- 64-bit compatibility
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


01/2006 Fred Rothganger -- Moved PixelFormat code into separate file.
02/2006 Fred Rothganger -- Change Image structure.
03/2006 Fred Rothganger -- Move endian code to endian.h

$Log$
Revision 1.29  2006/03/13 03:24:06  Fred
Add more fromGrayShort() functions.  Fix input size in RGBABits::fromGrayShort().

Suppress alpha channel in RGBABits::fromGrayChar() and fromGrayShort().

Fix sign on grayShift in RGBChar::fromGrayShort().

Experimet with RCS log to replace manually maintained revision history at head of file.

*/


#include "fl/image.h"
#include "fl/pi.h"
#include "fl/math.h"
#include "fl/endian.h"

#include <algorithm>


// Include for tracing
//#include <iostream>


using namespace std;
using namespace fl;


PixelFormatGrayChar   fl::GrayChar;
PixelFormatGrayShort  fl::GrayShort;
PixelFormatGrayFloat  fl::GrayFloat;
PixelFormatGrayDouble fl::GrayDouble;
PixelFormatRGBAChar   fl::RGBAChar;
PixelFormatRGBAShort  fl::RGBAShort;
PixelFormatRGBAFloat  fl::RGBAFloat;
PixelFormatRGBChar    fl::RGBChar;
PixelFormatRGBShort   fl::RGBShort;
PixelFormatUYVYChar   fl::UYVYChar;
PixelFormatYUYVChar   fl::YUYVChar;
PixelFormatUYVChar    fl::UYVChar;
PixelFormatHLSFloat   fl::HLSFloat;

// These "bits" formats must be endian independent.
#if BYTE_ORDER == LITTLE_ENDIAN
PixelFormatRGBABits   fl::BGRChar (3, 0xFF0000, 0xFF00, 0xFF, 0x0);
#else
PixelFormatRGBABits   fl::BGRChar (3, 0xFF, 0xFF00, 0xFF0000, 0x0);
#endif



// Color->gray conversion factors
// Make these user modfiable if at some point it turns out to be useful.
// First used: (54 183 19) / 256, same as linear sRGB below
// Linear sRGB to Y: 0.2126 0.7152 0.0722
// NTSC, PAL, and JPEG: 0.2989 0.5866 0.1145, produces a non-linear gray-value, appropriate for non-linear sRGB, which is our assumed RGB format
#define redWeight    76
#define greenWeight 150
#define blueWeight   29
#define totalWeight 255
#define redToY   0.2126
#define greenToY 0.7152
#define blueToY  0.0722


// Structure used for converting odd stride (ie: stride = 3 bytes)
union Int2Char
{
  unsigned int  all;
  struct
  {
#   if BYTE_ORDER == LITTLE_ENDIAN
	unsigned char piece0;
	unsigned char piece1;
	unsigned char piece2;
	unsigned char barf;
#   else
	unsigned char barf;
	unsigned char piece0;
	unsigned char piece1;
	unsigned char piece2;
#   endif
  };
};

#if BYTE_ORDER == LITTLE_ENDIAN
#  define endianAdjustFromPixel
#else
#  define endianAdjustFromPixel fromPixel--;
#endif


// Gamma functions ------------------------------------------------------------

// These functions convert from int (which is assumed to be non-linear) to
// floating point (assumed to be linear).  They assume that gamma = 2.2.

static inline void
delinearize (float & value)
{
  if (value <= 0.0031308f)
  {
	value *= 12.92f;
  }
  else
  {
	value = 1.055f * powf (value, 1.0f / 2.4f) - 0.055f;
  }
}

static inline void
delinearize (double & value)
{
  if (value <= 0.0031308)
  {
	value *= 12.92;
  }
  else
  {
	value = 1.055 * pow (value, 1.0 / 2.4) - 0.055;
  }
}

static inline void
linearize (float & value)
{
  if (value <= 0.04045f)
  {
	value /= 12.92f;
  }
  else
  {
	value = powf ((value + 0.055f) / 1.055f, 2.4f);
  }
}

static inline void
linearize (double & value)
{
  if (value <= 0.04045)
  {
	value /= 12.92;
  }
  else
  {
	value = pow ((value + 0.055) / 1.055, 2.4);
  }
}


// class PixelFormat ----------------------------------------------------------

Image
PixelFormat::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  fromAny (image, result);

  return result;
}

/**
   Uses getRGBA() and setRGBA().  XYZ would be more accurate, but this
   is also adequate, since RGB values are well defined (as non-linear sRGB).
**/
void
PixelFormat::fromAny (const Image & image, Image & result) const
{
  unsigned char * dest = (unsigned char *) ((PixelBufferPacked *) result.buffer)->memory;
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	unsigned char * end = dest + image.width * image.height * depth;
	while (dest < end)
	{
	  setRGBA (dest, sourceFormat->getRGBA (source));
	  source += sourceDepth;
	  dest += depth;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		setRGBA (dest, sourceFormat->getRGBA (image.buffer->pixel (x, y)));
	  }
	}
  }
}

bool
PixelFormat::operator == (const PixelFormat & that) const
{
  return typeid (*this) == typeid (that);
}

void
PixelFormat::getRGBA (void * pixel, float values[]) const
{
  unsigned int rgba = getRGBA (pixel);
  values[0] = ((rgba & 0xFF000000) >> 24) / 255.0f;
  values[1] = ((rgba &   0xFF0000) >> 16) / 255.0f;
  values[2] = ((rgba &     0xFF00) >>  8) / 255.0f;
  values[3] = ((rgba &       0xFF)      ) / 255.0f;
  linearize (values[0]);
  linearize (values[1]);
  linearize (values[2]);
  // Don't linearize alpha, because already linear
}

void
PixelFormat::getXYZ (void * pixel, float values[]) const
{
  float rgbValues[4];
  getRGBA (pixel, rgbValues);

  // Matrix multiply to cast into XYZ space
  values[0] = 0.4124f * rgbValues[0] + 0.3576f * rgbValues[1] + 0.1805f * rgbValues[2];
  values[1] = 0.2126f * rgbValues[0] + 0.7152f * rgbValues[1] + 0.0722f * rgbValues[2];
  values[2] = 0.0193f * rgbValues[0] + 0.1192f * rgbValues[1] + 0.9505f * rgbValues[2];
}

/**
   See PixelFormatUYVYChar::setRGBA() for more details on the conversion matrix.
 **/
unsigned int
PixelFormat::getYUV  (void * pixel) const
{
  unsigned int rgba = getRGBA (pixel);

  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  unsigned int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b,            0), 0xFFFFFF) & 0xFF0000;
  unsigned int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000, 0), 0xFFFFFF) & 0xFF0000;
  unsigned int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000, 0), 0xFFFFFF) & 0xFF0000;

  return y | (u >> 8) | (v >> 16);
}

unsigned char
PixelFormat::getGray (void * pixel) const
{
  unsigned int rgba = getRGBA (pixel);
  unsigned int r = (rgba & 0xFF000000) >> 16;
  unsigned int g = (rgba &   0xFF0000) >>  8;
  unsigned int b = (rgba &     0xFF00);
  return ((redWeight * r + greenWeight * g + blueWeight * b) / totalWeight) >> 8;
}

void
PixelFormat::getGray (void * pixel, float & gray) const
{
  gray = getGray (pixel) / 255.0f;
  linearize (gray);
}

unsigned char
PixelFormat::getAlpha (void * pixel) const
{
  // Presumably, the derived PixelFormat does not have an alpha channel,
  // so pretend that it is set to full opacity.
  return 0xFF;
}

void
PixelFormat::setRGBA (void * pixel, float values[]) const
{
  unsigned int rgba = (unsigned int) (values[3] * 255);
  int shift = 32;
  for (int i = 0; i < 3; i++)
  {
	float v = values[i];
	v = min (v, 1.0f);
	v = max (v, 0.0f);
	delinearize (v);
	rgba |= ((unsigned int) (v * 255)) << (shift -= 8);
  }
  setRGBA (pixel, rgba);
}

void
PixelFormat::setXYZ (void * pixel, float values[]) const
{
  // Don't clamp XYZ values

  // Do matrix multiply to get linear RGB values
  float rgbValues[4];
  rgbValues[0] =  3.2406f * values[0] - 1.5372f * values[1] - 0.4986f * values[2];
  rgbValues[1] = -0.9689f * values[0] + 1.8758f * values[1] + 0.0415f * values[2];
  rgbValues[2] =  0.0557f * values[0] - 0.2040f * values[1] + 1.0570f * values[2];
  rgbValues[3] = 1.0f;

  setRGBA (pixel, rgbValues);
}

void
PixelFormat::setYUV (void * pixel, unsigned int yuv) const
{
  // It is possible to pass a value where Y = 0 but U and V are not zero.
  // Technically, this is an illegal value.  However, this code doesn't
  // trap that case, so it can generate bogus RGB values when the pixel
  // should be black.

  int y =   yuv & 0xFF0000;
  int u = ((yuv &   0xFF00) >> 8) - 128;
  int v =  (yuv &     0xFF)       - 128;

  // See PixelFormatUYVY::getRGB() for an explanation of this arithmetic.
  unsigned int r = min (max (y               + 0x166F7 * v, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u,               0), 0xFFFFFF);

  setRGBA (pixel, ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF);
}

void
PixelFormat::setGray (void * pixel, unsigned char gray) const
{
  register unsigned int iv = gray;
  setRGBA (pixel, (iv << 24) | (iv << 16) | (iv << 8) | 0xFF);
}

void
PixelFormat::setGray (void * pixel, float gray) const
{
  gray = min (gray, 1.0f);
  gray = max (gray, 0.0f);
  delinearize (gray);
  unsigned int iv = (unsigned int) (gray * 255);
  unsigned int rgba = (iv << 24) | (iv << 16) | (iv << 8) | 0xFF;
  setRGBA (pixel, rgba);
}

void
PixelFormat::setAlpha (void * pixel, unsigned char alpha) const
{
  // Presumably, the derived PixelFormat does not have an alpha channel,
  // so just ignore this request.
}


// class PixelFormatGrayChar --------------------------------------------------

PixelFormatGrayChar::PixelFormatGrayChar ()
{
  planes     = 1;
  depth      = 1;
  precedence = 0;  // Below everything
  monochrome = true;
  hasAlpha   = false;
}

Image
PixelFormatGrayChar::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayShort))
  {
	fromGrayShort (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayFloat))
  {
	fromGrayFloat (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayDouble))
  {
	fromGrayDouble (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBAChar))
  {
	fromRGBAChar (image, result);
  }
  else if (dynamic_cast<const PixelFormatRGBABits *> (image.format))
  {
	fromRGBABits (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatGrayChar::fromGrayShort (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned short * fromPixel = (unsigned short *) i->memory;
  unsigned char *  toPixel   = (unsigned char *)  o->memory;
  unsigned char *  end       = toPixel + result.width * result.height;
  int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift - 8;
  if (grayShift > 0)
  {
	while (toPixel < end)
	{
	  *toPixel++ = (unsigned char) (*fromPixel++ << grayShift);
	}
  }
  else if (grayShift < 0)
  {
	while (toPixel < end)
	{
	  *toPixel++ = (unsigned char) (*fromPixel++ >> -grayShift);
	}
  }
  else  // grayShift == 0
  {
	while (toPixel < end)
	{
	  *toPixel++ = (unsigned char) *fromPixel++;
	}
  }
}

void
PixelFormatGrayChar::fromGrayFloat (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  float *         fromPixel = (float *)         i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float p = min (max (*fromPixel++, 0.0f), 1.0f);
	delinearize (p);
	*toPixel++ = (unsigned char) (p * 255);
  }
}

void
PixelFormatGrayChar::fromGrayDouble (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  double *        fromPixel = (double *)        i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double p = min (max (*fromPixel++, 0.0), 1.0);
	delinearize (p);
	*toPixel++ = (unsigned char) (p * 255);
  }
}

void
PixelFormatGrayChar::fromRGBAChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	unsigned int t;
	t  = fromPixel[0] * (redWeight   << 8);
	t += fromPixel[1] * (greenWeight << 8);
	t += fromPixel[2] * (blueWeight  << 8);
	fromPixel += 4;
	*toPixel++ = t / (totalWeight << 8);
  }
}

void
PixelFormatGrayChar::fromRGBABits (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, redShift, greenShift, blueShift, alphaShift);

  #define RGBBits2GrayChar(fromSize) \
  { \
    unsigned fromSize * fromPixel = (unsigned fromSize *) i->memory; \
    unsigned char * toPixel       = (unsigned char *)     o->memory; \
    unsigned char * end           = toPixel + result.width * result.height; \
    while (toPixel < end) \
    { \
      unsigned int r = *fromPixel & that->redMask; \
      unsigned int g = *fromPixel & that->greenMask; \
	  unsigned int b = *fromPixel & that->blueMask; \
      fromPixel++; \
	  *toPixel++ = (  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight \
	                + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight \
	                + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight \
	               ) / (totalWeight << 8); \
    } \
  }

  switch (that->depth)
  {
    case 1:
	  RGBBits2GrayChar (char);
	  break;
    case 2:
	  RGBBits2GrayChar (short);
	  break;
    case 3:
	{
	  unsigned char * fromPixel = (unsigned char *) i->memory;
	  endianAdjustFromPixel
	  unsigned char * toPixel   = (unsigned char *) o->memory;
	  unsigned char * end       = toPixel + result.width * result.height;
	  while (toPixel < end)
	  {
		unsigned int t = * (unsigned int *) fromPixel;
		fromPixel += 3;
		unsigned int r = t & that->redMask;
		unsigned int g = t & that->greenMask;
		unsigned int b = t & that->blueMask;
		*toPixel++ = (  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight
					  + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight
					  + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight
					 ) >> 16;
	  }
	  break;
	}
    case 4:
    default:
	  RGBBits2GrayChar (int);
  }
}

void
PixelFormatGrayChar::fromAny (const Image & image, Image & result) const
{
  unsigned char * dest = (unsigned char *) ((PixelBufferPacked *) result.buffer)->memory;
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	unsigned char * end = dest + image.width * image.height;
	while (dest < end)
	{
	  *dest++ = sourceFormat->getGray (source);
	  source += sourceDepth;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		*dest++ = sourceFormat->getGray (image.buffer->pixel (x, y));
	  }
	}
  }
}

bool
PixelFormatGrayChar::operator == (const PixelFormat & that) const
{
  if (typeid (*this) == typeid (that))
  {
	return true;
  }
  if (const PixelFormatRGBABits * other = dynamic_cast<const PixelFormatRGBABits *> (& that))
  {
	return    other->depth     == depth
	       && other->redMask   == 0xFF
	       && other->greenMask == 0xFF
	       && other->blueMask  == 0xFF;
	// We don't care about contents of alpha mask.
  }
  return false;
}

unsigned int
PixelFormatGrayChar::getRGBA (void * pixel) const
{
  unsigned int t = *((unsigned char *) pixel);
  return (t << 24) | (t << 16) | (t << 8) | 0xFF;
}

void
PixelFormatGrayChar::getXYZ (void * pixel, float values[]) const
{
  values[0] = 0;
  values[1] = *((unsigned char *) pixel) / 255.0f;
  values[2] = 0;

  linearize (values[1]);
}

unsigned char
PixelFormatGrayChar::getGray (void * pixel) const
{
  return *((unsigned char *) pixel);
}

void
PixelFormatGrayChar::getGray (void * pixel, float & gray) const
{
  gray = *((unsigned char *) pixel) / 255.0f;
  linearize (gray);
}

void
PixelFormatGrayChar::setRGBA (void * pixel, unsigned int rgba) const
{
  unsigned int r = (rgba & 0xFF000000) >> 16;
  unsigned int g = (rgba &   0xFF0000) >>  8;
  unsigned int b =  rgba &     0xFF00;

  *((unsigned char *) pixel) = (r * redWeight + g * greenWeight + b * blueWeight) / (totalWeight << 8);
}

void
PixelFormatGrayChar::setXYZ (void * pixel, float values[]) const
{
  // Convert Y value to non-linear form
  float v = min (max (values[1], 0.0f), 1.0f);
  delinearize (v);
  *((unsigned char *) pixel) = (unsigned int) rint (255 * v);
}

void
PixelFormatGrayChar::setGray (void * pixel, unsigned char gray) const
{
  *((unsigned char *) pixel) = gray;
}

void
PixelFormatGrayChar::setGray (void * pixel, float gray) const
{
  gray = min (max (gray, 0.0f), 1.0f);
  delinearize (gray);
  *((unsigned char *) pixel) = (unsigned int) rint (255 * gray);
}


// class PixelFormatGrayShort -------------------------------------------------

PixelFormatGrayShort::PixelFormatGrayShort (unsigned short grayMask)
{
  planes     = 1;
  depth      = 2;
  precedence = 2;  // Above GrayChar and UYVY, but below everything else
  monochrome = true;
  hasAlpha   = false;

  this->grayMask = grayMask;
  grayShift = 0;
  while (grayMask >>= 1) {grayShift++;}
  grayShift = 15 - grayShift;
}

Image
PixelFormatGrayShort::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (*image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
  }
  else if (typeid (*image.format) == typeid (PixelFormatGrayFloat))
  {
	fromGrayFloat (image, result);
  }
  else if (typeid (*image.format) == typeid (PixelFormatGrayDouble))
  {
	fromGrayDouble (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatGrayShort::fromGrayChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char *  fromPixel = (unsigned char *)  i->memory;
  unsigned short * toPixel   = (unsigned short *) o->memory;
  unsigned short * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	*toPixel++ = (((*fromPixel << 8) + *fromPixel) >> grayShift) & grayMask;
	fromPixel++;
  }
}

void
PixelFormatGrayShort::fromGrayFloat (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  float *          fromPixel = (float *)          i->memory;
  unsigned short * toPixel   = (unsigned short *) o->memory;
  unsigned short * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float p = min (max (*fromPixel++, 0.0f), 1.0f);
	delinearize (p);
	*toPixel++ = (unsigned short) (p * grayMask);
  }
}

void
PixelFormatGrayShort::fromGrayDouble (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  double *         fromPixel = (double *)         i->memory;
  unsigned short * toPixel   = (unsigned short *) o->memory;
  unsigned short * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double p = min (max (*fromPixel++, 0.0), 1.0);
	delinearize (p);
	*toPixel++ = (unsigned short) (p * grayMask);
  }
}

void
PixelFormatGrayShort::fromAny (const Image & image, Image & result) const
{
  unsigned short * dest = (unsigned short *) ((PixelBufferPacked *) result.buffer)->memory;
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	unsigned short * end = dest + image.width * image.height;
	while (dest < end)
	{
	  unsigned short p = sourceFormat->getGray (source);
	  *dest++ = (((p << 8) + p) >> grayShift) & grayMask;
	  source += sourceDepth;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		unsigned short p = sourceFormat->getGray (image.buffer->pixel (x, y));
		*dest++ = (((p << 8) + p) >> grayShift) & grayMask;
	  }
	}
  }
}

bool
PixelFormatGrayShort::operator == (const PixelFormat & that) const
{
  if (typeid (*this) == typeid (that))
  {
	return true;
  }
  if (const PixelFormatRGBABits * other = dynamic_cast<const PixelFormatRGBABits *> (& that))
  {
	return    other->depth     == depth
	       && other->redMask   == grayMask
	       && other->greenMask == grayMask
	       && other->blueMask  == grayMask;
  }
  return false;
}

unsigned int
PixelFormatGrayShort::getRGBA (void * pixel) const
{
  unsigned int t = (*((unsigned short *) pixel) << grayShift) & 0xFF00;
  return (t << 16) | (t << 8) | t | 0xFF;
}

void
PixelFormatGrayShort::getXYZ (void * pixel, float values[]) const
{
  values[0] = 0;
  values[1] = *((unsigned short *) pixel) / (float) grayMask;
  values[2] = 0;

  linearize (values[1]);
}

unsigned char
PixelFormatGrayShort::getGray (void * pixel) const
{
  if (grayShift == 8) return *((unsigned short *) pixel);
  if (grayShift > 8)  return *((unsigned short *) pixel) << (grayShift - 8);
  return *((unsigned short *) pixel) >> (8 - grayShift);
}

void
PixelFormatGrayShort::getGray (void * pixel, float & gray) const
{
  gray = *((unsigned short *) pixel) / (float) grayMask;
  linearize (gray);
}

void
PixelFormatGrayShort::setRGBA (void * pixel, unsigned int rgba) const
{
  unsigned int r = (rgba & 0xFF000000) >> 16;
  unsigned int g = (rgba &   0xFF0000) >> 8;
  unsigned int b =  rgba &     0xFF00;

  unsigned short t = (r * redWeight + g * greenWeight + b * blueWeight) / totalWeight;
  *((unsigned short *) pixel) = (t >> grayShift) & grayMask;
}

void
PixelFormatGrayShort::setXYZ (void * pixel, float values[]) const
{
  // Convert Y value to non-linear form
  float v = min (max (values[1], 0.0f), 1.0f);
  delinearize (v);
  *((unsigned short *) pixel) = (unsigned short) rint (grayMask * v);
}

void
PixelFormatGrayShort::setGray (void * pixel, unsigned char gray) const
{
  *((unsigned short *) pixel) = ((gray << 8 + gray) >> grayShift) & grayMask;
}

void
PixelFormatGrayShort::setGray (void * pixel, float gray) const
{
  gray = min (max (gray, 0.0f), 1.0f);
  delinearize (gray);
  *((unsigned short *) pixel) = (unsigned short) rint (grayMask * gray);
}


// class PixelFormatGrayFloat -------------------------------------------------

PixelFormatGrayFloat::PixelFormatGrayFloat ()
{
  planes      = 1;
  depth       = 4;
  precedence  = 4;  // Above all integer formats and below GrayDouble
  monochrome  = true;
  hasAlpha    = false;
}

Image
PixelFormatGrayFloat::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayShort))
  {
	fromGrayShort (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayDouble))
  {
	fromGrayDouble (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBAChar))
  {
	fromRGBAChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBChar))
  {
	fromRGBChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBABits))
  {
	fromRGBABits (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatGrayFloat::fromGrayChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  float *         toPixel   = (float *)         o->memory;
  float *         end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float v = *fromPixel++ / 255.0f;
	linearize (v);
	*toPixel++ = v;
  }
}

void
PixelFormatGrayFloat::fromGrayShort (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned short * fromPixel = (unsigned short *) i->memory;
  float *          toPixel   = (float *)          o->memory;
  float *          end       = toPixel + result.width * result.height;
  float grayMask = ((PixelFormatGrayShort *) image.format)->grayMask;
  while (toPixel < end)
  {
	float v = *fromPixel++ / grayMask;
	linearize (v);
	*toPixel++ = v;
  }
}

void
PixelFormatGrayFloat::fromGrayDouble (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  double * fromPixel = (double *) i->memory;
  float *  toPixel   = (float *)  o->memory;
  float *  end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	*toPixel++ = (float) *fromPixel++;
  }
}

void
PixelFormatGrayFloat::fromRGBAChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  float *         toPixel   = (float *)         o->memory;
  float *         end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float r = fromPixel[0] / 255.0f;
	float g = fromPixel[1] / 255.0f;
	float b = fromPixel[2] / 255.0f;
    fromPixel += 4;
	linearize (r);
	linearize (g);
	linearize (b);
	*toPixel++ = redToY * r + greenToY * g + blueToY * b;
  }
}

void
PixelFormatGrayFloat::fromRGBChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  float *         toPixel   = (float *)         o->memory;
  float *         end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float r = fromPixel[0] / 255.0f;
	float g = fromPixel[1] / 255.0f;
	float b = fromPixel[2] / 255.0f;
	fromPixel += 3;
	linearize (r);
	linearize (g);
	linearize (b);
	*toPixel++ = redToY * r + greenToY * g + blueToY * b;
  }
}

void
PixelFormatGrayFloat::fromRGBABits (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (0xFF, 0xFF, 0xFF, 0xFF, redShift, greenShift, blueShift, alphaShift);

  #define RGBBits2GrayFloat(imageSize) \
  { \
    unsigned imageSize * fromPixel = (unsigned imageSize *) i->memory; \
    float *              toPixel   = (float *)              o->memory; \
    float *              end       = toPixel + result.width * result.height; \
	while (toPixel < end) \
    { \
	  unsigned int r = *fromPixel & that->redMask; \
	  unsigned int g = *fromPixel & that->greenMask; \
	  unsigned int b = *fromPixel & that->blueMask; \
      fromPixel++; \
	  float fr = (redShift   > 0 ? r << redShift   : r >> -redShift)   / 255.0f; \
	  float fg = (greenShift > 0 ? g << greenShift : g >> -greenShift) / 255.0f; \
	  float fb = (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  / 255.0f; \
	  linearize (fr); \
	  linearize (fg); \
	  linearize (fb); \
	  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb; \
	} \
  }

  switch (that->depth)
  {
    case 1:
	  RGBBits2GrayFloat (char);
	  break;
    case 2:
	  RGBBits2GrayFloat (short);
	  break;
    case 3:
	{
	  unsigned char * fromPixel = (unsigned char *) i->memory;
	  endianAdjustFromPixel
	  float *         toPixel   = (float *)         o->memory;
	  float *         end       = toPixel + result.width * result.height;
	  while (toPixel < end)
	  {
		unsigned int t = * (unsigned int *) fromPixel;
		fromPixel += 3;
		unsigned int r = t & that->redMask;
		unsigned int g = t & that->greenMask;
		unsigned int b = t & that->blueMask;
		float fr = (redShift   > 0 ? r << redShift   : r >> -redShift)   / 255.0f;
		float fg = (greenShift > 0 ? g << greenShift : g >> -greenShift) / 255.0f;
		float fb = (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  / 255.0f;
		linearize (fr);
		linearize (fg);
		linearize (fb);
		*toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
	  }
	  break;
	}
    case 4:
    default:
	  RGBBits2GrayFloat (int);
  }
}

void
PixelFormatGrayFloat::fromAny (const Image & image, Image & result) const
{
  float * dest = (float *) ((PixelBufferPacked *) result.buffer)->memory;
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	float * end = dest + image.width * image.height;
	while (dest < end)
	{
	  sourceFormat->getGray (source, *dest++);
	  source += sourceDepth;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		sourceFormat->getGray (image.buffer->pixel (x, y), *dest++);
	  }
	}
  }
}

unsigned int
PixelFormatGrayFloat::getRGBA (void * pixel) const
{
  float v = min (max (*((float *) pixel), 0.0f), 1.0f);
  delinearize (v);
  unsigned int t = (unsigned int) (v * 255);
  return (t << 24) | (t << 16) | (t << 8) | 0xFF;
}

void
PixelFormatGrayFloat::getRGBA (void * pixel, float values[]) const
{
  float i = *((float *) pixel);
  values[0] = i;
  values[1] = i;
  values[2] = i;
  values[3] = 1.0f;
}

void
PixelFormatGrayFloat::getXYZ (void * pixel, float values[]) const
{
  values[0] = 0;
  values[1] = *((float *) pixel);
  values[2] = 0;
}

unsigned char
PixelFormatGrayFloat::getGray (void * pixel) const
{
  float v = min (max (*((float *) pixel), 0.0f), 1.0f);
  delinearize (v);
  return (unsigned char) (v * 255);
}

void
PixelFormatGrayFloat::getGray (void * pixel, float & gray) const
{
  gray = *((float *) pixel);
}

void
PixelFormatGrayFloat::setRGBA (void * pixel, unsigned int rgba) const
{
  float r = ((rgba & 0xFF000000) >> 24) / 255.0f;
  float g = ((rgba &   0xFF0000) >> 16) / 255.0f;
  float b = ((rgba &     0xFF00) >>  8) / 255.0f;
  linearize (r);
  linearize (g);
  linearize (b);
  *((float *) pixel) = redToY * r + greenToY * g + blueToY * b;
}

void
PixelFormatGrayFloat::setRGBA (void * pixel, float values[]) const
{
  *((float *) pixel) = redToY * values[0] + greenToY * values[1] + blueToY * values[2];
}

void
PixelFormatGrayFloat::setXYZ (void * pixel, float values[]) const
{
  *((float *) pixel) = values[1];
}

void
PixelFormatGrayFloat::setGray  (void * pixel, unsigned char gray) const
{
  register float g = gray / 255.0f;
  linearize (g);
  *((float *) pixel) = g;
}

void
PixelFormatGrayFloat::setGray  (void * pixel, float gray) const
{
  *((float *) pixel) = gray;
}


// class PixelFormatGrayDouble ------------------------------------------------

PixelFormatGrayDouble::PixelFormatGrayDouble ()
{
  planes      = 1;
  depth       = 8;
  precedence  = 6;  // above all integer formats and above GrayFloat
  monochrome  = true;
  hasAlpha    = false;
}

Image
PixelFormatGrayDouble::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayShort))
  {
	fromGrayShort (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayFloat))
  {
	fromGrayFloat (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBAChar))
  {
	fromRGBAChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBChar))
  {
	fromRGBChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBABits))
  {
	fromRGBABits (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatGrayDouble::fromGrayChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  double *        toPixel   = (double *)        o->memory;
  double *        end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double v = *fromPixel++ / 255.0;
	linearize (v);
	*toPixel++ = v;
  }
}

void
PixelFormatGrayDouble::fromGrayShort (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned short * fromPixel = (unsigned short *) i->memory;
  double *         toPixel   = (double *)         o->memory;
  double *         end       = toPixel + result.width * result.height;
  double grayMask = ((PixelFormatGrayShort *) image.format)->grayMask;
  while (toPixel < end)
  {
	double v = *fromPixel++ / grayMask;
	linearize (v);
	*toPixel++ = v;
  }
}

void
PixelFormatGrayDouble::fromGrayFloat (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  float *  fromPixel = (float *)  i->memory;
  double * toPixel   = (double *) o->memory;
  double * end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	*toPixel++ = *fromPixel++;
  }
}

void
PixelFormatGrayDouble::fromRGBAChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  double *        toPixel   = (double *)        o->memory;
  double *        end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double r = fromPixel[0] / 255.0;
	double g = fromPixel[1] / 255.0;
	double b = fromPixel[2] / 255.0;
    fromPixel += 4;
	linearize (r);
	linearize (g);
	linearize (b);
	*toPixel++ = redToY * r + greenToY * g + blueToY * b;
  }
}

void
PixelFormatGrayDouble::fromRGBChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  double *        toPixel   = (double *)        o->memory;
  double *        end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double r = fromPixel[0] / 255.0;
	double g = fromPixel[1] / 255.0;
	double b = fromPixel[2] / 255.0;
	fromPixel += 3;
	linearize (r);
	linearize (g);
	linearize (b);
	*toPixel++ = redToY * r + greenToY * g + blueToY * b;
  }
}

void
PixelFormatGrayDouble::fromRGBABits (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (0xFF, 0xFF, 0xFF, 0xFF, redShift, greenShift, blueShift, alphaShift);

  #define RGBBits2GrayDouble(imageSize) \
  { \
    unsigned imageSize * fromPixel = (unsigned imageSize *) i->memory; \
    double *             toPixel   = (double *)             o->memory; \
    double *             end       = toPixel + result.width * result.height; \
	while (toPixel < end) \
    { \
	  unsigned int r = *fromPixel & that->redMask; \
	  unsigned int g = *fromPixel & that->greenMask; \
	  unsigned int b = *fromPixel & that->blueMask; \
      fromPixel++; \
	  double fr = (redShift   > 0 ? r << redShift   : r >> -redShift)   / 255.0; \
	  double fg = (greenShift > 0 ? g << greenShift : g >> -greenShift) / 255.0; \
	  double fb = (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  / 255.0; \
	  linearize (fr); \
	  linearize (fg); \
	  linearize (fb); \
	  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb; \
	} \
  }

  switch (that->depth)
  {
    case 1:
	  RGBBits2GrayDouble (char);
	  break;
    case 2:
	  RGBBits2GrayDouble (short);
	  break;
    case 3:
	{
	  unsigned char * fromPixel = (unsigned char *) i->memory;
	  endianAdjustFromPixel
	  double *        toPixel   = (double *)        o->memory;
	  double *        end       = toPixel + result.width * result.height;
	  while (toPixel < end)
	  {
		unsigned int t = * (unsigned int *) fromPixel;
		fromPixel += 3;
		unsigned int r = t & that->redMask;
		unsigned int g = t & that->greenMask;
		unsigned int b = t & that->blueMask;
		double fr = (redShift   > 0 ? r << redShift   : r >> -redShift)   / 255.0;
		double fg = (greenShift > 0 ? g << greenShift : g >> -greenShift) / 255.0;
		double fb = (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  / 255.0;
		linearize (fr);
		linearize (fg);
		linearize (fb);
		*toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
	  }
	  break;
	}
    case 4:
    default:
	  RGBBits2GrayDouble (int);
  }
}

void
PixelFormatGrayDouble::fromAny (const Image & image, Image & result) const
{
  double * dest = (double *) ((PixelBufferPacked *) result.buffer)->memory;
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	double * end = dest + image.width * image.height;
	while (dest < end)
	{
	  float value;
	  sourceFormat->getGray (source, value);
	  *dest++ = value;
	  source += sourceDepth;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		float value;
		sourceFormat->getGray (image.buffer->pixel (x, y), value);
		*dest++ = value;
	  }
	}
  }
}

unsigned int
PixelFormatGrayDouble::getRGBA (void * pixel) const
{
  double v = min (max (*((double *) pixel), 0.0), 1.0);
  delinearize (v);
  unsigned int t = (unsigned int) (v * 255);
  return (t << 24) | (t << 16) | (t << 8) | 0xFF;
}

void
PixelFormatGrayDouble::getRGBA (void * pixel, float values[]) const
{
  float i = *((double *) pixel);
  values[0] = i;
  values[1] = i;
  values[2] = i;
  values[3] = 1.0f;
}

void
PixelFormatGrayDouble::getXYZ (void * pixel, float values[]) const
{
  values[0] = 0;
  values[1] = *((double *) pixel);
  values[2] = 0;
}

unsigned char
PixelFormatGrayDouble::getGray (void * pixel) const
{
  double v = min (max (*((double *) pixel), 0.0), 1.0);
  delinearize (v);
  return (unsigned char) (v * 255);
}

void
PixelFormatGrayDouble::getGray (void * pixel, float & gray) const
{
  gray = *((double *) pixel);
}

void
PixelFormatGrayDouble::setRGBA (void * pixel, unsigned int rgba) const
{
  double r = ((rgba & 0xFF000000) >> 24) / 255.0;
  double g = ((rgba &   0xFF0000) >> 16) / 255.0;
  double b = ((rgba &     0xFF00) >>  8) / 255.0;
  linearize (r);
  linearize (g);
  linearize (b);
  *((double *) pixel) = redToY * r + greenToY * g + blueToY * b;
}

void
PixelFormatGrayDouble::setRGBA (void * pixel, float values[]) const
{
  *((double *) pixel) = redToY * values[0] + greenToY * values[1] + blueToY * values[2];
}

void
PixelFormatGrayDouble::setXYZ (void * pixel, float values[]) const
{
  *((double *) pixel) = values[1];
}

void
PixelFormatGrayDouble::setGray  (void * pixel, unsigned char gray) const
{
  register double g = gray / 255.0;
  linearize (g);
  *((double *) pixel) = g;
}

void
PixelFormatGrayDouble::setGray  (void * pixel, float gray) const
{
  *((double *) pixel) = gray;
}


// class PixelFormatRGBABits --------------------------------------------------

PixelFormatRGBABits::PixelFormatRGBABits (int depth, unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask)
{
  this->depth     = depth;
  this->redMask   = redMask;
  this->greenMask = greenMask;
  this->blueMask  = blueMask;
  this->alphaMask = alphaMask;

  planes     = 1;
  precedence = 3;  // Above GrayChar and below all floating point formats
  monochrome = redMask == greenMask  &&  greenMask == blueMask;
  hasAlpha   = alphaMask;
}

Image
PixelFormatRGBABits::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayShort))
  {
	fromGrayShort (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayFloat))
  {
	fromGrayFloat (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayDouble))
  {
	fromGrayDouble (image, result);
  }
  else if (dynamic_cast<const PixelFormatRGBABits *> (image.format))
  {
	fromRGBABits (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

#define Bits2Bits(fromSize,toSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned fromSize * fromPixel = (unsigned fromSize *) i->memory; \
  unsigned toSize *   toPixel   = (unsigned toSize *)   o->memory; \
  unsigned toSize *   end       = toPixel + result.width * result.height; \
  while (toPixel < end) \
  { \
    unsigned int r = *fromPixel & fromRed; \
	unsigned int g = *fromPixel & fromGreen; \
	unsigned int b = *fromPixel & fromBlue; \
	unsigned int a = *fromPixel & fromAlpha; \
    fromPixel++; \
	*toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		         | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		         | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		         | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  } \
}

#define OddBits2Bits(toSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char *   fromPixel = (unsigned char *)   i->memory; \
  endianAdjustFromPixel \
  unsigned toSize * toPixel   = (unsigned toSize *) o->memory; \
  unsigned toSize * end       = toPixel + result.width * result.height; \
  while (toPixel < end) \
  { \
    unsigned int t = * (unsigned int *) fromPixel; \
    fromPixel += 3; \
    unsigned int r = t & fromRed; \
	unsigned int g = t & fromGreen; \
	unsigned int b = t & fromBlue; \
	unsigned int a = t & fromAlpha; \
	*toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		         | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		         | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		         | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  } \
}

#define Bits2OddBits(fromSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned fromSize * fromPixel = (unsigned fromSize *) i->memory; \
  unsigned char *     toPixel   = (unsigned char *)     o->memory; \
  unsigned char *     end       = toPixel + result.width * result.height * 3; \
  while (toPixel < end) \
  { \
    unsigned int r = *fromPixel & fromRed; \
	unsigned int g = *fromPixel & fromGreen; \
	unsigned int b = *fromPixel & fromBlue; \
	unsigned int a = *fromPixel & fromAlpha; \
    fromPixel++; \
    Int2Char t; \
	t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		    | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		    | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		    | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
    toPixel[0] = t.piece0; \
    toPixel[1] = t.piece1; \
    toPixel[2] = t.piece2; \
    toPixel += 3; \
  } \
}

#define OddBits2OddBits(fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char * fromPixel = (unsigned char *) i->memory; \
  endianAdjustFromPixel \
  unsigned char * toPixel   = (unsigned char *) o->memory; \
  unsigned char * end       = toPixel + result.width * result.height * 3; \
  while (toPixel < end) \
  { \
    Int2Char t; \
    t.all = * (unsigned int *) fromPixel; \
    fromPixel += 3; \
    unsigned int r = t.all & fromRed; \
	unsigned int g = t.all & fromGreen; \
	unsigned int b = t.all & fromBlue; \
	unsigned int a = t.all & fromAlpha; \
	t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		    | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		    | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		    | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
    toPixel[0] = t.piece0; \
    toPixel[1] = t.piece1; \
    toPixel[2] = t.piece2; \
    toPixel += 3; \
  } \
}

#define GrayFloat2Bits(fromSize,toSize) \
{ \
  fromSize *        fromPixel = (fromSize *)        i->memory; \
  unsigned toSize * toPixel   = (unsigned toSize *) o->memory; \
  unsigned toSize * end       = toPixel + result.width * result.height; \
  while (toPixel < end) \
  { \
	fromSize v = min (max (*fromPixel++, (fromSize) 0.0), (fromSize) 1.0); \
	delinearize (v); \
	unsigned int t = (unsigned int) (v * (255 << 8)); \
	*toPixel++ =   ((redShift   > 0 ? t << redShift   : t >> -redShift)   & redMask) \
	             | ((greenShift > 0 ? t << greenShift : t >> -greenShift) & greenMask) \
                 | ((blueShift  > 0 ? t << blueShift  : t >> -blueShift)  & blueMask) \
                 | alphaMask; \
  } \
}

#define GrayFloat2OddBits(fromSize) \
{ \
  fromSize *      fromPixel = (fromSize *)      i->memory; \
  unsigned char * toPixel   = (unsigned char *) o->memory; \
  unsigned char * end       = toPixel + result.width * result.height * 3; \
  while (toPixel < end) \
  { \
	fromSize v = min (max (*fromPixel++, (fromSize) 0.0), (fromSize) 1.0); \
	delinearize (v); \
	Int2Char t; \
	t.all = (unsigned int) (v * (255 << 8)); \
	t.all =   ((redShift   > 0 ? t.all << redShift   : t.all >> -redShift)   & redMask) \
	        | ((greenShift > 0 ? t.all << greenShift : t.all >> -greenShift) & greenMask) \
            | ((blueShift  > 0 ? t.all << blueShift  : t.all >> -blueShift)  & blueMask) \
            | alphaMask; \
    toPixel[0] = t.piece0; \
    toPixel[1] = t.piece1; \
    toPixel[2] = t.piece2; \
    toPixel += 3; \
  } \
}

// fromGrayChar () produces a bogus alpha channel!  Fix it.
void
PixelFormatRGBABits::fromGrayChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFF, 0xFF, 0xFF, 0, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;
  alphaShift *= -1;

  switch (depth)
  {
    case 1:
	  Bits2Bits (char, char, 0xFF, 0xFF, 0xFF, 0, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 2:
	  Bits2Bits (char, short, 0xFF, 0xFF, 0xFF, 0, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 3:
	  Bits2OddBits (char, 0xFF, 0xFF, 0xFF, 0, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 4:
    default:
	  Bits2Bits (char, int, 0xFF, 0xFF, 0xFF, 0, redMask, greenMask, blueMask, alphaMask);
  }
}

void
PixelFormatRGBABits::fromGrayShort (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  unsigned int grayMask = ((PixelFormatGrayShort *) image.format)->grayMask;
  shift (grayMask, grayMask, grayMask, grayMask, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;
  alphaShift *= -1;

  switch (depth)
  {
    case 1:
	  Bits2Bits (short, char, grayMask, grayMask, grayMask, 0, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 2:
	  Bits2Bits (short, short, grayMask, grayMask, grayMask, 0, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 3:
	  Bits2OddBits (short, grayMask, grayMask, grayMask, 0, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 4:
    default:
	  Bits2Bits (short, int, grayMask, grayMask, grayMask, 0, redMask, greenMask, blueMask, alphaMask);
  }
}

void
PixelFormatRGBABits::fromGrayFloat (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;
  alphaShift *= -1;

  switch (depth)
  {
    case 1:
	  GrayFloat2Bits (float, char);
	  break;
    case 2:
	  GrayFloat2Bits (float, short);
	  break;
    case 3:
	  GrayFloat2OddBits (float);
	  break;
    case 4:
    default:
	  GrayFloat2Bits (float, int);
  }
}

void
PixelFormatRGBABits::fromGrayDouble (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;
  alphaShift *= -1;

  switch (depth)
  {
    case 1:
	  GrayFloat2Bits (double, char);
	  break;
    case 2:
	  GrayFloat2Bits (double, short);
	  break;
    case 3:
	  GrayFloat2OddBits (double);
	  break;
    case 4:
    default:
	  GrayFloat2Bits (double, int);
  }
}

void
PixelFormatRGBABits::fromRGBABits (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (redMask, greenMask, blueMask, alphaMask, redShift, greenShift, blueShift, alphaShift);

  switch (depth)
  {
    case 1:
	  switch (that->depth)
	  {
	    case 1:
		  Bits2Bits (char, char, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 2:
		  Bits2Bits (short, char, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 3:
		  OddBits2Bits (char, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 4:
	    default:
		  Bits2Bits (int, char, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
	  }
	  break;
    case 2:
	  switch (that->depth)
	  {
	    case 1:
		  Bits2Bits (char, short, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 2:
		  Bits2Bits (short, short, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 3:
		  OddBits2Bits (short, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 4:
	    default:
		  Bits2Bits (int, short, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
	  }
	  break;
    case 3:
	  switch (that->depth)
	  {
	    case 1:
		  Bits2OddBits (char, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 2:
		  Bits2OddBits (short, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 3:
		  OddBits2OddBits (that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 4:
	    default:
		  Bits2OddBits (int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
	  }
	  break;
    case 4:
    default:
	  switch (that->depth)
	  {
	    case 1:
		  Bits2Bits (char, int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 2:
		  Bits2Bits (short, int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 3:
		  OddBits2Bits (int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
		  break;
	    case 4:
	    default:
		  Bits2Bits (int, int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, redMask, greenMask, blueMask, alphaMask);
	  }
	  break;
  }
}

bool
PixelFormatRGBABits::operator == (const PixelFormat & that) const
{
  if (depth != that.depth)
  {
	return false;
  }
  if (const PixelFormatRGBABits * other = dynamic_cast<const PixelFormatRGBABits *> (& that))
  {
	return    redMask   == other->redMask
	       && greenMask == other->greenMask
	       && blueMask  == other->blueMask
	       && alphaMask == other->alphaMask;
  }
  if (const PixelFormatGrayChar * other = dynamic_cast<const PixelFormatGrayChar *> (& that))
  {
	return    redMask   == 0xFF
	       && greenMask == 0xFF
	       && blueMask  == 0xFF;
  }
  if (const PixelFormatGrayShort * other = dynamic_cast<const PixelFormatGrayShort *> (& that))
  {
	return    redMask   == other->grayMask
	       && greenMask == other->grayMask
	       && blueMask  == other->grayMask;
  }
  return false;
}

void
PixelFormatRGBABits::shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift) const
{
  unsigned int t;

  redShift = 0;
  while (redMask >>= 1) {redShift++;}
  t = this->redMask;
  while (t >>= 1) {redShift--;}

  greenShift = 0;
  while (greenMask >>= 1) {greenShift++;}
  t = this->greenMask;
  while (t >>= 1) {greenShift--;}

  blueShift = 0;
  while (blueMask >>= 1) {blueShift++;}
  t = this->blueMask;
  while (t >>= 1) {blueShift--;}

  alphaShift = 0;
  while (alphaMask >>= 1) {alphaShift++;}
  t = this->alphaMask;
  while (t >>= 1) {alphaShift--;}
}

unsigned int
PixelFormatRGBABits::getRGBA (void * pixel) const
{
  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFF000000, 0xFF0000, 0xFF00, 0xFF, redShift, greenShift, blueShift, alphaShift);

  Int2Char value;

  switch (depth)
  {
    case 1:
	  value.all = *((unsigned char *) pixel);
	  break;
    case 2:
	  value.all = *((unsigned short *) pixel);
	  break;
    case 3:
	  value.piece0 = ((unsigned char *) pixel)[0];
	  value.piece1 = ((unsigned char *) pixel)[1];
	  value.piece2 = ((unsigned char *) pixel)[2];
	  break;
    case 4:
    default:
	  value.all = *((unsigned int *) pixel);
	  break;
  }

  unsigned int r = value.all & redMask;
  unsigned int g = value.all & greenMask;
  unsigned int b = value.all & blueMask;
  unsigned int a = value.all & alphaMask;

  return   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & 0xFF000000)
		 | ((greenShift > 0 ? g << greenShift : g >> -greenShift) &   0xFF0000)
		 | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  &     0xFF00)
		 | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) &       0xFF);
}

unsigned char
PixelFormatRGBABits::getAlpha (void * pixel) const
{
  Int2Char value;
  switch (depth)
  {
    case 1:
	  value.all = *((unsigned char *) pixel);
	  break;
    case 2:
	  value.all = *((unsigned short *) pixel);
	  break;
    case 3:
	  value.piece0 = ((unsigned char *) pixel)[0];
	  value.piece1 = ((unsigned char *) pixel)[1];
	  value.piece2 = ((unsigned char *) pixel)[2];
	  break;
    case 4:
    default:
	  value.all = *((unsigned int *) pixel);
	  break;
  }
  unsigned int a = value.all & alphaMask;

  int shift = 7;
  unsigned int mask = alphaMask;
  while (mask >>= 1) {shift--;}

  return (shift > 0 ? a << shift : a >> -shift) & 0xFF;
}

void
PixelFormatRGBABits::setRGBA (void * pixel, unsigned int rgba) const
{
  unsigned int r = rgba & 0xFF000000;
  unsigned int g = rgba &   0xFF0000;
  unsigned int b = rgba &     0xFF00;
  unsigned int a = rgba &       0xFF;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFF000000, 0xFF0000, 0xFF00, 0xFF, redShift, greenShift, blueShift, alphaShift);
  // The shifts must be negated, since we are going from the rgba format
  // into our own, rather than the other way around, as the above call might
  // suggest.  The negation is handled by switching the shifts below.

  Int2Char value;
  value.all =   ((redShift   > 0 ? r >> redShift   : r << -redShift)   & redMask)
	          | ((greenShift > 0 ? g >> greenShift : g << -greenShift) & greenMask)
              | ((blueShift  > 0 ? b >> blueShift  : b << -blueShift)  & blueMask)
              | ((alphaShift > 0 ? a >> alphaShift : a << -alphaShift) & alphaMask);

  switch (depth)
  {
    case 1:
	  *((unsigned char *) pixel) = value.all;
	  break;
    case 2:
	  *((unsigned short *) pixel) = value.all;
	  break;
    case 3:
	  ((unsigned char *) pixel)[0] = value.piece0;
	  ((unsigned char *) pixel)[1] = value.piece1;
	  ((unsigned char *) pixel)[2] = value.piece2;
	  break;
    case 4:
    default:
	  *((unsigned int *) pixel) = value.all;
	  break;
  }
}

void
PixelFormatRGBABits::setAlpha (void * pixel, unsigned char alpha) const
{
  // There's no need to be careful about number of bytes in a pixel,
  // because the bit masking below will safely preserve data outside the
  // current pixel.

  int shift = -7;
  unsigned int mask = alphaMask;
  while (mask >>= 1) {shift++;}

  unsigned int a = alpha;
  a = (shift > 0 ? a << shift : a >> -shift) & alphaMask;

  * (unsigned int *) pixel = a | ((* (unsigned int *) pixel) & ~alphaMask);
}


// class PixelFormatRGBAChar ---------------------------------------------------

PixelFormatRGBAChar::PixelFormatRGBAChar ()
#if BYTE_ORDER == LITTLE_ENDIAN
: PixelFormatRGBABits (4, 0xFF, 0xFF00, 0xFF0000, 0xFF000000)
#else
: PixelFormatRGBABits (4, 0xFF000000, 0xFF0000, 0xFF00, 0xFF)
#endif
{
}

Image
PixelFormatRGBAChar::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayFloat))
  {
	fromGrayFloat (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayDouble))
  {
	fromGrayDouble (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBChar))
  {
	fromRGBChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBABits))
  {
	fromRGBABits (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatRGBAChar::fromGrayChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	unsigned char t = *fromPixel++;
	toPixel[0] = t;
	toPixel[1] = t;
	toPixel[2] = t;
	toPixel[3] = 0xFF;
	toPixel += 4;
  }
}

void
PixelFormatRGBAChar::fromGrayFloat (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  float *         fromPixel = (float *)         i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	float v = min (max (*fromPixel++, 0.0f), 1.0f);
	delinearize (v);
	unsigned char t = (unsigned char) (v * 255);
	toPixel[0] = t;
	toPixel[1] = t;
	toPixel[2] = t;
	toPixel[3] = 0xFF;
	toPixel += 4;
  }
}

void
PixelFormatRGBAChar::fromGrayDouble (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  double *        fromPixel = (double *)        i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	float v = min (max (*fromPixel++, 0.0), 1.0);
	delinearize (v);
	unsigned char t = (unsigned char) (v * 255);
	toPixel[0] = t;
	toPixel[1] = t;
	toPixel[2] = t;
	toPixel[3] = 0xFF;
	toPixel += 4;
  }
}

void
PixelFormatRGBAChar::fromRGBChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	toPixel[0] = fromPixel[0];
	toPixel[1] = fromPixel[1];
	toPixel[2] = fromPixel[2];
	toPixel[3] = 0xFF;
	toPixel += 4;
	fromPixel += 3;
  }
}

unsigned int
PixelFormatRGBAChar::getRGBA (void * pixel) const
{
# if BYTE_ORDER == LITTLE_ENDIAN
  return bswap (*((unsigned int *) pixel));
# else
  return *((unsigned int *) pixel);
# endif
}

unsigned char
PixelFormatRGBAChar::getAlpha (void * pixel) const
{
  return ((unsigned char *) pixel)[3];
}

void
PixelFormatRGBAChar::setRGBA (void * pixel, unsigned int rgba) const
{
# if BYTE_ORDER == LITTLE_ENDIAN
  *((unsigned int *) pixel) = bswap (rgba);
# else
  *((unsigned int *) pixel) = rgba;
# endif
}

void
PixelFormatRGBAChar::setAlpha (void * pixel, unsigned char alpha) const
{
  ((unsigned char *) pixel)[3] = alpha;
}


// class PixelFormatRGBChar ---------------------------------------------------

PixelFormatRGBChar::PixelFormatRGBChar ()
#if BYTE_ORDER == LITTLE_ENDIAN
: PixelFormatRGBABits (3, 0xFF, 0xFF00, 0xFF0000, 0x0)
#else
: PixelFormatRGBABits (3, 0xFF0000, 0xFF00, 0xFF, 0x0)
#endif
{
}

Image
PixelFormatRGBChar::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayShort))
  {
	fromGrayShort (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayFloat))
  {
	fromGrayFloat (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayDouble))
  {
	fromGrayDouble (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBAChar))
  {
	fromRGBAChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatRGBABits))
  {
	fromRGBABits (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatRGBChar::fromGrayChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	unsigned char t = *fromPixel++;
	toPixel[0] = t;
	toPixel[1] = t;
	toPixel[2] = t;
	toPixel += 3;
  }
}

void
PixelFormatRGBChar::fromGrayShort (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned short * fromPixel = (unsigned short *) i->memory;
  unsigned char *  toPixel   = (unsigned char *)  o->memory;
  unsigned char *  end       = toPixel + result.width * result.height * depth;
  int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift - 8;
  if (grayShift > 0)
  {
	while (toPixel < end)
	{
	  unsigned char t = *fromPixel++ << grayShift;
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
  }
  else
  {
	while (toPixel < end)
	{
	  unsigned char t = *fromPixel++ >> -grayShift;
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
  }
}

void
PixelFormatRGBChar::fromGrayFloat (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  float *         fromPixel = (float *)         i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	float v = min (max (*fromPixel++, 0.0f), 1.0f);
	delinearize (v);
	unsigned char t = (unsigned char) (v * 255);
	toPixel[0] = t;
	toPixel[1] = t;
	toPixel[2] = t;
	toPixel += 3;
  }
}

void
PixelFormatRGBChar::fromGrayDouble (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  double *        fromPixel = (double *)        i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	double v = min (max (*fromPixel++, 0.0), 1.0);
	delinearize (v);
	unsigned char t = (unsigned char) (v * 255);
	toPixel[0] = t;
	toPixel[1] = t;
	toPixel[2] = t;
	toPixel += 3;
  }
}

void
PixelFormatRGBChar::fromRGBAChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->memory;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height * depth;
  while (toPixel < end)
  {
	toPixel[0] = fromPixel[0];
	toPixel[1] = fromPixel[1];
	toPixel[2] = fromPixel[2];
	toPixel += 3;
	fromPixel += 4;
  }
}

unsigned int
PixelFormatRGBChar::getRGBA  (void * pixel) const
{
# if BYTE_ORDER == LITTLE_ENDIAN
  return bswap (*((unsigned int *) pixel)) | 0xFF;
# else
  return *((unsigned int *) pixel) | 0xFF;
# endif
}

void
PixelFormatRGBChar::setRGBA  (void * pixel, unsigned int rgba) const
{
  ((unsigned char *) pixel)[2] = rgba >>= 8;
  ((unsigned char *) pixel)[1] = rgba >>= 8;
  ((unsigned char *) pixel)[0] = rgba >>  8;
}


// class PixelFormatRGBAShort -------------------------------------------------

PixelFormatRGBAShort::PixelFormatRGBAShort ()
{
  planes     = 1;
  depth      = 8;
  precedence = 5;  // Above RGBAChar and GrayFloat.  Slightly below GrayDouble.
  monochrome = false;
  hasAlpha   = true;
}

unsigned int
PixelFormatRGBAShort::getRGBA (void * pixel) const
{
  unsigned int rgba;
  rgba =   ((((unsigned short *) pixel)[0] << 16) & 0xFF000000)
         | ((((unsigned short *) pixel)[1] <<  8) &   0xFF0000)
         | ( ((unsigned short *) pixel)[2]        &     0xFF00)
         |   ((unsigned short *) pixel)[3] >>  8;
  return rgba;
}

unsigned char
PixelFormatRGBAShort::getAlpha (void * pixel) const
{
  return ((unsigned short *) pixel)[3] >> 8;
}

void
PixelFormatRGBAShort::setRGBA (void * pixel, unsigned int rgba) const
{
  ((unsigned short *) pixel)[0] = (rgba & 0xFF000000) >> 16;
  ((unsigned short *) pixel)[1] = (rgba &   0xFF0000) >> 8;
  ((unsigned short *) pixel)[2] = (rgba &     0xFF00);
  ((unsigned short *) pixel)[3] = (rgba &       0xFF) << 8;
}

void
PixelFormatRGBAShort::setAlpha (void * pixel, unsigned char alpha) const
{
  ((unsigned short *) pixel)[3] = alpha << 8;
}


// class PixelFormatRGBShort -------------------------------------------------

PixelFormatRGBShort::PixelFormatRGBShort ()
{
  planes     = 1;
  depth      = 6;
  precedence = 5;  // Above RGBAChar and GrayFloat.  Slightly below GrayDouble.
  monochrome = false;
  hasAlpha   = false;
}

unsigned int
PixelFormatRGBShort::getRGBA (void * pixel) const
{
  unsigned int rgba = 0;
  rgba =   ((((unsigned short *) pixel)[0] << 16) & 0xFF000000)
         | ((((unsigned short *) pixel)[1] <<  8) &   0xFF0000)
         | ( ((unsigned short *) pixel)[2]        &     0xFF00)
         | 0xFF;
  return rgba;
}

void
PixelFormatRGBShort::setRGBA (void * pixel, unsigned int rgba) const
{
  ((unsigned short *) pixel)[0] = (rgba & 0xFF000000) >> 16;
  ((unsigned short *) pixel)[1] = (rgba &   0xFF0000) >> 8;
  ((unsigned short *) pixel)[2] = (rgba &     0xFF00);
}


// class PixelFormatRGBAFloat -------------------------------------------------

PixelFormatRGBAFloat::PixelFormatRGBAFloat ()
{
  planes     = 1;
  depth      = 4 * sizeof (float);
  precedence = 7;  // Above everything
  monochrome = false;
  hasAlpha   = true;
}

unsigned int
PixelFormatRGBAFloat::getRGBA (void * pixel) const
{
  float rgbaValues[4];
  getRGBA (pixel, rgbaValues);
  for (int i = 0; i < 4; i++)
  {
	rgbaValues[i] = max (rgbaValues[i], 0.0f);
	rgbaValues[i] = min (rgbaValues[i], 1.0f);
  }
  delinearize (rgbaValues[0]);
  delinearize (rgbaValues[1]);
  delinearize (rgbaValues[2]);
  // assume alpha is already linear
  unsigned int r = ((unsigned int) (rgbaValues[0] * 255)) << 24;
  unsigned int g = ((unsigned int) (rgbaValues[1] * 255)) << 16;
  unsigned int b = ((unsigned int) (rgbaValues[2] * 255)) <<  8;
  unsigned int a = ((unsigned int) (rgbaValues[3] * 255));
  return r | g | b | a;
}

void
PixelFormatRGBAFloat::getRGBA (void * pixel, float values[]) const
{
  //memcpy (values, pixel, 4 * sizeof (float));  // It is probably more efficient to directly copy these than to set up the function call.
  values[0] = ((float *) pixel)[0];
  values[1] = ((float *) pixel)[1];
  values[2] = ((float *) pixel)[2];
  values[3] = ((float *) pixel)[3];
}

unsigned char
PixelFormatRGBAFloat::getAlpha (void * pixel) const
{
  return (unsigned char) (((float *) pixel)[3] * 255);
}

void
PixelFormatRGBAFloat::setRGBA (void * pixel, unsigned int rgba) const
{
  float * rgbaValues = (float *) pixel;
  rgbaValues[0] = ((rgba & 0xFF000000) >> 24) / 255.0f;
  rgbaValues[1] = ((rgba &   0xFF0000) >> 16) / 255.0f;
  rgbaValues[2] = ((rgba &     0xFF00) >>  8) / 255.0f;
  rgbaValues[3] =  (rgba &       0xFF)        / 255.0f;
  linearize (rgbaValues[0]);
  linearize (rgbaValues[1]);
  linearize (rgbaValues[2]);
  // Don't linearize alpha, because it is always linear
}

void
PixelFormatRGBAFloat::setRGBA (void * pixel, float values[]) const
{
  //memcpy (pixel, values, 4 * sizeof (float));  // It is probably more efficient to directly copy these than to set up the function call.
  ((float *) pixel)[0] = values[0];
  ((float *) pixel)[1] = values[1];
  ((float *) pixel)[2] = values[2];
  ((float *) pixel)[3] = values[3];
}

void
PixelFormatRGBAFloat::setAlpha (void * pixel, unsigned char alpha) const
{
  ((float *) pixel)[3] = alpha / 255.0f;
}


// class PixelFormatUYVYChar --------------------------------------------------

// Notes...
// YUV <-> RGB conversion matrices are specified by the standards in terms of
// non-linear RGB.  IE: even though the conversion matrices are linear ops,
// they work on non-linear inputs.  Therefore, even though YUV is essentially
// non-linear, it should not be linearized until after it is converted into
// RGB.  The matrices output non-linear RGB.

PixelFormatUYVYChar::PixelFormatUYVYChar ()
{
  planes     = 1;
  depth      = 2;
  precedence = 1;  // above GrayChar and below GrayShort
  monochrome = false;
  hasAlpha   = false;
}

Image
PixelFormatUYVYChar::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatYUYVChar))
  {
	fromYUYVChar (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatUYVYChar::fromYUYVChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned int * fromPixel = (unsigned int *) i->memory;
  unsigned int * toPixel   = (unsigned int *) o->memory;
  unsigned int * end       = toPixel + result.width * result.height / 2;  // width *must* be a multiple of 2!
  while (toPixel < end)
  {
	register unsigned int p = *fromPixel++;
	*toPixel++ = ((p & 0xFF0000) << 8) | ((p & 0xFF000000) >> 8) | ((p & 0xFF) << 8) | ((p & 0xFF00) >> 8);
  }
}

unsigned int
PixelFormatUYVYChar::getRGBA (void * pixel) const
{
  int y;
  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = ((unsigned char *) pixel)[3] << 16;
  }
  else  // on 32-bit word boundary
  {
	y = ((unsigned char *) pixel)[1] << 16;
  }
  int u = ((unsigned char *) pixel)[0] - 128;
  int v = ((unsigned char *) pixel)[2] - 128;

  // R = Y           +1.4022*V
  // G = Y -0.3456*U -0.7145*V
  // B = Y +1.7710*U
  // The coefficients below are in fixed-point with decimal between bits 15 and 16.
  // Figure out a more elegant way to express these constants letting the
  // compiler do the work!
  unsigned int r = min (max (y               + 0x166F7 * v, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u,               0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatUYVYChar::getYUV (void * pixel) const
{
  unsigned int y;
  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = ((unsigned char *) pixel)[3] << 16;
  }
  else  // on 32-bit word boundary
  {
	y = ((unsigned char *) pixel)[1] << 16;
  }

  return y | (((unsigned char *) pixel)[0] << 8) | ((unsigned char *) pixel)[2];
}

unsigned char
PixelFormatUYVYChar::getGray (void * pixel) const
{
  return ((unsigned char *) pixel)[1];
}

void
PixelFormatUYVYChar::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  // Y =  0.2989*R +0.5866*G +0.1145*B
  // U = -0.1687*R -0.3312*G +0.5000*B
  // V =  0.5000*R -0.4183*G -0.0816*B
  unsigned char y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b,            0), 0xFFFFFF) >> 16;
  unsigned char u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000, 0), 0xFFFFFF) >> 16;
  unsigned char v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000, 0), 0xFFFFFF) >> 16;

  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	((unsigned char *) pixel)[0] = u;
	((unsigned char *) pixel)[2] = v;
	((unsigned char *) pixel)[3] = y;
  }
  else  // on 32-bit word boundary
  {
	((unsigned char *) pixel)[0] = u;
	((unsigned char *) pixel)[1] = y;
	((unsigned char *) pixel)[2] = v;
  }
}

void
PixelFormatUYVYChar::setYUV (void * pixel, unsigned int yuv) const
{
  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	((unsigned char *) pixel)[0] = (yuv & 0xFF00) >>  8;
	((unsigned char *) pixel)[2] =  yuv &   0xFF;
	((unsigned char *) pixel)[3] =  yuv           >> 16;
  }
  else  // on 32-bit word boundary
  {
	((unsigned char *) pixel)[0] = (yuv & 0xFF00) >>  8;
	((unsigned char *) pixel)[1] =  yuv           >> 16;
	((unsigned char *) pixel)[2] =  yuv &   0xFF;
  }
}


// class PixelFormatYUYVChar --------------------------------------------------

PixelFormatYUYVChar::PixelFormatYUYVChar ()
{
  planes     = 1;
  depth      = 2;
  precedence = 1;  // same as UYVY
  monochrome = false;
  hasAlpha   = false;
}

Image
PixelFormatYUYVChar::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height, depth);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatUYVYChar))
  {
	fromUYVYChar (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatYUYVChar::fromUYVYChar (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned int * fromPixel = (unsigned int *) i->memory;
  unsigned int * toPixel   = (unsigned int *) o->memory;
  unsigned int * end       = toPixel + result.width * result.height / 2;
  while (toPixel < end)
  {
	register unsigned int p = *fromPixel++;
	*toPixel++ = ((p & 0xFF0000) << 8) | ((p & 0xFF000000) >> 8) | ((p & 0xFF) << 8) | ((p & 0xFF00) >> 8);
  }
}

unsigned int
PixelFormatYUYVChar::getRGBA (void * pixel) const
{
  int y;
  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = ((unsigned char *) pixel)[2] << 16;
  }
  else  // on 32-bit word boundary
  {
	y = ((unsigned char *) pixel)[0] << 16;
  }
  int u = ((unsigned char *) pixel)[1] - 128;
  int v = ((unsigned char *) pixel)[3] - 128;

  unsigned int r = min (max (y               + 0x166F7 * v, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u,               0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatYUYVChar::getYUV (void * pixel) const
{
  unsigned int y;
  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = ((unsigned char *) pixel)[2] << 16;
  }
  else  // on 32-bit word boundary
  {
	y = ((unsigned char *) pixel)[0] << 16;
  }

  return y | (((unsigned char *) pixel)[1] << 8) | ((unsigned char *) pixel)[3];
}

unsigned char
PixelFormatYUYVChar::getGray (void * pixel) const
{
  return * (unsigned char *) pixel;
}

void
PixelFormatYUYVChar::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  unsigned char y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b,            0), 0xFFFFFF) >> 16;
  unsigned char u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000, 0), 0xFFFFFF) >> 16;
  unsigned char v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000, 0), 0xFFFFFF) >> 16;

  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	((unsigned char *) pixel)[1] = u;
	((unsigned char *) pixel)[2] = y;
	((unsigned char *) pixel)[3] = v;
  }
  else  // on 32-bit word boundary
  {
	((unsigned char *) pixel)[0] = y;
	((unsigned char *) pixel)[1] = u;
	((unsigned char *) pixel)[3] = v;
  }
}

void
PixelFormatYUYVChar::setYUV (void * pixel, unsigned int yuv) const
{
  if (((unsigned long) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	((unsigned char *) pixel)[1] = (yuv & 0xFF00) >>  8;
	((unsigned char *) pixel)[2] =  yuv           >> 16;
	((unsigned char *) pixel)[3] =  yuv &   0xFF;
  }
  else  // on 32-bit word boundary
  {
	((unsigned char *) pixel)[0] =  yuv           >> 16;
	((unsigned char *) pixel)[1] = (yuv & 0xFF00) >>  8;
	((unsigned char *) pixel)[3] =  yuv &   0xFF;
  }
}


// class PixelFormatUYVChar ---------------------------------------------------

PixelFormatUYVChar::PixelFormatUYVChar ()
{
  planes     = 1;
  depth      = 3;
  precedence = 1;  // same as UYVY
  monochrome = false;
  hasAlpha   = false;
}

unsigned int
PixelFormatUYVChar::getRGBA (void * pixel) const
{
  int u = ((unsigned char *) pixel)[0] - 128;
  int y = ((unsigned char *) pixel)[1] << 16;
  int v = ((unsigned char *) pixel)[2] - 128;

  unsigned int r = min (max (y               + 0x166F7 * v, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u,               0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatUYVChar::getYUV (void * pixel) const
{
  return (((unsigned char *) pixel)[1] << 16) | (((unsigned char *) pixel)[0] << 8) | ((unsigned char *) pixel)[2];
}

unsigned char
PixelFormatUYVChar::getGray (void * pixel) const
{
  return ((unsigned char *) pixel)[1];
}

void
PixelFormatUYVChar::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  unsigned char y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b,            0), 0xFFFFFF) >> 16;
  unsigned char u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000, 0), 0xFFFFFF) >> 16;
  unsigned char v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000, 0), 0xFFFFFF) >> 16;

  ((unsigned char *) pixel)[0] = u;
  ((unsigned char *) pixel)[1] = y;
  ((unsigned char *) pixel)[2] = v;
}

void
PixelFormatUYVChar::setYUV (void * pixel, unsigned int yuv) const
{
  ((unsigned char *) pixel)[0] = (yuv & 0xFF00) >>  8;
  ((unsigned char *) pixel)[1] =  yuv           >> 16;
  ((unsigned char *) pixel)[2] =  yuv &   0xFF;
}


// class PixelFormatHLSFloat --------------------------------------------------

const float root32 = sqrtf (3.0f) / 2.0f;
const float onesixth = 1.0f / 6.0f;
const float onethird = 1.0f / 3.0f;
const float twothirds = 2.0f / 3.0f;

PixelFormatHLSFloat::PixelFormatHLSFloat ()
{
  planes     = 1;
  depth      = 3 * sizeof (float);
  precedence = 7;  // on par with RGBAFloat
  monochrome = false;
  hasAlpha   = false;
}

unsigned int
PixelFormatHLSFloat::getRGBA (void * pixel) const
{
  float rgbaValues[4];
  getRGBA (pixel, rgbaValues);
  delinearize (rgbaValues[0]);
  delinearize (rgbaValues[1]);
  delinearize (rgbaValues[2]);
  // assume alpha is already linear
  unsigned int r = ((unsigned int) (rgbaValues[0] * 255)) << 24;
  unsigned int g = ((unsigned int) (rgbaValues[1] * 255)) << 16;
  unsigned int b = ((unsigned int) (rgbaValues[2] * 255)) <<  8;
  unsigned int a =  (unsigned int) (rgbaValues[3] * 255);
  return r | g | b | a;
}

inline float
PixelFormatHLSFloat::HLSvalue (const float & n1, const float & n2, float h) const
{
  if (h > 1.0f)
  {
	h -= 1.0f;
  }
  if (h < 0)
  {
	h += 1.0f;
  }

  if (h < onesixth)
  {
    return n1 + (n2 - n1) * h * 6.0f;
  }
  else if (h < 0.5f)
  {
    return n2;
  }
  else if (h < twothirds)
  {
    return n1 + (n2 - n1) * (twothirds - h) * 6.0f;
  }
  else
  {
    return n1;
  }
}

void
PixelFormatHLSFloat::getRGBA (void * pixel, float values[]) const
{
  float h = ((float *) pixel)[0];
  float l = ((float *) pixel)[1];
  float s = ((float *) pixel)[2];

  if (s == 0)
  {
    values[0] = l;
    values[1] = l;
    values[2] = l;
  }
  else
  {
	float m2;
	if (l <= 0.5f)
	{
	  m2 = l + l * s;
	}
	else
	{
	  m2 = l + s - l * s;
	}
	float m1 = 2.0f * l - m2;

	double barf;
	h = modf (h, &barf);
	if (h < 0)
	{
	  h += 1.0f;
	}

    values[0] = HLSvalue (m1, m2, h + onethird);
    values[1] = HLSvalue (m1, m2, h);
    values[2] = HLSvalue (m1, m2, h - onethird);
  }

  values[3] = 1.0f;
}

void
PixelFormatHLSFloat::setRGBA (void * pixel, unsigned int rgba) const
{
  float rgbaValues[3];  // Ignore alpha channel, because it is not processed or stored by any function of this format.
  rgbaValues[0] = ((rgba & 0xFF000000) >> 24) / 255.0f;
  rgbaValues[1] = ((rgba &   0xFF0000) >> 16) / 255.0f;
  rgbaValues[2] = ((rgba &     0xFF00) >>  8) / 255.0f;
  linearize (rgbaValues[0]);
  linearize (rgbaValues[1]);
  linearize (rgbaValues[2]);
  setRGBA (pixel, rgbaValues);
}

void
PixelFormatHLSFloat::setRGBA (void * pixel, float values[]) const
{
  // Lightness
  float rgbmax = max (values[0], max (values[1], values[2]));
  float rgbmin = min (values[0], min (values[1], values[2]));
  float l = (rgbmax + rgbmin) / 2.0f;

  // Hue and Saturation
  float h;
  float s;
  if (rgbmax == rgbmin)
  {
	h = 0;
    s = 0;
  }
  else
  {
	float mmm = rgbmax - rgbmin;  // "max minus min"
	float mpm = rgbmax + rgbmin;  // "max plus min"

	// Saturation
	if (l <= 0.5f)
	{
      s = mmm / mpm;
	}
    else
	{
      s = mmm / (2.0f - mpm);
	}

	// Hue
	float x =  -0.5f * values[0] -   0.5f * values[1] + values[2];
	float y = root32 * values[0] - root32 * values[1];
	h = atan2f (y, x) / (2 * PI) - onethird;
	if (h < 0)
	{
	  h += 1.0f;
	}
  }

  ((float *) pixel)[0] = h;
  ((float *) pixel)[1] = l;
  ((float *) pixel)[2] = s;
}
