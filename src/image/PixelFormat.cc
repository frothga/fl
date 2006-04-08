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
Revision 1.37  2006/04/08 15:16:08  Fred
Strip "Char" from all YUV-type formats.  It is ugly, and doesn't add any information in this case.

Revision 1.36  2006/04/08 14:46:16  Fred
Add PixelFormatUYYVYY.  Fix deficiency in PlanarYUV::fromAny(): it now handles converting to a non-planar buffer, needed by UYYVYY.

Guard PlanarYCbCr against similar deficiency, but don't fix for now, because no obvious need for it.

Revision 1.35  2006/04/06 03:48:42  Fred
Count bits in each color mask and include that info as metadata on RGBABits.  Add function "dublicate" which echoes the bits in a channel in order to fill out a wider channel.  Use to fill out alpha channel in RGBABits accessors.  Should use it for all converters as well, but that will slow them down.

Revision 1.34  2006/04/05 02:40:57  Fred
Handle stride != width.

Supply missing alpha value in RGBABits conversions if source alphaMask == 0.

Revision 1.33  2006/04/02 03:05:03  Fred
Add PlanarYCbCr class, and use it instead of generic PlanarYUV for YUV420 and YUV411.  This allows proper conversion of the excursions for video.

Change PixelFormatPlanar into PixelFormatYUV and use it to store ratioH and ratioV for all YUV classes.  Move job of selecting PixelBuffer type back to PixelFormat::buffer(), and base the selection on the number of planes.

Add inner loop to some converters to handle the difference between stride and image width.  Some converters still need this change.

Add 0.5 to the result of all fixed-point arithmetic to get the same effect as rint().  This produces more consistent results, which the test program needs.

Change PixelFormat::build* routines to use double rather than float.  Not really an important change, but it costs nothing to use the extra precision.  Maybe it would be possible to get rid of the linear hack and just calculate the gamma function across the entire range.

Fix GrayFloat::fromGrayShort() and RGBChar::fromGrayShort() to use lut.

Fix GrayShort::fromAny() to use linear getGray() accessor.

Get rid of alpha channel in RGBABits::fromGrayFloat() and fromGrayDouble().

Add UYVY::fromAny(), YUYV::fromAny() and PlanarYUV::fromAny() to average RGB values when computing UV components.

Add PlanarYUVChar::operator== to compare ratioH and ratioV.

Revision 1.32  2006/03/20 05:39:25  Fred
Add PixelFormatPlanarYUV.  Broaden PixelFormat::fromAny() to handle outputting to a non-packed buffer.

Add PixelFormat::buffer() to create an appropriate PixelBuffer for each PixelFormat.

Revision 1.31  2006/03/18 14:25:55  Fred
Use look up tables to convert gamma, rather than functions.  Hopefully this will make the process more efficient.

Changed 16 bit formats (GrayShort, RGBShort) to use gamma=1, on the assumption that the larger number of bits makes them more suitable for linear data, similar to float formats.  Not sure if images in the wild will satisfy this assumption.  Need more research.  Plan to add dynamically built luts for these formats at some point, so one can specify the gamma for the data.

Revision 1.30  2006/03/13 03:50:19  Fred
Remove comment from RGBABits::fromGrayChar() on the assumption that the alpha problem is now fixed.

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


PixelFormatGrayChar           fl::GrayChar;
PixelFormatGrayShort          fl::GrayShort;
PixelFormatGrayFloat          fl::GrayFloat;
PixelFormatGrayDouble         fl::GrayDouble;
PixelFormatRGBAChar           fl::RGBAChar;
PixelFormatRGBAShort          fl::RGBAShort;
PixelFormatRGBAFloat          fl::RGBAFloat;
PixelFormatRGBChar            fl::RGBChar;
PixelFormatRGBShort           fl::RGBShort;
PixelFormatUYVY           fl::UYVY;
PixelFormatYUYV           fl::YUYV;
PixelFormatUYV            fl::UYV;
PixelFormatUYYVYY             fl::UYYVYY;
PixelFormatPlanarYCbCr    fl::YUV420 (2, 2);
PixelFormatPlanarYCbCr    fl::YUV411 (4, 1);
PixelFormatHLSFloat           fl::HLSFloat;

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


// class PixelFormat ----------------------------------------------------------

unsigned char * PixelFormat::lutFloat2Char = PixelFormat::buildFloat2Char ();
float *         PixelFormat::lutChar2Float = PixelFormat::buildChar2Float ();

PixelFormat::~PixelFormat ()
{
}

Image
PixelFormat::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height);
  result.timestamp = image.timestamp;

  fromAny (image, result);

  return result;
}

PixelBuffer *
PixelFormat::buffer () const
{
  if (planes == 1)
  {
	return new PixelBufferPacked;
  }
  else
  {
	return new PixelBufferPlanar;
  }
}

/**
   Uses getRGBA() and setRGBA().  XYZ would be more accurate, but this
   is also adequate, since RGB values are well defined (as non-linear sRGB).
**/
void
PixelFormat::fromAny (const Image & image, Image & result) const
{
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	if (o)
	{
	  unsigned char * dest = (unsigned char *) o->memory;
	  unsigned char * end  = dest + image.width * image.height * depth;
	  int step = (i->stride - image.width) * sourceDepth;
	  while (dest < end)
	  {
		unsigned char * rowEnd = dest + result.width * depth;
		while (dest < rowEnd)
		{
		  setRGBA (dest, sourceFormat->getRGBA (source));
		  source += sourceDepth;
		  dest += depth;
		}
		source += step;
	  }
	}
	else
	{
	  for (int y = 0; y < image.height; y++)
	  {
		for (int x = 0; x < image.width; x++)
		{
		  setRGBA (result.buffer->pixel (x, y), sourceFormat->getRGBA (source));
		  source += sourceDepth;
		}
	  }
	}
  }
  else
  {
	if (o)
	{
	  unsigned char * dest = (unsigned char *) o->memory;
	  for (int y = 0; y < image.height; y++)
	  {
		for (int x = 0; x < image.width; x++)
		{
		  setRGBA (dest, sourceFormat->getRGBA (image.buffer->pixel (x, y)));
		  dest += depth;
		}
	  }
	}
	else
	{
	  for (int y = 0; y < image.height; y++)
	  {
		for (int x = 0; x < image.width; x++)
		{
		  setRGBA (result.buffer->pixel (x, y), sourceFormat->getRGBA (image.buffer->pixel (x, y)));
		}
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
  values[0] = lutChar2Float[(rgba & 0xFF000000) >> 24];
  values[1] = lutChar2Float[(rgba &   0xFF0000) >> 16];
  values[2] = lutChar2Float[(rgba &     0xFF00) >>  8];
  values[3] = (rgba & 0xFF) / 255.0f;  // Don't linearize alpha, because already linear
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
   See PixelFormatUYVY::setRGBA() for more details on the conversion matrix.
 **/
unsigned int
PixelFormat::getYUV  (void * pixel) const
{
  unsigned int rgba = getRGBA (pixel);

  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  unsigned int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
  unsigned int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
  unsigned int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) & 0xFF0000;

  return y | (u >> 8) | (v >> 16);
}

unsigned char
PixelFormat::getGray (void * pixel) const
{
  unsigned int rgba = getRGBA (pixel);
  unsigned int r = (rgba & 0xFF000000) >> 16;
  unsigned int g = (rgba &   0xFF0000) >>  8;
  unsigned int b = (rgba &     0xFF00);
  return ((redWeight * r + greenWeight * g + blueWeight * b) / totalWeight + 0x80) >> 8;
}

void
PixelFormat::getGray (void * pixel, float & gray) const
{
  gray = lutChar2Float[getGray (pixel)];
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
  rgba |= lutFloat2Char[(unsigned int) (65535 * min (max (values[0], 0.0f), 1.0f))] << 24;
  rgba |= lutFloat2Char[(unsigned int) (65535 * min (max (values[1], 0.0f), 1.0f))] << 16;
  rgba |= lutFloat2Char[(unsigned int) (65535 * min (max (values[2], 0.0f), 1.0f))] <<  8;
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

  // See PixelFormatUYVY::getRGBA() for an explanation of this arithmetic.
  unsigned int r = min (max (y               + 0x166F7 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u               + 0x8000, 0), 0xFFFFFF);

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
  gray = min (max (gray, 0.0f), 1.0f);
  unsigned int iv = lutFloat2Char[(unsigned short) (65535 * gray)];
  unsigned int rgba = (iv << 24) | (iv << 16) | (iv << 8) | 0xFF;
  setRGBA (pixel, rgba);
}

void
PixelFormat::setAlpha (void * pixel, unsigned char alpha) const
{
  // Presumably, the derived PixelFormat does not have an alpha channel,
  // so just ignore this request.
}

inline unsigned char *
PixelFormat::buildFloat2Char ()
{
  unsigned char * result = (unsigned char *) malloc (65536);
  for (int i = 0; i < 65536; i++)
  {
	double f = i / 65535.0;

	// For small numbers, use linear approximation.  sRGB says that some
	// systems can't handle these small pow() computations accurately.
	if (f <= 0.0031308)
	{
	  f *= 12.92;
	}
	else
	{
	  f = 1.055 * pow (f, 1.0 / 2.4) - 0.055;
	}

	result[i] = (unsigned char) rint (f * 255);
  }
  return result;
}

inline float *
PixelFormat::buildChar2Float ()
{
  float * result = (float *) malloc (256 * sizeof (float));
  for (int i = 0; i < 256; i++)
  {
	double f = i / 255.0;

	if (f <= 0.04045)
	{
	  f /= 12.92;
	}
	else
	{
	  f = pow ((f + 0.055) / 1.055, 2.4);
	}

	result[i] = f;
  }
  return result;
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

  result.resize (image.width, image.height);
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
  else if (typeid (* image.format) == typeid (PixelFormatPlanarYCbCr))
  {
	fromYCbCr (image, result);
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
  int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift;
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = lutFloat2Char[*fromPixel++ << grayShift];
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  float p = min (max (*fromPixel++, 0.0f), 1.0f);
	  *toPixel++ = lutFloat2Char[(unsigned short) (65535 * p)];
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  double p = min (max (*fromPixel++, 0.0), 1.0);
	  *toPixel++ = lutFloat2Char[(unsigned short) (65535 * p)];
	}
	fromPixel += step;
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
  int step = (i->stride - image.width) * image.format->depth;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  unsigned int t;
	  t  = fromPixel[0] * (redWeight   << 8);
	  t += fromPixel[1] * (greenWeight << 8);
	  t += fromPixel[2] * (blueWeight  << 8);
	  fromPixel += 4;
	  *toPixel++ = (t / totalWeight + 0x80) >> 8;
	}
	fromPixel += step;
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
	int step = i->stride - image.width; \
    while (toPixel < end) \
    { \
	  unsigned char * rowEnd = toPixel + result.width; \
	  while (toPixel < rowEnd) \
	  { \
		unsigned int r = *fromPixel & that->redMask; \
		unsigned int g = *fromPixel & that->greenMask; \
		unsigned int b = *fromPixel & that->blueMask; \
		fromPixel++; \
		*toPixel++ = ((  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight \
					   + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight \
					   + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight \
					  ) / totalWeight + 0x80) >> 8; \
	  } \
	  fromPixel += step; \
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
	  int step = (i->stride - image.width) * 3;
	  while (toPixel < end)
	  {
		unsigned char * rowEnd = toPixel + result.width;
		while (toPixel < rowEnd)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel += 3;
		  unsigned int r = t & that->redMask;
		  unsigned int g = t & that->greenMask;
		  unsigned int b = t & that->blueMask;
		  *toPixel++ = ((  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight
						 + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight
						 + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight
						) / totalWeight + 0x80) >> 8;
		}
		fromPixel += step;
	  }
	  break;
	}
    case 4:
    default:
	  RGBBits2GrayChar (int);
  }
}

void
PixelFormatGrayChar::fromYCbCr (const Image & image, Image & result) const
{
  PixelBufferPlanar * i = (PixelBufferPlanar *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->plane0;
  unsigned char * toPixel   = (unsigned char *) o->memory;
  unsigned char * end       = toPixel + result.width * result.height;
  int step = i->stride0 - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = PixelFormatPlanarYCbCr::lutYout[*fromPixel++];
	}
	fromPixel += step;
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
	int step = (i->stride - image.width) * sourceDepth;
	unsigned char * end = dest + image.width * image.height;
	while (dest < end)
	{
	  unsigned char * rowEnd = dest + result.width;
	  while (dest < rowEnd)
	  {
		*dest++ = sourceFormat->getGray (source);
		source += sourceDepth;
	  }
	  source += step;
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
  values[1] = lutChar2Float[*(unsigned char *) pixel];
  values[2] = 0;
}

unsigned char
PixelFormatGrayChar::getGray (void * pixel) const
{
  return *((unsigned char *) pixel);
}

void
PixelFormatGrayChar::getGray (void * pixel, float & gray) const
{
  gray = lutChar2Float[*(unsigned char *) pixel];
}

void
PixelFormatGrayChar::setRGBA (void * pixel, unsigned int rgba) const
{
  unsigned int r = (rgba & 0xFF000000) >> 16;
  unsigned int g = (rgba &   0xFF0000) >>  8;
  unsigned int b =  rgba &     0xFF00;

  *((unsigned char *) pixel) = ((r * redWeight + g * greenWeight + b * blueWeight) / totalWeight + 0x80) >> 8;
}

void
PixelFormatGrayChar::setXYZ (void * pixel, float values[]) const
{
  // Convert Y value to non-linear form
  float v = min (max (values[1], 0.0f), 1.0f);
  *((unsigned char *) pixel) = lutFloat2Char[(unsigned short) (65535 * v)];
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
  *((unsigned char *) pixel) = lutFloat2Char[(unsigned short) (65535 * gray)];
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

  result.resize (image.width, image.height);
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned short * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = (unsigned short) (grayMask * lutChar2Float[*fromPixel++]);
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned short * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  float p = min (max (*fromPixel++, 0.0f), 1.0f);
	  *toPixel++ = (unsigned short) (p * grayMask);
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned short * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  double p = min (max (*fromPixel++, 0.0), 1.0);
	  *toPixel++ = (unsigned short) (p * grayMask);
	}
	fromPixel += step;
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
	int step = (i->stride - image.width) * sourceDepth;
	unsigned short * end = dest + image.width * image.height;
	while (dest < end)
	{
	  unsigned short * rowEnd = dest + result.width;
	  while (dest < rowEnd)
	  {
		float gray;
		sourceFormat->getGray (source, gray);
		*dest++ = (unsigned short) (grayMask * gray);
		source += sourceDepth;
	  }
	  source += step;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		float gray;
		sourceFormat->getGray (image.buffer->pixel (x, y), gray);
		*dest++ = (unsigned short) (grayMask * gray);
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
  unsigned int t = lutFloat2Char[*(unsigned short *) pixel << grayShift];
  return (t << 24) | (t << 16) | (t << 8) | 0xFF;
}

void
PixelFormatGrayShort::getXYZ (void * pixel, float values[]) const
{
  values[0] = 0;
  values[1] = *((unsigned short *) pixel) / (float) grayMask;
  values[2] = 0;
}

unsigned char
PixelFormatGrayShort::getGray (void * pixel) const
{
  return lutFloat2Char[*(unsigned short *) pixel << grayShift];
}

void
PixelFormatGrayShort::getGray (void * pixel, float & gray) const
{
  gray = *((unsigned short *) pixel) / (float) grayMask;
}

void
PixelFormatGrayShort::setRGBA (void * pixel, unsigned int rgba) const
{
  float r = lutChar2Float[ rgba             >> 24];
  float g = lutChar2Float[(rgba & 0xFF0000) >> 16];
  float b = lutChar2Float[(rgba &   0xFF00) >>  8];

  float t = r * redToY + g * greenToY + b * blueToY;
  *((unsigned short *) pixel) = (unsigned short) (grayMask * t);
}

void
PixelFormatGrayShort::setXYZ (void * pixel, float values[]) const
{
  float v = min (max (values[1], 0.0f), 1.0f);
  *((unsigned short *) pixel) = (unsigned short) (grayMask * v);
}

void
PixelFormatGrayShort::setGray (void * pixel, unsigned char gray) const
{
  *((unsigned short *) pixel) = (unsigned short) (grayMask * lutChar2Float[gray]);
}

void
PixelFormatGrayShort::setGray (void * pixel, float gray) const
{
  gray = min (max (gray, 0.0f), 1.0f);
  *((unsigned short *) pixel) = (unsigned short) (grayMask * gray);
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

  result.resize (image.width, image.height);
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
  else if (typeid (* image.format) == typeid (PixelFormatPlanarYCbCr))
  {
	fromYCbCr (image, result);
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = lutChar2Float[*fromPixel++];
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  float grayMask = ((PixelFormatGrayShort *) image.format)->grayMask;
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel++ / grayMask;
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = (float) *fromPixel++;
	}
	fromPixel += step;
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
  int step = (i->stride - image.width) * image.format->depth;
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  float r = lutChar2Float[fromPixel[0]];
	  float g = lutChar2Float[fromPixel[1]];
	  float b = lutChar2Float[fromPixel[2]];
	  fromPixel += 4;
	  *toPixel++ = redToY * r + greenToY * g + blueToY * b;
	}
	fromPixel += step;
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
  int step = (i->stride - image.width) * image.format->depth;
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  float r = lutChar2Float[fromPixel[0]];
	  float g = lutChar2Float[fromPixel[1]];
	  float b = lutChar2Float[fromPixel[2]];
	  fromPixel += 3;
	  *toPixel++ = redToY * r + greenToY * g + blueToY * b;
	}
	fromPixel += step;
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
	int step = i->stride - image.width; \
	while (toPixel < end) \
    { \
	  float * rowEnd = toPixel + result.width; \
	  while (toPixel < rowEnd) \
	  { \
		unsigned int r = *fromPixel & that->redMask; \
		unsigned int g = *fromPixel & that->greenMask; \
		unsigned int b = *fromPixel & that->blueMask; \
		fromPixel++; \
		float fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift]; \
		float fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift]; \
		float fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift]; \
		*toPixel++ = redToY * fr + greenToY * fg + blueToY * fb; \
	  } \
	  fromPixel += step; \
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
	  int step = (i->stride - image.width) * 3;
	  while (toPixel < end)
	  {
		float * rowEnd = toPixel + result.width;
		while (toPixel < rowEnd)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel += 3;
		  unsigned int r = t & that->redMask;
		  unsigned int g = t & that->greenMask;
		  unsigned int b = t & that->blueMask;
		  float fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
		  float fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
		  float fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
		  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
		}
		fromPixel += step;
	  }
	  break;
	}
    case 4:
    default:
	  RGBBits2GrayFloat (int);
  }
}

void
PixelFormatGrayFloat::fromYCbCr (const Image & image, Image & result) const
{
  PixelBufferPlanar * i = (PixelBufferPlanar *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->plane0;
  float *         toPixel   = (float *)         o->memory;
  float *         end       = toPixel + result.width * result.height;
  int step = i->stride0 - image.width;
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = PixelFormatPlanarYCbCr::lutGrayOut[*fromPixel++];
	}
	fromPixel += step;
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
	int step = (i->stride - image.width) * sourceFormat->depth;
	float * end = dest + image.width * image.height;
	while (dest < end)
	{
	  float * rowEnd = dest + result.width;
	  while (dest < rowEnd)
	  {
		sourceFormat->getGray (source, *dest++);
		source += sourceDepth;
	  }
	  source += step;
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
  unsigned int t = lutFloat2Char[(unsigned short) (65535 * v)];
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
  return lutFloat2Char[(unsigned short) (65535 * v)];
}

void
PixelFormatGrayFloat::getGray (void * pixel, float & gray) const
{
  gray = *((float *) pixel);
}

void
PixelFormatGrayFloat::setRGBA (void * pixel, unsigned int rgba) const
{
  float r = lutChar2Float[(rgba & 0xFF000000) >> 24];
  float g = lutChar2Float[(rgba &   0xFF0000) >> 16];
  float b = lutChar2Float[(rgba &     0xFF00) >>  8];
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
  *((float *) pixel) = lutChar2Float[gray];
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

  result.resize (image.width, image.height);
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
  else if (typeid (* image.format) == typeid (PixelFormatPlanarYCbCr))
  {
	fromYCbCr (image, result);
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = lutChar2Float[*fromPixel++];
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  double grayMask = ((PixelFormatGrayShort *) image.format)->grayMask;
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel++ / grayMask;
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel++;
	}
	fromPixel += step;
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
  int step = (i->stride - image.width) * image.format->depth;
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  double r = lutChar2Float[fromPixel[0]];
	  double g = lutChar2Float[fromPixel[1]];
	  double b = lutChar2Float[fromPixel[2]];
	  fromPixel += 4;
	  *toPixel++ = redToY * r + greenToY * g + blueToY * b;
	}
	fromPixel += step;
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
  int step = (i->stride - image.width) * image.format->depth;
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  double r = lutChar2Float[fromPixel[0]];
	  double g = lutChar2Float[fromPixel[1]];
	  double b = lutChar2Float[fromPixel[2]];
	  fromPixel += 3;
	  *toPixel++ = redToY * r + greenToY * g + blueToY * b;
	}
	fromPixel += step;
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
	int step = i->stride - image.width; \
	while (toPixel < end) \
    { \
	  double * rowEnd = toPixel + result.width; \
	  while (toPixel < rowEnd) \
	  { \
		unsigned int r = *fromPixel & that->redMask; \
		unsigned int g = *fromPixel & that->greenMask; \
		unsigned int b = *fromPixel & that->blueMask; \
		fromPixel++; \
		double fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift]; \
		double fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift]; \
		double fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift]; \
		*toPixel++ = redToY * fr + greenToY * fg + blueToY * fb; \
	  } \
	  fromPixel += step; \
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
	  int step = (i->stride - image.width) * 3;
	  while (toPixel < end)
	  {
		double * rowEnd = toPixel + result.width;
		while (toPixel < rowEnd)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel += 3;
		  unsigned int r = t & that->redMask;
		  unsigned int g = t & that->greenMask;
		  unsigned int b = t & that->blueMask;
		  double fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
		  double fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
		  double fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
		  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
		}
		fromPixel += step;
	  }
	  break;
	}
    case 4:
    default:
	  RGBBits2GrayDouble (int);
  }
}

void
PixelFormatGrayDouble::fromYCbCr (const Image & image, Image & result) const
{
  PixelBufferPlanar * i = (PixelBufferPlanar *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned char * fromPixel = (unsigned char *) i->plane0;
  double *        toPixel   = (double *)        o->memory;
  double *        end       = toPixel + result.width * result.height;
  int step = i->stride0 - image.width;
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = PixelFormatPlanarYCbCr::lutGrayOut[*fromPixel++];
	}
	fromPixel += step;
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
	int step = (i->stride - image.width) * sourceDepth;
	double * end = dest + image.width * image.height;
	while (dest < end)
	{
	  double * rowEnd = dest + result.width;
	  while (dest < rowEnd)
	  {
		float value;
		sourceFormat->getGray (source, value);
		*dest++ = value;
		source += sourceDepth;
	  }
	  source += step;
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
  unsigned int t = lutFloat2Char[(unsigned short) (65535 * v)];
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
  return lutFloat2Char[(unsigned short) (65535 * v)];
}

void
PixelFormatGrayDouble::getGray (void * pixel, float & gray) const
{
  gray = *((double *) pixel);
}

void
PixelFormatGrayDouble::setRGBA (void * pixel, unsigned int rgba) const
{
  double r = lutChar2Float[(rgba & 0xFF000000) >> 24];
  double g = lutChar2Float[(rgba &   0xFF0000) >> 16];
  double b = lutChar2Float[(rgba &     0xFF00) >>  8];
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
  *((double *) pixel) = lutChar2Float[gray];
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

  redBits   = countBits (redMask);
  greenBits = countBits (greenMask);
  blueBits  = countBits (blueMask);
  alphaBits = countBits (alphaMask);

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

  result.resize (image.width, image.height);
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
  else if (typeid (* image.format) == typeid (PixelFormatPlanarYCbCr))
  {
	fromYCbCr (image, result);
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
	unsigned toSize * rowEnd = toPixel + result.width; \
	while (toPixel < rowEnd) \
	{ \
	  unsigned int r = *fromPixel & fromRed; \
	  unsigned int g = *fromPixel & fromGreen; \
	  unsigned int b = *fromPixel & fromBlue; \
	  unsigned int a = fromAlpha ? *fromPixel & fromAlpha : 0xFFFFFFFF; \
	  fromPixel++; \
	  *toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		           | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		           | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		           | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	} \
	fromPixel += step; \
  } \
}

#define OddBits2Bits(toSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char *   fromPixel = (unsigned char *)   i->memory; \
  endianAdjustFromPixel \
  unsigned toSize * toPixel   = (unsigned toSize *) o->memory; \
  unsigned toSize * end       = toPixel + result.width * result.height; \
  step *= 3; \
  while (toPixel < end) \
  { \
	unsigned toSize * rowEnd = toPixel + result.width; \
	while (toPixel < rowEnd) \
	{ \
	  unsigned int t = * (unsigned int *) fromPixel; \
	  fromPixel += 3; \
	  unsigned int r = t & fromRed; \
	  unsigned int g = t & fromGreen; \
	  unsigned int b = t & fromBlue; \
	  unsigned int a = fromAlpha ? t & fromAlpha : 0xFFFFFFFF; \
	  *toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	               | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	               | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	               | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	} \
	fromPixel += step; \
  } \
}

#define Bits2OddBits(fromSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned fromSize * fromPixel = (unsigned fromSize *) i->memory; \
  unsigned char *     toPixel   = (unsigned char *)     o->memory; \
  unsigned char *     end       = toPixel + result.width * result.height * 3; \
  while (toPixel < end) \
  { \
	unsigned char * rowEnd = toPixel + result.width * 3; \
	while (toPixel < rowEnd) \
	{ \
	  unsigned int r = *fromPixel & fromRed; \
	  unsigned int g = *fromPixel & fromGreen; \
	  unsigned int b = *fromPixel & fromBlue; \
	  unsigned int a = fromAlpha ? *fromPixel & fromAlpha : 0xFFFFFFFF; \
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
	fromPixel += step; \
  } \
}

#define OddBits2OddBits(fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char * fromPixel = (unsigned char *) i->memory; \
  endianAdjustFromPixel \
  unsigned char * toPixel   = (unsigned char *) o->memory; \
  unsigned char * end       = toPixel + result.width * result.height * 3; \
  step *= 3; \
  while (toPixel < end) \
  { \
	unsigned char * rowEnd = toPixel + result.width * 3; \
	while (toPixel < rowEnd) \
	{ \
	  Int2Char t; \
	  t.all = * (unsigned int *) fromPixel; \
	  fromPixel += 3; \
	  unsigned int r = t.all & fromRed; \
	  unsigned int g = t.all & fromGreen; \
	  unsigned int b = t.all & fromBlue; \
	  unsigned int a = fromAlpha ? t.all & fromAlpha : 0xFFFFFFFF; \
	  t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	          | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	          | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	          | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	  toPixel[0] = t.piece0; \
	  toPixel[1] = t.piece1; \
	  toPixel[2] = t.piece2; \
	  toPixel += 3; \
	} \
	fromPixel += step; \
  } \
}

#define GrayFloat2Bits(fromSize,toSize) \
{ \
  fromSize *        fromPixel = (fromSize *)        i->memory; \
  unsigned toSize * toPixel   = (unsigned toSize *) o->memory; \
  unsigned toSize * end       = toPixel + result.width * result.height; \
  while (toPixel < end) \
  { \
	unsigned toSize * rowEnd = toPixel + result.width; \
	while (toPixel < rowEnd) \
	{ \
	  fromSize v = min (max (*fromPixel++, (fromSize) 0.0), (fromSize) 1.0); \
	  unsigned int t = lutFloat2Char[(unsigned short) (65535 * v)]; \
	  *toPixel++ =   ((redShift   > 0 ? t << redShift   : t >> -redShift)   & redMask) \
	               | ((greenShift > 0 ? t << greenShift : t >> -greenShift) & greenMask) \
	               | ((blueShift  > 0 ? t << blueShift  : t >> -blueShift)  & blueMask) \
	               | alphaMask; \
	} \
	fromPixel += step; \
  } \
}

#define GrayFloat2OddBits(fromSize) \
{ \
  fromSize *      fromPixel = (fromSize *)      i->memory; \
  unsigned char * toPixel   = (unsigned char *) o->memory; \
  unsigned char * end       = toPixel + result.width * result.height * 3; \
  while (toPixel < end) \
  { \
	unsigned char * rowEnd = toPixel + result.width * 3; \
	while (toPixel < rowEnd) \
	{ \
	  fromSize v = min (max (*fromPixel++, (fromSize) 0.0), (fromSize) 1.0); \
	  Int2Char t; \
	  t.all = lutFloat2Char[(unsigned short) (65535 * v)]; \
	  t.all =   ((redShift   > 0 ? t.all << redShift   : t.all >> -redShift)   & redMask) \
	          | ((greenShift > 0 ? t.all << greenShift : t.all >> -greenShift) & greenMask) \
	          | ((blueShift  > 0 ? t.all << blueShift  : t.all >> -blueShift)  & blueMask) \
	          | alphaMask; \
	  toPixel[0] = t.piece0; \
	  toPixel[1] = t.piece1; \
	  toPixel[2] = t.piece2; \
	  toPixel += 3; \
	} \
	fromPixel += step; \
  } \
}

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

  int step = i->stride - image.width;

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

  int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift;
  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFF, 0xFF, 0xFF, 0, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;

  int step = i->stride - image.width;

# define GrayShort2Bits(toSize) \
  { \
    unsigned short *  fromPixel = (unsigned short *)  i->memory; \
	unsigned toSize * toPixel   = (unsigned toSize *) o->memory; \
	unsigned toSize * end       = toPixel + result.width * result.height; \
	while (toPixel < end) \
	{ \
	  unsigned toSize * rowEnd = toPixel + result.width; \
	  while (toPixel < rowEnd) \
	  { \
		unsigned int t = lutFloat2Char[*fromPixel++ << grayShift]; \
		*toPixel++ =   ((redShift   > 0 ? t << redShift   : t >> -redShift)   & redMask) \
		             | ((greenShift > 0 ? t << greenShift : t >> -greenShift) & greenMask) \
		             | ((blueShift  > 0 ? t << blueShift  : t >> -blueShift)  & blueMask) \
		             | alphaMask; \
	  } \
	  fromPixel += step; \
	} \
  }

  switch (depth)
  {
    case 1:
	  GrayShort2Bits (char);
	  break;
    case 2:
	  GrayShort2Bits (short);
	  break;
    case 3:
	{
	  unsigned short * fromPixel = (unsigned short *) i->memory;
	  unsigned char *  toPixel   = (unsigned char *)  o->memory;
	  unsigned char *  end       = toPixel + result.width * result.height * 3;
	  while (toPixel < end)
	  {
		unsigned char * rowEnd = toPixel + result.width * 3;
		while (toPixel < rowEnd)
		{
		  int v = lutFloat2Char[*fromPixel++ << grayShift];
		  Int2Char t;
		  t.all =   ((redShift   > 0 ? v << redShift   : v >> -redShift)   & redMask)
		          | ((greenShift > 0 ? v << greenShift : v >> -greenShift) & greenMask)
		          | ((blueShift  > 0 ? v << blueShift  : v >> -blueShift)  & blueMask)
		          | alphaMask;
		  toPixel[0] = t.piece0;
		  toPixel[1] = t.piece1;
		  toPixel[2] = t.piece2;
		  toPixel += 3;
		}
		fromPixel += step;
	  }
	  break;
	}
    case 4:
    default:
	  GrayShort2Bits (int);
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
  shift (0xFF, 0xFF, 0xFF, 0, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;

  int step = i->stride - image.width;

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
  shift (0xFF, 0xFF, 0xFF, 0, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;

  int step = i->stride - image.width;

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

  int step = i->stride - image.width;

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

void
PixelFormatRGBABits::fromYCbCr (const Image & image, Image & result) const
{
  PixelBufferPlanar * i = (PixelBufferPlanar *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;

  unsigned char * fromPixel = (unsigned char *) i->plane0;
  unsigned char * Cb        = (unsigned char *) i->plane1;
  unsigned char * Cr        = (unsigned char *) i->plane2;

  assert (image.width % i->ratioH == 0  &&  image.height % i->ratioV == 0);
  int rowWidth = image.width;
  int blockRowWidth = i->ratioH;
  int blockSwath = o->stride * i->ratioV;
  int step12 = i->stride12 - image.width / i->ratioH;
  int toStep   = o->stride  * i->ratioV - image.width;
  int fromStep = i->stride0 * i->ratioV - image.width;
  int toBlockStep   = i->ratioH - o->stride  * i->ratioV;
  int fromBlockStep = i->ratioH - i->stride0 * i->ratioV;
  int toBlockRowStep   = o->stride  - i->ratioH;
  int fromBlockRowStep = i->stride0 - i->ratioH;

  // This can be made more efficient by making code specific to certain
  // combinations of ratioH and ratioV which unrolls the inner loops.

#define YCbCr2Bits(toSize) \
{ \
  unsigned toSize * toPixel = (unsigned toSize *) o->memory; \
  unsigned toSize * end     = toPixel + result.width * result.height; \
  while (toPixel < end) \
  { \
	unsigned toSize * rowEnd = toPixel + rowWidth; \
	while (toPixel < rowEnd) \
	{ \
	  int u = *Cb++ - 128; \
	  int v = *Cr++ - 128; \
	  int tr =               0x19895 * v; \
	  int tg =  0x644A * u +  0xD01F * v; \
	  int tb = 0x20469 * u; \
	  unsigned toSize * blockEnd = toPixel + blockSwath; \
	  while (toPixel < blockEnd) \
	  { \
		unsigned toSize * blockRowEnd = toPixel + blockRowWidth; \
		while (toPixel < blockRowEnd) \
		{ \
		  int y = (*fromPixel++ - 16) * 0x12A15; \
		  unsigned int r = min (max (y + tr + 0x8000, 0), 0xFFFFFF); \
		  unsigned int g = min (max (y - tg + 0x8000, 0), 0xFFFFFF); \
		  unsigned int b = min (max (y + tb + 0x8000, 0), 0xFFFFFF); \
		  *toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & redMask) \
			           | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & greenMask) \
			           | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & blueMask) \
			           | alphaMask; \
		} \
		toPixel += toBlockRowStep; \
		fromPixel += fromBlockRowStep; \
	  } \
	  toPixel += toBlockStep; \
	  fromPixel += fromBlockStep; \
	} \
	toPixel += toStep; \
	fromPixel += fromStep; \
	Cb += step12; \
	Cr += step12; \
  } \
}

  switch (depth)
  {
    case 1:
	  YCbCr2Bits (char);
	  break;
    case 2:
	  YCbCr2Bits (short);
	  break;
    case 3:
	{
	  blockSwath *= 3;
	  rowWidth *= 3;
	  blockRowWidth *= 3;
	  toBlockRowStep *= 3;
	  toBlockStep *= 3;
	  toStep *= 3;
	  unsigned char * toPixel = (unsigned char *) o->memory;
	  unsigned char * end     = toPixel + result.width * result.height * depth;
	  while (toPixel < end)
	  {
		unsigned char * rowEnd = toPixel + rowWidth;
		while (toPixel < rowEnd)
		{
		  int u = *Cb++ - 128;
		  int v = *Cr++ - 128;
		  int tr =               0x19895 * v;
		  int tg =  0x644A * u +  0xD01F * v;
		  int tb = 0x20469 * u;
		  unsigned char * blockEnd = toPixel + blockSwath;
		  while (toPixel < blockEnd)
		  {
			unsigned char * blockRowEnd = toPixel + blockRowWidth;
			while (toPixel < blockRowEnd)
			{
			  int y = (*fromPixel++ - 16) * 0x12A15;
			  unsigned int r = min (max (y + tr + 0x8000, 0), 0xFFFFFF);
			  unsigned int g = min (max (y - tg + 0x8000, 0), 0xFFFFFF);
			  unsigned int b = min (max (y + tb + 0x8000, 0), 0xFFFFFF);
			  Int2Char t;
			  t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & redMask)
				      | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & greenMask)
				      | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & blueMask)
				      | alphaMask;
			  toPixel[0] = t.piece0;
			  toPixel[1] = t.piece1;
			  toPixel[2] = t.piece2;
			  toPixel += 3;
			}
			toPixel += toBlockRowStep;
			fromPixel += fromBlockRowStep;
		  }
		  toPixel += toBlockStep;
		  fromPixel += fromBlockStep;
		}
		toPixel += toStep;
		fromPixel += fromStep;
		Cb += step12;
		Cr += step12;
	  }
	  break;
	}
    case 4:
	  YCbCr2Bits (int);
	  break;
    default:
	  throw "Unhandled depth in PixelFormatRGBABits::fromYCbCr";
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
  if (redMask  &&  this->redMask)
  {
	while (redMask >>= 1) {redShift++;}
	t = this->redMask;
	while (t >>= 1) {redShift--;}
  }

  greenShift = 0;
  if (greenMask  &&  this->greenMask)
  {
	while (greenMask >>= 1) {greenShift++;}
	t = this->greenMask;
	while (t >>= 1) {greenShift--;}
  }

  blueShift = 0;
  if (blueMask  &&  this->blueMask)
  {
	while (blueMask >>= 1) {blueShift++;}
	t = this->blueMask;
	while (t >>= 1) {blueShift--;}
  }

  alphaShift = 0;
  if (alphaMask  &&  this->alphaMask)
  {
	while (alphaMask >>= 1) {alphaShift++;}
	t = this->alphaMask;
	while (t >>= 1) {alphaShift--;}
  }
}

int
PixelFormatRGBABits::countBits (unsigned int mask)
{
  int count = 0;
  while (mask)
  {
	if (mask & 0x1) count++;
	mask >>= 1;
  }
  return count;
}

static inline unsigned int
dublicate (unsigned int value, int shift, int bits, int times)
{
  if (shift > 0)
  {
	value <<= shift;
  }
  else
  {
	value >>= -shift;
  }

  unsigned int result = value;
  while (--times > 0)
  {
	value >>= bits;
	result |= value;
  }

  return result;
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
  int ab = alphaBits;
  if (! alphaMask)
  {
	a = 0xFFFFFFFF;
	ab = 8;
  }

  return   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & 0xFF000000)
		 | ((greenShift > 0 ? g << greenShift : g >> -greenShift) &   0xFF0000)
		 | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  &     0xFF00)
		 | (dublicate (a, alphaShift, alphaBits, 7 / ab + 1)      &       0xFF);
}

unsigned char
PixelFormatRGBABits::getAlpha (void * pixel) const
{
  if (! alphaMask) return 0xFF;

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

  return dublicate (a, shift, alphaBits, 7 / alphaBits + 1) & 0xFF;
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

  result.resize (image.width, image.height);
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  unsigned char t = *fromPixel++;
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel[3] = 0xFF;
	  toPixel += 4;
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  float v = min (max (*fromPixel++, 0.0f), 1.0f);
	  unsigned char t = lutFloat2Char[(unsigned short) (65535 * v)];
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel[3] = 0xFF;
	  toPixel += 4;
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  double v = min (max (*fromPixel++, 0.0), 1.0);
	  unsigned char t = lutFloat2Char[(unsigned short) (65535 * v)];
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel[3] = 0xFF;
	  toPixel += 4;
	}
	fromPixel += step;
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
  int step = (i->stride - image.width) * 3;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  toPixel[0] = fromPixel[0];
	  toPixel[1] = fromPixel[1];
	  toPixel[2] = fromPixel[2];
	  toPixel[3] = 0xFF;
	  toPixel += 4;
	  fromPixel += 3;
	}
	fromPixel += step;
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

  result.resize (image.width, image.height);
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  unsigned char t = *fromPixel++;
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
	fromPixel += step;
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
  int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift;
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 3;
	while (toPixel < rowEnd)
	{
	  unsigned char t = lutFloat2Char[*fromPixel++ << grayShift];
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  float v = min (max (*fromPixel++, 0.0f), 1.0f);
	  unsigned char t = lutFloat2Char[(unsigned short) (65535 * v)];
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
	fromPixel += step;
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
  int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  double v = min (max (*fromPixel++, 0.0), 1.0);
	  unsigned char t = lutFloat2Char[(unsigned short) (65535 * v)];
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
	fromPixel += step;
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
  int step = (i->stride - image.width) * image.format->depth;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * depth;
	while (toPixel < rowEnd)
	{
	  toPixel[0] = fromPixel[0];
	  toPixel[1] = fromPixel[1];
	  toPixel[2] = fromPixel[2];
	  toPixel += 3;
	  fromPixel += 4;
	}
	fromPixel += step;
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
  ((unsigned char *) pixel)[2] = (rgba >>= 8) & 0xFF;
  ((unsigned char *) pixel)[1] = (rgba >>= 8) & 0xFF;
  ((unsigned char *) pixel)[0] =  rgba >>  8;
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
  rgba =   (lutFloat2Char[((unsigned short *) pixel)[0]] << 24)
         | (lutFloat2Char[((unsigned short *) pixel)[1]] << 16)
         | (lutFloat2Char[((unsigned short *) pixel)[2]] <<  8)
         | 0xFF;
  return rgba;
}

void
PixelFormatRGBShort::setRGBA (void * pixel, unsigned int rgba) const
{
  ((unsigned short *) pixel)[0] = (unsigned short) (65535 * lutChar2Float[ rgba             >> 24]);
  ((unsigned short *) pixel)[1] = (unsigned short) (65535 * lutChar2Float[(rgba & 0xFF0000) >> 16]);
  ((unsigned short *) pixel)[2] = (unsigned short) (65535 * lutChar2Float[(rgba &   0xFF00) >>  8]);
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
  unsigned int r = (unsigned int) lutFloat2Char[(unsigned short) (65535 * rgbaValues[0])] << 24;
  unsigned int g = (unsigned int) lutFloat2Char[(unsigned short) (65535 * rgbaValues[1])] << 16;
  unsigned int b = (unsigned int) lutFloat2Char[(unsigned short) (65535 * rgbaValues[2])] <<  8;
  unsigned int a = (unsigned int) (255 * rgbaValues[3]);  // assume alpha is already linear
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
  rgbaValues[0] = lutChar2Float[ rgba             >> 24];
  rgbaValues[1] = lutChar2Float[(rgba & 0xFF0000) >> 16];
  rgbaValues[2] = lutChar2Float[(rgba &   0xFF00) >>  8];
  rgbaValues[3] = (rgba & 0xFF) / 255.0f;  // Don't linearize alpha, because it is always linear
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


// class PixelFormatUYVY ------------------------------------------------------

// Notes...
// YUV <-> RGB conversion matrices are specified by the standards in terms of
// non-linear RGB.  IE: even though the conversion matrices are linear ops,
// they work on non-linear inputs.  Therefore, even though YUV is essentially
// non-linear, it should not be linearized until after it is converted into
// RGB.  The matrices output non-linear RGB.

PixelFormatUYVY::PixelFormatUYVY ()
: PixelFormatYUV (2, 1)
{
  planes     = 1;
  depth      = 2;
  precedence = 1;  // above GrayChar and below GrayShort
  monochrome = false;
  hasAlpha   = false;
}

Image
PixelFormatUYVY::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  assert (image.width % 2 == 0);  // No odd-width images allowed!
  result.resize (image.width, image.height);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatYUYV))
  {
	fromYUYV (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatUYVY::fromAny (const Image & image, Image & result) const
{
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  unsigned char * dest = (unsigned char *) o->memory;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	unsigned char * end  = dest + image.width * image.height * depth;
	int step = (i->stride - image.width) * sourceDepth;
	while (dest < end)
	{
	  unsigned char * rowEnd = dest + result.width * depth;
	  while (dest < rowEnd)
	  {
		unsigned int rgba = sourceFormat->getRGBA (source);
		source += sourceDepth;
		int r0 =  rgba             >> 24;
		int g0 = (rgba & 0xFF0000) >> 16;
		int b0 = (rgba &   0xFF00) >>  8;
		dest[1] = min (max (0x4C84 * r0 + 0x962B * g0 + 0x1D4F * b0 + 0x8000, 0), 0xFFFFFF) >> 16;

		rgba = sourceFormat->getRGBA (source);
		source += sourceDepth;
		int r1 =  rgba             >> 24;
		int g1 = (rgba & 0xFF0000) >> 16;
		int b1 = (rgba &   0xFF00) >>  8;
		dest[3] = min (max (0x4C84 * r1 + 0x962B * g1 + 0x1D4F * b1 + 0x8000, 0), 0xFFFFFF) >> 16;

		r0 += r1;
		g0 += g1;
		b0 += b1;
		// Since r0, g0 and b0 are 9 bits wide, the appropriate constants below inclued 1 more bit than usual.
		dest[0] = min (max (- 0x2B2F * r0 - 0x54C9 * g0 + 0x8000 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;
		dest[2] = min (max (  0x8000 * r0 - 0x6B15 * g0 - 0x14E3 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;

		dest += 4;
	  }
	  source += step;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width;)
	  {
		unsigned int rgba = sourceFormat->getRGBA (image.buffer->pixel (x++, y));
		int r0 =  rgba             >> 24;
		int g0 = (rgba & 0xFF0000) >> 16;
		int b0 = (rgba &   0xFF00) >>  8;
		dest[1] = min (max (0x4C84 * r0 + 0x962B * g0 + 0x1D4F * b0 + 0x8000, 0), 0xFFFFFF) >> 16;

		rgba = sourceFormat->getRGBA (image.buffer->pixel (x++, y));
		int r1 =  rgba             >> 24;
		int g1 = (rgba & 0xFF0000) >> 16;
		int b1 = (rgba &   0xFF00) >>  8;
		dest[3] = min (max (0x4C84 * r1 + 0x962B * g1 + 0x1D4F * b1 + 0x8000, 0), 0xFFFFFF) >> 16;

		r0 += r1;
		g0 += g1;
		b0 += b1;
		dest[0] = min (max (- 0x2B2F * r0 - 0x54C9 * g0 + 0x8000 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;
		dest[2] = min (max (  0x8000 * r0 - 0x6B15 * g0 - 0x14E3 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;

		dest += 4;
	  }
	}
  }
}

void
PixelFormatUYVY::fromYUYV (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned int * fromPixel = (unsigned int *) i->memory;
  unsigned int * toPixel   = (unsigned int *) o->memory;
  unsigned int * end       = toPixel + result.width * result.height / 2;  // width *must* be a multiple of 2!
  int step = (i->stride - image.width) / 2;
  while (toPixel < end)
  {
	unsigned int * rowEnd = toPixel + result.width / 2;
	while (toPixel < rowEnd)
	{
	  register unsigned int p = *fromPixel++;
	  *toPixel++ = ((p & 0xFF0000) << 8) | ((p & 0xFF000000) >> 8) | ((p & 0xFF) << 8) | ((p & 0xFF00) >> 8);
	}
	fromPixel += step;
  }
}

unsigned int
PixelFormatUYVY::getRGBA (void * pixel) const
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
  unsigned int r = min (max (y               + 0x166F7 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u               + 0x8000, 0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatUYVY::getYUV (void * pixel) const
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
PixelFormatUYVY::getGray (void * pixel) const
{
  return ((unsigned char *) pixel)[1];
}

void
PixelFormatUYVY::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  // Y =  0.2989*R +0.5866*G +0.1145*B
  // U = -0.1687*R -0.3312*G +0.5000*B
  // V =  0.5000*R -0.4183*G -0.0816*B
  unsigned char y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

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
PixelFormatUYVY::setYUV (void * pixel, unsigned int yuv) const
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


// class PixelFormatYUYV ------------------------------------------------------

PixelFormatYUYV::PixelFormatYUYV ()
: PixelFormatYUV (2, 1)
{
  planes     = 1;
  depth      = 2;
  precedence = 1;  // same as UYVY
  monochrome = false;
  hasAlpha   = false;
}

Image
PixelFormatYUYV::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height);
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatUYVY))
  {
	fromUYVY (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatYUYV::fromAny (const Image & image, Image & result) const
{
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  unsigned char * dest = (unsigned char *) o->memory;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	int sourceDepth = sourceFormat->depth;
	unsigned char * end  = dest + image.width * image.height * depth;
	int step = (i->stride - image.width) * sourceDepth;
	while (dest < end)
	{
	  unsigned char * rowEnd = dest + result.width * depth;
	  while (dest < rowEnd)
	  {
		unsigned int rgba = sourceFormat->getRGBA (source);
		source += sourceDepth;
		int r0 =  rgba             >> 24;
		int g0 = (rgba & 0xFF0000) >> 16;
		int b0 = (rgba &   0xFF00) >>  8;
		dest[0] = min (max (0x4C84 * r0 + 0x962B * g0 + 0x1D4F * b0 + 0x8000, 0), 0xFFFFFF) >> 16;

		rgba = sourceFormat->getRGBA (source);
		source += sourceDepth;
		int r1 =  rgba             >> 24;
		int g1 = (rgba & 0xFF0000) >> 16;
		int b1 = (rgba &   0xFF00) >>  8;
		dest[2] = min (max (0x4C84 * r1 + 0x962B * g1 + 0x1D4F * b1 + 0x8000, 0), 0xFFFFFF) >> 16;

		r0 += r1;
		g0 += g1;
		b0 += b1;
		// Since r0, g0 and b0 are 9 bits wide, the appropriate constants below inclued 1 more bit than usual.
		dest[1] = min (max (- 0x2B2F * r0 - 0x54C9 * g0 + 0x8000 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;
		dest[3] = min (max (  0x8000 * r0 - 0x6B15 * g0 - 0x14E3 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;

		dest += 4;
	  }
	  source += step;
	}
  }
  else
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width;)
	  {
		unsigned int rgba = sourceFormat->getRGBA (image.buffer->pixel (x++, y));
		int r0 =  rgba             >> 24;
		int g0 = (rgba & 0xFF0000) >> 16;
		int b0 = (rgba &   0xFF00) >>  8;
		dest[0] = min (max (0x4C84 * r0 + 0x962B * g0 + 0x1D4F * b0 + 0x8000, 0), 0xFFFFFF) >> 16;

		rgba = sourceFormat->getRGBA (image.buffer->pixel (x++, y));
		int r1 =  rgba             >> 24;
		int g1 = (rgba & 0xFF0000) >> 16;
		int b1 = (rgba &   0xFF00) >>  8;
		dest[2] = min (max (0x4C84 * r1 + 0x962B * g1 + 0x1D4F * b1 + 0x8000, 0), 0xFFFFFF) >> 16;

		r0 += r1;
		g0 += g1;
		b0 += b1;
		dest[1] = min (max (- 0x2B2F * r0 - 0x54C9 * g0 + 0x8000 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;
		dest[3] = min (max (  0x8000 * r0 - 0x6B15 * g0 - 0x14E3 * b0 + 0x1000000 + 0x10000, 0), 0x1FFFFFF) >> 17;

		dest += 4;
	  }
	}
  }
}

void
PixelFormatYUYV::fromUYVY (const Image & image, Image & result) const
{
  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  assert (i  &&  o);

  unsigned int * fromPixel = (unsigned int *) i->memory;
  unsigned int * toPixel   = (unsigned int *) o->memory;
  unsigned int * end       = toPixel + result.width * result.height / 2;
  int step = (i->stride - image.width) / 2;
  while (toPixel < end)
  {
	unsigned int * rowEnd = toPixel + result.width / 2;
	while (toPixel < rowEnd)
	{
	  register unsigned int p = *fromPixel++;
	  *toPixel++ = ((p & 0xFF0000) << 8) | ((p & 0xFF000000) >> 8) | ((p & 0xFF) << 8) | ((p & 0xFF00) >> 8);
	}
	fromPixel += step;
  }
}

unsigned int
PixelFormatYUYV::getRGBA (void * pixel) const
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

  unsigned int r = min (max (y               + 0x166F7 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u               + 0x8000, 0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatYUYV::getYUV (void * pixel) const
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
PixelFormatYUYV::getGray (void * pixel) const
{
  return * (unsigned char *) pixel;
}

void
PixelFormatYUYV::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  unsigned char y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

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
PixelFormatYUYV::setYUV (void * pixel, unsigned int yuv) const
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


// class PixelFormatUYV -------------------------------------------------------

PixelFormatUYV::PixelFormatUYV ()
: PixelFormatYUV (1, 1)
{
  planes     = 1;
  depth      = 3;
  precedence = 1;  // same as UYVY
  monochrome = false;
  hasAlpha   = false;
}

unsigned int
PixelFormatUYV::getRGBA (void * pixel) const
{
  int u = ((unsigned char *) pixel)[0] - 128;
  int y = ((unsigned char *) pixel)[1] << 16;
  int v = ((unsigned char *) pixel)[2] - 128;

  unsigned int r = min (max (y               + 0x166F7 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u               + 0x8000, 0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatUYV::getYUV (void * pixel) const
{
  return (((unsigned char *) pixel)[1] << 16) | (((unsigned char *) pixel)[0] << 8) | ((unsigned char *) pixel)[2];
}

unsigned char
PixelFormatUYV::getGray (void * pixel) const
{
  return ((unsigned char *) pixel)[1];
}

void
PixelFormatUYV::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  unsigned char y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

  ((unsigned char *) pixel)[0] = u;
  ((unsigned char *) pixel)[1] = y;
  ((unsigned char *) pixel)[2] = v;
}

void
PixelFormatUYV::setYUV (void * pixel, unsigned int yuv) const
{
  ((unsigned char *) pixel)[0] = (yuv & 0xFF00) >>  8;
  ((unsigned char *) pixel)[1] =  yuv           >> 16;
  ((unsigned char *) pixel)[2] =  yuv &   0xFF;
}


// class PixelFormatPlanarYUV -------------------------------------------------

PixelFormatPlanarYUV::PixelFormatPlanarYUV (int ratioH, int ratioV)
: PixelFormatYUV (ratioH, ratioV)
{
  planes     = 3;
  depth      = sizeof (char);
  precedence = 1;  // same as UYVY
  monochrome = false;
  hasAlpha   = false;
}

void
PixelFormatPlanarYUV::fromAny (const Image & image, Image & result) const
{
  assert (image.width % ratioH == 0  &&  image.height % ratioV == 0);

  const PixelFormat * sourceFormat = image.format;
  PixelBuffer *       sourceBuffer = (PixelBuffer *) image.buffer;
  const int           sourceDepth  = sourceFormat->depth;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPlanar * o = (PixelBufferPlanar *) result.buffer;

  int rowWidth      = result.width;
  int blockRowWidth = ratioH;

  const unsigned int shift = 16 + (int) rint (log (ratioH * ratioV) / log (2));
  const int bias           = 0x808 << (shift - 4);  // include both bias and rounding in single constant
  const int maximum        = (~(unsigned int) 0) >> (24 - shift);

  if (o)
  {
	unsigned char * Y = (unsigned char *) o->plane0;
	unsigned char * U = (unsigned char *) o->plane1;
	unsigned char * V = (unsigned char *) o->plane2;

	const int blockSwath     = o->stride0 * ratioV;
	const int step12         = o->stride12 - result.width / ratioH;
	const int toStep         = o->stride0 * ratioV - result.width;
	const int toBlockStep    = ratioH - o->stride0 * ratioV;
	const int toBlockRowStep = o->stride0 - ratioH;

	if (i)
	{
	  unsigned char * source = (unsigned char *) i->memory;

	  const int fromStep         = (i->stride * ratioV - image.width) * sourceDepth;
	  const int fromBlockStep    = (ratioH - i->stride * ratioV     ) * sourceDepth;
	  const int fromBlockRowStep = (i->stride - ratioH              ) * sourceDepth;

	  unsigned char * end = Y + result.width * result.height;
	  while (Y < end)
	  {
		unsigned char * rowEnd = Y + rowWidth;
		while (Y < rowEnd)
		{
		  int r = 0;
		  int g = 0;
		  int b = 0;
		  unsigned char * blockEnd = Y + blockSwath;
		  while (Y < blockEnd)
		  {
			unsigned char * blockRowEnd = Y + blockRowWidth;
			while (Y < blockRowEnd)
			{
			  unsigned int rgba = sourceFormat->getRGBA (source);
			  source += sourceDepth;
			  int sr =  rgba             >> 24;  // assumes 32-bit int
			  int sg = (rgba & 0xFF0000) >> 16;
			  int sb = (rgba &   0xFF00) >>  8;
			  r += sr;
			  g += sg;
			  b += sb;
			  *Y++ = min (max (0x4C84 * sr + 0x962B * sg + 0x1D4F * sb + 0x8000, 0), 0xFFFFFF) >> 16;
			}
			Y += toBlockRowStep;
			source += fromBlockRowStep;
		  }
		  *U++ = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
		  *V++ = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
		  Y += toBlockStep;
		  source += fromBlockStep;
		}
		Y += toStep;
		source += fromStep;
		U += step12;
		V += step12;
	  }
	}
	else
	{
	  int y = 0;
	  unsigned char * end = Y + result.width * result.height;
	  while (Y < end)
	  {
		int x = 0;
		unsigned char * rowEnd = Y + rowWidth;
		while (Y < rowEnd)
		{
		  int r = 0;
		  int g = 0;
		  int b = 0;
		  unsigned char * blockEnd = Y + blockSwath;
		  while (Y < blockEnd)
		  {
			unsigned char * blockRowEnd = Y + blockRowWidth;
			while (Y < blockRowEnd)
			{
			  unsigned int rgba = sourceFormat->getRGBA (sourceBuffer->pixel (x++, y));
			  int sr =  rgba             >> 24;
			  int sg = (rgba & 0xFF0000) >> 16;
			  int sb = (rgba &   0xFF00) >>  8;
			  r += sr;
			  g += sg;
			  b += sb;
			  *Y++ = min (max (0x4C84 * sr + 0x962B * sg + 0x1D4F * sb + 0x8000, 0), 0xFFFFFF) >> 16;
			}
			Y += toBlockRowStep;
			x -= ratioH;
			y++;
		  }
		  *U++ = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
		  *V++ = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
		  Y += toBlockStep;
		  x += ratioH;
		  y -= ratioV;
		}
		Y += toStep;
		y += ratioV;
		U += step12;
		V += step12;
	  }
	}
  }
  else
  {
	PixelBuffer * destBuffer = (PixelBuffer *) result.buffer;

	if (i)
	{
	  unsigned char * source = (unsigned char *) i->memory;

	  rowWidth      *= sourceDepth;
	  blockRowWidth *= sourceDepth;
	  const int blockSwath       = (i->stride * ratioV              ) * sourceDepth;
	  const int fromStep         = (i->stride * ratioV - image.width) * sourceDepth;
	  const int fromBlockStep    = (ratioH - i->stride * ratioV     ) * sourceDepth;
	  const int fromBlockRowStep = (i->stride - ratioH              ) * sourceDepth;

	  int y = 0;
	  unsigned char * end = source + i->stride * image.height * sourceDepth;
	  while (source < end)
	  {
		int x = 0;
		unsigned char * rowEnd = source + rowWidth;
		while (source < rowEnd)
		{
		  int r = 0;
		  int g = 0;
		  int b = 0;
		  unsigned char * blockEnd = source + blockSwath;
		  while (source < blockEnd)
		  {
			unsigned char * blockRowEnd = source + blockRowWidth;
			while (source < blockRowEnd)
			{
			  unsigned int rgba = sourceFormat->getRGBA (source);
			  source += sourceDepth;
			  int sr =  rgba             >> 24;  // assumes 32-bit int
			  int sg = (rgba & 0xFF0000) >> 16;
			  int sb = (rgba &   0xFF00) >>  8;
			  r += sr;
			  g += sg;
			  b += sb;
			  *((unsigned char **) destBuffer->pixel (x++, y))[0] = min (max (0x4C84 * sr + 0x962B * sg + 0x1D4F * sb + 0x8000, 0), 0xFFFFFF) >> 16;
			}
			source += fromBlockRowStep;
			x -= ratioH;
			y++;
		  }
		  y -= ratioV;
		  unsigned char ** pixel = (unsigned char **) destBuffer->pixel (x, y);
		  *pixel[1] = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
		  *pixel[2] = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
		  source += fromBlockStep;
		  x += ratioH;
		}
		source += fromStep;
		y += ratioV;
	  }
	}
	else
	{
	  for (int y = 0; y < result.height; y += ratioV)
	  {
		for (int x = 0; x < result.width; x += ratioH)
		{
		  int r = 0;
		  int g = 0;
		  int b = 0;
		  int yend = y + ratioV;
		  int xend = x + ratioH;
		  for (int yy = y; yy < yend; yy++)
		  {
			for (int xx = x; xx < xend; xx++)
			{
			  unsigned int rgba = sourceFormat->getRGBA (sourceBuffer->pixel (xx, yy));
			  int sr =  rgba             >> 24;
			  int sg = (rgba & 0xFF0000) >> 16;
			  int sb = (rgba &   0xFF00) >>  8;
			  r += sr;
			  g += sg;
			  b += sb;
			  *((unsigned char **) destBuffer->pixel (xx, yy))[0] = min (max (0x4C84 * sr + 0x962B * sg + 0x1D4F * sb + 0x8000, 0), 0xFFFFFF) >> 16;
			}
		  }
		  unsigned char ** pixel = (unsigned char **) destBuffer->pixel (x, y);
		  *pixel[1] = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
		  *pixel[2] = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
		}
	  }
	}
  }
}

bool
PixelFormatPlanarYUV::operator == (const PixelFormat & that) const
{
  const PixelFormatPlanarYUV * p = dynamic_cast<const PixelFormatPlanarYUV *> (&that);
  return    p
         && ratioH == p->ratioH
         && ratioV == p->ratioV;
}

unsigned int
PixelFormatPlanarYUV::getRGBA (void * pixel) const
{
  int y = *((unsigned char **) pixel)[0] << 16;
  int u = *((unsigned char **) pixel)[1] - 128;
  int v = *((unsigned char **) pixel)[2] - 128;

  unsigned int r = min (max (y               + 0x166F7 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u               + 0x8000, 0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatPlanarYUV::getYUV (void * pixel) const
{
  return (*((unsigned char **) pixel)[0] << 16) | (*((unsigned char **) pixel)[1] << 8) | *((unsigned char **) pixel)[2];
}

unsigned char
PixelFormatPlanarYUV::getGray (void * pixel) const
{
  return *((unsigned char **) pixel)[0];
}

void
PixelFormatPlanarYUV::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  *((unsigned char **) pixel)[0] = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
  *((unsigned char **) pixel)[1] = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
  *((unsigned char **) pixel)[2] = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
}

void
PixelFormatPlanarYUV::setYUV  (void * pixel, unsigned int yuv) const
{
  *((unsigned char **) pixel)[0] =  yuv           >> 16;
  *((unsigned char **) pixel)[1] = (yuv & 0xFF00) >>  8;
  *((unsigned char **) pixel)[2] =  yuv &   0xFF;
}


// class PixelFormatUYYVYY ----------------------------------------------------

PixelFormatUYYVYY::PixelFormatUYYVYY ()
: PixelFormatPlanarYUV (4, 1)
{
}

PixelBuffer *
PixelFormatUYYVYY::buffer () const
{
  return new PixelBufferUYYVYY;
}


// class PixelFormatPlanarYCbCr -----------------------------------------------

unsigned char * PixelFormatPlanarYCbCr::lutYin = PixelFormatPlanarYCbCr::buildAll ();
unsigned char * PixelFormatPlanarYCbCr::lutUVin;
unsigned char * PixelFormatPlanarYCbCr::lutYout;
unsigned char * PixelFormatPlanarYCbCr::lutUVout;
float *         PixelFormatPlanarYCbCr::lutGrayOut;

PixelFormatPlanarYCbCr::PixelFormatPlanarYCbCr (int ratioH, int ratioV)
: PixelFormatYUV (ratioH, ratioV)
{
  planes     = 3;
  depth      = sizeof (char);
  precedence = 1;  // same as UYVY
  monochrome = false;
  hasAlpha   = false;
}

unsigned char *
PixelFormatPlanarYCbCr::buildAll ()
{
  lutYin     = (unsigned char *) malloc (256);
  lutUVin    = (unsigned char *) malloc (256);
  lutYout    = (unsigned char *) malloc (256);
  lutUVout   = (unsigned char *) malloc (256);
  lutGrayOut = (float *)         malloc (256 * sizeof (float));

  for (int i = 0; i < 256; i++)
  {
	lutYin[i]   = (int) rint (i * 219.0 / 255.0) + 16;
	lutUVin[i]  = (int) rint (i * 224.0 / 255.0) + 16;
	lutYout[i]  = min (max ((int) rint ((i - 16) * 255.0 / 219.0), 0), 255);
	lutUVout[i] = min (max ((int) rint ((i - 16) * 255.0 / 224.0), 0), 255);

	double f = (i - 16) / 219.0;
	if (f <= 0.04045)  // This linear portion extends into the negative numbers
	{
	  f /= 12.92;
	}
	else
	{
	  f = pow ((f + 0.055) / 1.055, 2.4);
	}
	lutGrayOut[i] = f;
  }
  return lutYin;
}

void
PixelFormatPlanarYCbCr::fromAny (const Image & image, Image & result) const
{
  assert (image.width % ratioH == 0  &&  image.height % ratioV == 0);

  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPlanar * o = (PixelBufferPlanar *) result.buffer;
  assert (o);
  unsigned char * Y  = (unsigned char *) o->plane0;
  unsigned char * Cb = (unsigned char *) o->plane1;
  unsigned char * Cr = (unsigned char *) o->plane2;

  const int rowWidth       = result.width;
  const int blockRowWidth  = ratioH;
  const int blockSwath     = o->stride0 * ratioV;
  const int step12         = o->stride12 - result.width / ratioH;
  const int toStep         = o->stride0 * ratioV - result.width;
  const int toBlockStep    = ratioH - o->stride0 * ratioV;
  const int toBlockRowStep = o->stride0 - ratioH;

  const unsigned int shift = 16 + (int) rint (log (ratioH * ratioV) / log (2));
  const int bias = 0x808 << (shift - 4);

  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	const int sourceDepth = sourceFormat->depth;

	const int fromStep         = (i->stride * ratioV - image.width) * sourceDepth;
	const int fromBlockStep    = (ratioH - i->stride * ratioV     ) * sourceDepth;
	const int fromBlockRowStep = (i->stride - ratioH              ) * sourceDepth;

	unsigned char * end = Y + result.width * result.height;
	while (Y < end)
	{
	  unsigned char * rowEnd = Y + rowWidth;
	  while (Y < rowEnd)
	  {
		int r = 0;
		int g = 0;
		int b = 0;
		unsigned char * blockEnd = Y + blockSwath;
		while (Y < blockEnd)
		{
		  unsigned char * blockRowEnd = Y + blockRowWidth;
		  while (Y < blockRowEnd)
		  {
			unsigned int rgba = sourceFormat->getRGBA (source);
			source += sourceDepth;
			int sr =  rgba             >> 24;  // assumes 32-bit int
			int sg = (rgba & 0xFF0000) >> 16;
			int sb = (rgba &   0xFF00) >>  8;
			r += sr;
			g += sg;
			b += sb;
			*Y++ = (0x41BD * sr + 0x810F * sg + 0x1910 * sb + 0x100000 + 0x8000) >> 16;
		  }
		  Y += toBlockRowStep;
		  source += fromBlockRowStep;
		}
		*Cb++ = (- 0x25F2 * r - 0x4A7E * g + 0x7070 * b + bias) >> shift;
		*Cr++ = (  0x7070 * r - 0x5E28 * g - 0x1248 * b + bias) >> shift;
		Y += toBlockStep;
		source += fromBlockStep;
	  }
	  Y += toStep;
	  source += fromStep;
	  Cb += step12;
	  Cr += step12;
	}
  }
  else
  {
	int y = 0;
	unsigned char * end = Y + result.width * result.height;
	while (Y < end)
	{
	  int x = 0;
	  unsigned char * rowEnd = Y + rowWidth;
	  while (Y < rowEnd)
	  {
		int r = 0;
		int g = 0;
		int b = 0;
		unsigned char * blockEnd = Y + blockSwath;
		while (Y < blockEnd)
		{
		  unsigned char * blockRowEnd = Y + blockRowWidth;
		  while (Y < blockRowEnd)
		  {
			unsigned int rgba = sourceFormat->getRGBA (image.buffer->pixel (x++, y));
			int sr =  rgba             >> 24;  // assumes 32-bit int
			int sg = (rgba & 0xFF0000) >> 16;
			int sb = (rgba &   0xFF00) >>  8;
			r += sr;
			g += sg;
			b += sb;
			*Y++ = (0x41BD * sr + 0x810F * sg + 0x1910 * sb + 0x100000 + 0x8000) >> 16;
		  }
		  Y += toBlockRowStep;
		  x -= ratioH;
		  y++;
		}
		*Cb++ = (- 0x25F2 * r - 0x4A7E * g + 0x7070 * b + bias) >> shift;
		*Cr++ = (  0x7070 * r - 0x5E28 * g - 0x1248 * b + bias) >> shift;
		Y += toBlockStep;
		x += ratioH;
		y -= ratioV;
	  }
	  Y += toStep;
	  y += ratioV;
	  Cb += step12;
	  Cr += step12;
	}
  }
}

bool
PixelFormatPlanarYCbCr::operator == (const PixelFormat & that) const
{
  const PixelFormatPlanarYCbCr * p = dynamic_cast<const PixelFormatPlanarYCbCr *> (&that);
  return    p
         && ratioH == p->ratioH
         && ratioV == p->ratioV;
}

unsigned int
PixelFormatPlanarYCbCr::getRGBA (void * pixel) const
{
  // Converts from YCbCr using the matrix given in the Poynton color FAQ:
  // [R]    1  [298.082    0      408.583]   [Y  -  16]
  // [G] = --- [298.082 -100.291 -208.120] * [Cb - 128]
  // [B]   256 [298.082  516.411    0    ]   [Cr - 128]
  // These values are for fixed-point after bit 8, but in this code they are
  // adjusted to put the fixed-point after bit 16.

  int y = (*((unsigned char **) pixel)[0] -  16) * 0x12A15;
  int u =  *((unsigned char **) pixel)[1] - 128;
  int v =  *((unsigned char **) pixel)[2] - 128;

  unsigned int r = min (max (y               + 0x19895 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x644A * u -  0xD01F * v + 0x8000, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x20469 * u               + 0x8000, 0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

/**
   This function does not provide direct access to the scaled values stored
   in memory, but instead rescales them to standard [0,255] range values.
 **/
unsigned int
PixelFormatPlanarYCbCr::getYUV (void * pixel) const
{
  return   (lutYout [*((unsigned char **) pixel)[0]] << 16)
	     | (lutUVout[*((unsigned char **) pixel)[1]] <<  8)
	     |  lutUVout[*((unsigned char **) pixel)[2]];
}

/**
   This function can return values outside of [0,1] if pixels are blacker
   than black or whiter than white.
 **/
void
PixelFormatPlanarYCbCr::getGray (void * pixel, float & gray) const
{
  gray = lutGrayOut[*((unsigned char **) pixel)[0]];
}

void
PixelFormatPlanarYCbCr::setRGBA (void * pixel, unsigned int rgba) const
{
  // Converts to YCbCr using the matrix given in the Poynton color FAQ:
  // [Y ]    1  [ 65.738  129.057   25.064]   [R]   [ 16]
  // [Cb] = --- [-37.945  -74.494  112.439] * [G] + [128]
  // [Cr]   256 [112.439  -94.154  -18.285]   [B]   [128]
  // These values are for fixed-point after bit 8, but in this code they
  // are adjusted to put the fixed-point after bit 16.  The hex values
  // below have been carefully adjusted so the Cb and Cr rows sum to zero
  // and the Y row sums to 219 * (256/255).  Note that clamping is unecessary
  // because of the footroom and headroom in the resulting values.

  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  *((unsigned char **) pixel)[0] = (  0x41BD * r + 0x810F * g + 0x1910 * b + 0x100000 + 0x8000) >> 16;
  *((unsigned char **) pixel)[1] = (- 0x25F2 * r - 0x4A7E * g + 0x7070 * b + 0x800000 + 0x8000) >> 16;
  *((unsigned char **) pixel)[2] = (  0x7070 * r - 0x5E28 * g - 0x1248 * b + 0x800000 + 0x8000) >> 16;
}

/**
   This function does not directly set the values stored in memory, but
   instead rescales them to their shortened ranges.
 **/
void
PixelFormatPlanarYCbCr::setYUV  (void * pixel, unsigned int yuv) const
{
  *((unsigned char **) pixel)[0] = lutYin [ yuv           >> 16];
  *((unsigned char **) pixel)[1] = lutUVin[(yuv & 0xFF00) >>  8];
  *((unsigned char **) pixel)[2] = lutUVin[ yuv &   0xFF       ];
}

/**
   This function can set values outside of [0,1] if pixels are blacker
   than black or whiter than white.
 **/
void
PixelFormatPlanarYCbCr::setGray (void * pixel, float gray) const
{
  // de-linearize
  if (gray <= 0.0031308f)
  {
	gray *= 12.92f;
  }
  else
  {
	gray = 1.055f * powf (gray, 1.0f / 2.4f) - 0.055f;
  }

  *((unsigned char **) pixel)[0] = (unsigned char) min (max (gray * 219.0f + 16.0f, 1.0f), 254.0f);
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
  unsigned int r = lutFloat2Char[(unsigned short) (65535 * rgbaValues[0])];
  unsigned int g = lutFloat2Char[(unsigned short) (65535 * rgbaValues[1])];
  unsigned int b = lutFloat2Char[(unsigned short) (65535 * rgbaValues[2])];
  unsigned int a = (unsigned int) (255 * rgbaValues[3]);
  return (r << 24) | (g << 16) | (b << 8) | a;
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
  rgbaValues[0] = lutChar2Float[ rgba             >> 24];
  rgbaValues[1] = lutChar2Float[(rgba & 0xFF0000) >> 16];
  rgbaValues[2] = lutChar2Float[(rgba &   0xFF00) >>  8];
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
