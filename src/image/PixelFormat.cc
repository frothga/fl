/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.8, 1.9, 1.11 thru 1.22 Copyright 2005 Sandia Corporation.
Revisions 1.24 thru 1.42           Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.42  2007/08/15 03:48:05  Fred
Move PixelFormat::buffer() to correct location and handle more cases.

Add default attach method for packed buffers.  Override attach() methods for
select formats.

Revision 1.41  2007/08/13 03:06:51  Fred
Replace PixelFormatUYVY, PixelFormatYUYV, and PixelFormatUYV with
PixelFormatPackedYUV.  This new format handles all possible packed YUV
layouts using a lookup table to specify the channel locations within a
macropixel.  Added UYVYUYVYYYYY as an exercise (since it is unikely that
modern cameras will generate such a bizarre format).

Revision 1.40  2007/08/02 12:34:23  Fred
Use PixelBufferYUYV rather than address-sensitive accessors in PixelFormatYUYV
PixelFormatUYVY.

Rearrange code and place comments about YUV <-> RGB conversion at top of file.

Revision 1.39  2007/08/01 03:45:03  Fred
Change the meaning of PixelBuffer*::stride from pixels to bytes.  Various
implied changes needed to allow code to work with this convention, particularly
scaling pixel pointers while incrementing them.

Specifically check for BYTE_ORDER == BIG_ENDIAN, and trap is BYTE_ORDER is not
one of BIG_ENDIAN or LITTLE_ENDIAN.

Trap empty images in filter().

Change PixelFormat::depth to a float.  This allows us to represent things like
IF09 (4CC) which effectively has 9.5 bits per pixel.  The role of depth has
changed now that stide is in bytes.  Will probably change further, and come to
mean an amount by which to multiply width*height to get the number of bytes
required to store one complete image.

Add PixelFormatGrayBits, which stores monochrome pixels with fewer than 8 bits.
Useful for handling certain TIFF files created by scanners.

Make some one-time calculated values const.

Use sizeof() and magic numbers for certain pixel sizes rather than referring
to depth.

Write endian-specific conversion routines for 3-byte pixels, in order to avoid
overrunning the buffer.  These routines handle the first or last pixel
(depending on endian) as a special case.  BIG_ENDIAN is untested.

Revision 1.38  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.37  2006/04/08 15:16:08  Fred
Strip "Char" from all YUV-type formats.  It is ugly, and doesn't add any
information in this case.

Revision 1.36  2006/04/08 14:46:16  Fred
Add PixelFormatUYYVYY.  Fix deficiency in PlanarYUV::fromAny(): it now handles
converting to a non-planar buffer, needed by UYYVYY.

Guard PlanarYCbCr against similar deficiency, but don't fix for now, because no
obvious need for it.

Revision 1.35  2006/04/06 03:48:42  Fred
Count bits in each color mask and include that info as metadata on RGBABits. 
Add function "dublicate" which echoes the bits in a channel in order to fill
out a wider channel.  Use to fill out alpha channel in RGBABits accessors. 
Should use it for all converters as well, but that will slow them down.

Revision 1.34  2006/04/05 02:40:57  Fred
Handle stride != width.

Supply missing alpha value in RGBABits conversions if source alphaMask == 0.

Revision 1.33  2006/04/02 03:05:03  Fred
Add PlanarYCbCr class, and use it instead of generic PlanarYUV for YUV420 and
YUV411.  This allows proper conversion of the excursions for video.

Change PixelFormatPlanar into PixelFormatYUV and use it to store ratioH and
ratioV for all YUV classes.  Move job of selecting PixelBuffer type back to
PixelFormat::buffer(), and base the selection on the number of planes.

Add inner loop to some converters to handle the difference between stride and
image width.  Some converters still need this change.

Add 0.5 to the result of all fixed-point arithmetic to get the same effect as
rint().  This produces more consistent results, which the test program needs.

Change PixelFormat::build* routines to use double rather than float.  Not
really an important change, but it costs nothing to use the extra precision. 
Maybe it would be possible to get rid of the linear hack and just calculate the
gamma function across the entire range.

Fix GrayFloat::fromGrayShort() and RGBChar::fromGrayShort() to use lut.

Fix GrayShort::fromAny() to use linear getGray() accessor.

Get rid of alpha channel in RGBABits::fromGrayFloat() and fromGrayDouble().

Add UYVY::fromAny(), YUYV::fromAny() and PlanarYUV::fromAny() to average RGB
values when computing UV components.

Add PlanarYUVChar::operator== to compare ratioH and ratioV.

Revision 1.32  2006/03/20 05:39:25  Fred
Add PixelFormatPlanarYUV.  Broaden PixelFormat::fromAny() to handle outputting
to a non-packed buffer.

Add PixelFormat::buffer() to create an appropriate PixelBuffer for each
PixelFormat.

Revision 1.31  2006/03/18 14:25:55  Fred
Use look up tables to convert gamma, rather than functions.  Hopefully this
will make the process more efficient.

Changed 16 bit formats (GrayShort, RGBShort) to use gamma=1, on the assumption
that the larger number of bits makes them more suitable for linear data,
similar to float formats.  Not sure if images in the wild will satisfy this
assumption.  Need more research.  Plan to add dynamically built luts for these
formats at some point, so one can specify the gamma for the data.

Revision 1.30  2006/03/13 03:50:19  Fred
Remove comment from RGBABits::fromGrayChar() on the assumption that the alpha
problem is now fixed.

Revision 1.29  2006/03/13 03:24:06  Fred
Add more fromGrayShort() functions.  Fix input size in
RGBABits::fromGrayShort().

Suppress alpha channel in RGBABits::fromGrayChar() and fromGrayShort().

Fix sign on grayShift in RGBChar::fromGrayShort().

Experimet with RCS log to replace manually maintained revision history at head
of file.

Revision 1.28  2006/03/13 02:43:04  Fred
Modify PixelFormatGrayShort to keep a mask of how many bits it is actually
using, rather than just assuming all bits are significant.  This is mainly to
support NITF images, where 16 bit gray values may contain less than 16
significant bits.

Add PixelFormatGrayChar::fromGrayShort().

Revision 1.27  2006/03/12 03:22:27  Fred
Move endian code to endian.h.  Don't use double underscores in front of
endian-related macros.

Revision 1.26  2006/02/25 22:38:32  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.25  2006/01/06 18:04:22  Fred
Move PixelFormat classes into separate file.  Actually, PixelFormat is much
more important than Pixel.  The main reason for the move is symmetry with a
soon-to-be-added set of classes called PixelBuffer.

Revision 1.24  2005/10/15 21:37:41  Fred
In YUV formats, cast pixel to long rather than int when testing for eveness. 
long is more likely than int to match the size of a pointer.  This hack should
be enough to work on 64-bit systems.

Revision 1.23  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.22  2005/10/09 05:30:53  Fred
Update revision history and add Sandia copyright notice.

Revision 1.21  2005/05/29 15:57:58  Fred
Pixel class: Remove const claim from setters.  Add get/setAlpha().

Revision 1.20  2005/05/03 04:35:39  Fred
Add UYVChar and BGRChar formats.

Revision 1.19  2005/05/03 03:48:43  Fred
Add MSVC variant of bswap function.

Revision 1.18  2005/05/02 05:23:05  Fred
Minimize number of increment operations on pixel pointers.  Instead use
relative offset, which should compile to smaller machine code.

Revision 1.17  2005/05/02 04:15:02  Fred
Add converter fromRGBChar to GrayFloat and GrayDouble.

Revision 1.16  2005/05/02 03:58:28  Fred
Make RGBAChar and RGBChar subclasses of RGBABits.

Revision 1.15  2005/05/02 01:23:15  Fred
Place RGBABits ahead of RGBAChar and RGBChar in the file.  No functional
impact, just placing things in the same order they will appear in image.h when
RGBAChar and RGBChar are subclasses of RGBABits.

Revision 1.14  2005/05/02 00:17:54  Fred
More rigorous naming:  Renamed YVYUChar->UYVYChar and VYUYChar->YUYVChar to
follow memory order convention.  Changed implementation of RGBAChar to follow
memory order convention.

Changed get/setRGBA() to guarantee an order within machine word (which is of
course invariant to endianness).

With the above two changes and a few other small tweaks to the code, it should
now work for big-endian as well as small-endian, so removed endian warning. 
However, this hasn't been tested on an actual big-endian yet.

Added RGBChar.  This is the format used by all the image i/o libraries that I
currently link to.

Still to do:  make RGBAChar and RGBChar subclasses of RGBABits.  Need support
for direct conversion from RGBChar to GrayFloat and GrayDouble so that they use
proper linear Y conversion.  Finally, some code is using Int2Char when it could
load a word directly, and some code is incrementing pointers more than
necessary.

Revision 1.13  2005/04/26 03:48:56  Fred
Remove alpha channel from PixelFormat::get/setXYZ(float[]) because this is
inconsistent with the naming convention and with the behavior of the more
specialized PixelFormat classes.

Revision 1.12  2005/04/25 02:19:41  Fred
Add PixelFormatGrayShort to support TIFF.  Rearranged precendence values.

Add get/setAlpha() to PixelFormatRGBAShort.  All formats with alpha channels
should implement these methods since the default implementation assumes no
alpha channel.  Restored default implementation in PixelFormat and strengthened
comments there to explain the no-alpha assumption.

Revision 1.11  2005/04/23 20:52:03  Fred
Add PixelFormats RGBChar, RGBShort and RGBAShort to accomodate TIFF.  Reorder
precedence to insert RBG*Short formats between GrayFloat and GrayDouble.

Revision 1.10  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.9  2005/01/22 21:22:32  Fred
MSVC compilability fix:  Use fl/math.h.  Work around endian detection.  Work
around lvalue complaint.

Revision 1.8  2005/01/12 05:11:16  rothgang
cygwin compilation fix: Use sys/param.h rather than endian.h.

Revision 1.7  2004/03/22 19:24:15  rothgang
Fix bug in PixelFormatHLSFloat::setRGBA() handling hue.  Rearrange code
slightly and pre-compute more constants in hopes of being more efficient.

Revision 1.6  2004/03/10 01:47:12  rothgang
Change pointers to references in getGray(float).  Change pointers to values in
setGray(float).

Revision 1.5  2004/01/08 21:25:12  rothgang
Getters and setters for gray and alpha.

Revision 1.4  2003/12/30 16:11:59  rothgang
Add HLS color space.  Add YUV accessors.

Revision 1.3  2003/07/09 15:46:26  rothgang
Add YUV color space.  Make individual formats more responsible for selecting
conversion.

Revision 1.2  2003/07/08 23:47:57  rothgang
Added YUV.  Added operations.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/image.h"
#include "fl/pi.h"
#include "fl/math.h"
#include "fl/endian.h"

#include <algorithm>
#include <set>


// Include for tracing
//#include <iostream>


using namespace std;
using namespace fl;


// Tables for packed YUV formats

PixelFormatPackedYUV::YUVindex tableUYVY[] =
{
  {1, 0, 2},
  {3, 0, 2},
  {-1}
};

PixelFormatPackedYUV::YUVindex tableYUYV[] =
{
  {0, 1, 3},
  {2, 1, 3},
  {-1}
};

PixelFormatPackedYUV::YUVindex tableUYV[] =
{
  {1, 0, 2},
  {-1}
};

PixelFormatPackedYUV::YUVindex tableUYYVYY[] =
{
  {1, 0, 3},
  {2, 0, 3},
  {4, 0, 3},
  {5, 0, 3},
  {-1}
};

PixelFormatPackedYUV::YUVindex tableUYVYUYVYYYYY[] =
{
  {1,  0, 2},
  {3,  0, 2},
  {5,  0, 2},
  {7,  0, 2},
  {8,  4, 6},
  {9,  4, 6},
  {10, 4, 6},
  {11, 4, 6},
  {-1}
};


PixelFormatGrayChar           fl::GrayChar;
PixelFormatGrayShort          fl::GrayShort;
PixelFormatGrayFloat          fl::GrayFloat;
PixelFormatGrayDouble         fl::GrayDouble;
PixelFormatRGBAChar           fl::RGBAChar;
PixelFormatRGBAShort          fl::RGBAShort;
PixelFormatRGBAFloat          fl::RGBAFloat;
PixelFormatRGBChar            fl::RGBChar;
PixelFormatRGBShort           fl::RGBShort;
PixelFormatPackedYUV          fl::UYVY         (tableUYVY);
PixelFormatPackedYUV          fl::YUYV         (tableYUYV);
PixelFormatPackedYUV          fl::UYV          (tableUYV);
PixelFormatPackedYUV          fl::UYYVYY       (tableUYYVYY);
PixelFormatPackedYUV          fl::UYVYUYVYYYYY (tableUYVYUYVYYYYY);
PixelFormatPlanarYCbCr        fl::YUV420 (2, 2);
PixelFormatPlanarYCbCr        fl::YUV411 (4, 1);
PixelFormatHLSFloat           fl::HLSFloat;

// These "bits" formats must be endian independent.
#if BYTE_ORDER == LITTLE_ENDIAN
PixelFormatRGBABits   fl::BGRChar (3, 0xFF0000, 0xFF00, 0xFF, 0x0);
#elif BYTE_ORDER == BIG_ENDIAN
PixelFormatRGBABits   fl::BGRChar (3, 0xFF, 0xFF00, 0xFF0000, 0x0);
#else
// This traps unhandled endian types for the entire file, not just the "bits"
// formats.  The other BYTE_ORDER tests in this file can safely skip this case.
#  error This endian is not currently handled.
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

// YUV <-> RGB conversion matrices are specified by the standards in terms of
// non-linear RGB.  IE: even though the conversion matrices are linear ops,
// they work on non-linear inputs.  Therefore, even though YUV is essentially
// non-linear, it should not be linearized until after it is converted into
// RGB.  The matrices output non-linear RGB.

// R = Y           +1.4022*V
// G = Y -0.3456*U -0.7145*V
// B = Y +1.7710*U

// Y =  0.2989*R +0.5866*G +0.1145*B
// U = -0.1687*R -0.3312*G +0.5000*B
// V =  0.5000*R -0.4183*G -0.0816*B


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
#   elif BYTE_ORDER == BIG_ENDIAN
	unsigned char barf;
	unsigned char piece0;
	unsigned char piece1;
	unsigned char piece2;
#   endif
  };
};


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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
  const PixelFormat * sourceFormat = image.format;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPacked * o = (PixelBufferPacked *) result.buffer;
  const int destDepth = (int) depth;
  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	const int sourceDepth = (int) sourceFormat->depth;
	if (o)
	{
	  unsigned char * dest   = (unsigned char *) o->memory;
	  unsigned char * end    = dest + o->stride * result.height;
	  unsigned char * rowEnd = dest + result.width * destDepth;
	  const int step = i->stride - image.width * sourceDepth;
	  while (dest < end)
	  {
		while (dest < rowEnd)
		{
		  setRGBA (dest, sourceFormat->getRGBA (source));
		  source += sourceDepth;
		  dest += destDepth;
		}
		source += step;
		rowEnd += o->stride;
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
		  dest += destDepth;
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

PixelBuffer *
PixelFormat::buffer () const
{
  if (planes == 1)
  {
	return new PixelBufferPacked;
  }
  else if (planes > 1)
  {
	return new PixelBufferPlanar;
  }
  else if (planes == -1)
  {
	return new PixelBufferGroups (1, 1);
  }
  else
  {
	throw "Need to override default PixelFormat::buffer()";
  }
}

PixelBuffer *
PixelFormat::attach (void * block, int width, int height, bool copy) const
{
  PixelBufferPacked * result = new PixelBufferPacked (block, width * (int) depth, height, (int) depth);
  if (copy) result->memory.copyFrom (block);
  return result;
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


// class PixelFormatGrayBits --------------------------------------------------

PixelFormatGrayBits::PixelFormatGrayBits (int bits, bool bigendian)
{
  planes     = -1;
  depth      = bits / 8.0f;
  precedence = 0;  // Below everything.  Should actually be lower than GrayChar, but plan to replace this ordering system anyway.
  monochrome = true;
  hasAlpha   = false;
  this->bits = bits;
  pixels     = 8 / bits;

  // build masks

  unsigned char mask = 0x1;
  for (int i = 1; i < bits; i++) mask |= mask << 1;

  int i = bigendian ? pixels - 1 : 0;
  int step = bigendian ? -1 : 1;
  int shift = 8 - bits;
  while (mask)
  {
	masks[i] = mask;
	shifts[i] = shift;
	mask << bits;
	shift -= bits;
	i += step;
  }
}

PixelBuffer *
PixelFormatGrayBits::attach (void * block, int width, int height, bool copy) const
{
  PixelBufferGroups * result = new PixelBufferGroups (block, (int) ceil (width / pixels), height, pixels, 1);
  if (copy) result->memory.copyFrom (block);
  return result;
}

unsigned int
PixelFormatGrayBits::getRGBA  (void * pixel) const
{
  PixelBufferGroups::PixelData * data = (PixelBufferGroups::PixelData *) pixel;

  unsigned int t = (*data->address & masks[data->index]) << shifts[data->index];
  for (int i = 1; i < pixels; i++) t |= t >> bits;  // dublicate

  return (t << 24) | (t << 16) | (t << 8) | 0xFF;
}

void
PixelFormatGrayBits::setRGBA  (void * pixel, unsigned int rgba) const
{
  unsigned int r = (rgba & 0xFF000000) >> 16;
  unsigned int g = (rgba &   0xFF0000) >>  8;
  unsigned int b =  rgba &     0xFF00;
  unsigned char t = ((r * redWeight + g * greenWeight + b * blueWeight) / totalWeight + 0x80) >> 8;

  PixelBufferGroups::PixelData * data = (PixelBufferGroups::PixelData *) pixel;
  unsigned char m = masks[data->index];
  *data->address = (*data->address & ~m) | ((t >> shifts[data->index]) & m);
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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
  const int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift;
  const int step = i->stride - image.width * sizeof (short);
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = lutFloat2Char[*fromPixel++ << grayShift];
	}
	fromPixel = (unsigned short *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * sizeof (float);
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  float p = min (max (*fromPixel++, 0.0f), 1.0f);
	  *toPixel++ = lutFloat2Char[(unsigned short) (65535 * p)];
	}
	fromPixel = (float *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * sizeof (double);
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  double p = min (max (*fromPixel++, 0.0), 1.0);
	  *toPixel++ = lutFloat2Char[(unsigned short) (65535 * p)];
	}
	fromPixel = (double *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * 4;
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
	const int step = i->stride - image.width * sizeof (fromSize); \
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
	  fromPixel = (unsigned fromSize *) ((char *) fromPixel + step); \
    } \
  }

  switch ((int) that->depth)
  {
    case 1:
	  RGBBits2GrayChar (char);
	  break;
    case 2:
	  RGBBits2GrayChar (short);
	  break;
    case 3:
	{
	  const int step = i->stride - image.width * 3;
#     if BYTE_ORDER == LITTLE_ENDIAN
	  unsigned char * fromPixel = (unsigned char *) i->memory + i->stride * image.height - step - 3;
	  unsigned char * begin     = (unsigned char *) o->memory;
	  unsigned char * toPixel   = begin + result.width * result.height;
	  unsigned char * rowBegin  = toPixel - result.width;
	  // process last pixel
	  toPixel--;
	  Int2Char t;
	  t.piece0 = fromPixel[0];
	  t.piece1 = fromPixel[1];
	  t.piece2 = fromPixel[2];
	  fromPixel -= 3;
	  unsigned int r = t.all & that->redMask;
	  unsigned int g = t.all & that->greenMask;
	  unsigned int b = t.all & that->blueMask;
	  *toPixel-- = ((  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight
					 + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight
					 + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight
					) / totalWeight + 0x80) >> 8;
	  // process other pixels
	  while (toPixel >= begin)
	  {
		while (toPixel >= rowBegin)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel -= 3;
		  r = t & that->redMask;
		  g = t & that->greenMask;
		  b = t & that->blueMask;
		  *toPixel-- = ((  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight
						 + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight
						 + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight
						) / totalWeight + 0x80) >> 8;
		}
		fromPixel -= step;
		rowBegin -= result.width;
	  }
#     elif BYTE_ORDER == BIG_ENDIAN
	  unsigned char * fromPixel = (unsigned char *) i->memory;
	  unsigned char * toPixel   = (unsigned char *) o->memory;
	  unsigned char * end       = toPixel + result.width * result.height;
	  unsigned char * rowEnd    = toPixel + result.width;
	  // process first pixel
	  Int2Char t;
	  t.piece0 = fromPixel[0];
	  t.piece1 = fromPixel[1];
	  t.piece2 = fromPixel[2];
	  fromPixel += 2;  // only 2, because big-endian pointer is set back by 1
	  unsigned int r = t.all & that->redMask;
	  unsigned int g = t.all & that->greenMask;
	  unsigned int b = t.all & that->blueMask;
	  *toPixel++ = ((  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight
					 + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight
					 + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight
					) / totalWeight + 0x80) >> 8;
	  // process other pixels
	  while (toPixel < end)
	  {
		while (toPixel < rowEnd)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel += 3;
		  r = t & that->redMask;
		  g = t & that->greenMask;
		  b = t & that->blueMask;
		  *toPixel++ = ((  (redShift   > 0 ? r << redShift   : r >> -redShift)   * redWeight
						 + (greenShift > 0 ? g << greenShift : g >> -greenShift) * greenWeight
						 + (blueShift  > 0 ? b << blueShift  : b >> -blueShift)  * blueWeight
						) / totalWeight + 0x80) >> 8;
		}
		fromPixel += step;
		rowEnd += result.width;
	  }
#     endif
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
  const int step = i->stride0 - image.width;
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
	const int sourceDepth = (int) sourceFormat->depth;
	const int step = i->stride - image.width * sourceDepth;
	unsigned char * end = dest + result.width * result.height;
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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
  const int step = i->stride - image.width;
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
  const int step = i->stride - image.width * sizeof (float);
  while (toPixel < end)
  {
	unsigned short * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  float p = min (max (*fromPixel++, 0.0f), 1.0f);
	  *toPixel++ = (unsigned short) (p * grayMask);
	}
	fromPixel = (float *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * sizeof (double);
  while (toPixel < end)
  {
	unsigned short * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  double p = min (max (*fromPixel++, 0.0), 1.0);
	  *toPixel++ = (unsigned short) (p * grayMask);
	}
	fromPixel = (double *) ((char *) fromPixel + step);
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
	const int sourceDepth = (int) sourceFormat->depth;
	const int step = i->stride - image.width * sourceDepth;
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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
  const int step = i->stride - image.width;
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
  const int step = i->stride - image.width * sizeof (short);
  const float grayMask = ((PixelFormatGrayShort *) image.format)->grayMask;
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel++ / grayMask;
	}
	fromPixel = (unsigned short *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * sizeof (double);
  while (toPixel < end)
  {
	float * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = (float) *fromPixel++;
	}
	fromPixel = (double *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * 4;
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
  const int step = i->stride - image.width * 3;
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
	const int step = i->stride - image.width * sizeof (imageSize); \
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
	  fromPixel = (unsigned imageSize *) ((char *) fromPixel + step); \
	} \
  }

  switch ((int) that->depth)
  {
    case 1:
	  RGBBits2GrayFloat (char);
	  break;
    case 2:
	  RGBBits2GrayFloat (short);
	  break;
    case 3:
	{
	  const int step = i->stride - image.width * 3;
#     if BYTE_ORDER == LITTLE_ENDIAN
	  unsigned char * fromPixel = (unsigned char *) i->memory + i->stride * image.height - step - 3;
	  float *         begin     = (float *)         o->memory;
	  float *         toPixel   = begin + result.width * result.height;
	  float *         rowBegin  = toPixel - result.width;
	  // Process last pixel
	  toPixel--;
	  Int2Char t;
	  t.piece0 = fromPixel[0];
	  t.piece1 = fromPixel[1];
	  t.piece2 = fromPixel[2];
	  fromPixel -= 3;
	  unsigned int r = t.all & that->redMask;
	  unsigned int g = t.all & that->greenMask;
	  unsigned int b = t.all & that->blueMask;
	  float fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
	  float fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
	  float fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
	  *toPixel-- = redToY * fr + greenToY * fg + blueToY * fb;
	  // Process other pixels
	  while (toPixel >= begin)
	  {
		while (toPixel >= rowBegin)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel -= 3;
		  r = t & that->redMask;
		  g = t & that->greenMask;
		  b = t & that->blueMask;
		  fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
		  fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
		  fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
		  *toPixel-- = redToY * fr + greenToY * fg + blueToY * fb;
		}
		fromPixel -= step;
		rowBegin -= result.width;
	  }
#     elif BYTE_ORDER == BIG_ENDIAN
	  unsigned char * fromPixel = (unsigned char *) i->memory;
	  float *         toPixel   = (float *)         o->memory;
	  float *         end       = toPixel + result.width * result.height;
	  float *         rowEnd    = toPixel + result.width;
	  // Process first pixel
	  Int2Char t;
	  t.piece0 = fromPixel[0];
	  t.piece1 = fromPixel[1];
	  t.piece2 = fromPixel[2];
	  fromPixel += 2;  // only 2, because big-endian pointer is set back by 1
	  unsigned int r = t.all & that->redMask;
	  unsigned int g = t.all & that->greenMask;
	  unsigned int b = t.all & that->blueMask;
	  float fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
	  float fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
	  float fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
	  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
	  // Process other pixels
	  while (toPixel < end)
	  {
		while (toPixel < rowEnd)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel += 3;
		  r = t & that->redMask;
		  g = t & that->greenMask;
		  b = t & that->blueMask;
		  fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
		  fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
		  fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
		  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
		}
		fromPixel += step;
		rowEnd += result.width;
	  }
#     endif
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
  const int step = i->stride0 - image.width;
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
	const int sourceDepth = (int) sourceFormat->depth;
	const int step = i->stride - image.width * sourceDepth;
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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
  const int step = i->stride - image.width;
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
  const int step = i->stride - image.width * sizeof (short);
  const double grayMask = ((PixelFormatGrayShort *) image.format)->grayMask;
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel++ / grayMask;
	}
	fromPixel = (unsigned short *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * sizeof (float);
  while (toPixel < end)
  {
	double * rowEnd = toPixel + result.width;
	while (toPixel < rowEnd)
	{
	  *toPixel++ = *fromPixel++;
	}
	fromPixel = (float *) ((char *) fromPixel + step);
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
  const int step = i->stride - image.width * 4;
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
  const int step = i->stride - image.width * 3;
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
	const int step = i->stride - image.width * sizeof (imageSize); \
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
	  fromPixel = (unsigned imageSize *) ((char *) fromPixel + step); \
	} \
  }

  switch ((int) that->depth)
  {
    case 1:
	  RGBBits2GrayDouble (char);
	  break;
    case 2:
	  RGBBits2GrayDouble (short);
	  break;
    case 3:
	{
	  const int step = i->stride - image.width * 3;
#     if BYTE_ORDER == LITTLE_ENDIAN
	  unsigned char * fromPixel = (unsigned char *) i->memory + i->stride * image.height - step - 3;
	  double *        begin     = (double *)        o->memory;
	  double *        toPixel   = begin + result.width * result.height;
	  double *        rowBegin  = toPixel - result.width;
	  // Process lasst pixel
	  toPixel--;
	  Int2Char t;
	  t.piece0 = fromPixel[0];
	  t.piece1 = fromPixel[1];
	  t.piece2 = fromPixel[2];
	  fromPixel -= 3;
	  unsigned int r = t.all & that->redMask;
	  unsigned int g = t.all & that->greenMask;
	  unsigned int b = t.all & that->blueMask;
	  double fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
	  double fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
	  double fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
	  *toPixel-- = redToY * fr + greenToY * fg + blueToY * fb;
	  // Process other pixels
	  while (toPixel >= begin)
	  {
		while (toPixel >= rowBegin)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel -= 3;
		  r = t & that->redMask;
		  g = t & that->greenMask;
		  b = t & that->blueMask;
		  fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
		  fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
		  fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
		  *toPixel-- = redToY * fr + greenToY * fg + blueToY * fb;
		}
		fromPixel -= step;
		rowBegin -= result.width;
	  }
#     elif BYTE_ORDER == BIG_ENDIAN
	  unsigned char * fromPixel = (unsigned char *) i->memory;
	  double *        toPixel   = (double *)        o->memory;
	  double *        end       = toPixel + result.width * result.height;
	  double *        rowEnd    = toPixel + result.width;
	  // Process first pixel
	  Int2Char t;
	  t.piece0 = fromPixel[0];
	  t.piece1 = fromPixel[1];
	  t.piece2 = fromPixel[2];
	  fromPixel += 2;  // only 2, because big-endian pointer is set back by 1
	  unsigned int r = t.all & that->redMask;
	  unsigned int g = t.all & that->greenMask;
	  unsigned int b = t.all & that->blueMask;
	  double fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
	  double fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
	  double fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
	  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
	  // Process other pixels
	  while (toPixel < end)
	  {
		while (toPixel < rowEnd)
		{
		  unsigned int t = * (unsigned int *) fromPixel;
		  fromPixel += 3;
		  r = t & that->redMask;
		  g = t & that->greenMask;
		  b = t & that->blueMask;
		  fr = lutChar2Float[redShift   > 0 ? r << redShift   : r >> -redShift];
		  fg = lutChar2Float[greenShift > 0 ? g << greenShift : g >> -greenShift];
		  fb = lutChar2Float[blueShift  > 0 ? b << blueShift  : b >> -blueShift];
		  *toPixel++ = redToY * fr + greenToY * fg + blueToY * fb;
		}
		fromPixel += step;
		rowEnd += result.width;
	  }
#     endif
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
  const int step = i->stride0 - image.width;
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
	const int sourceDepth = (int) sourceFormat->depth;
	const int step = i->stride - image.width * sourceDepth;
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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
	fromPixel = (unsigned fromSize *) ((char *) fromPixel + step); \
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
	fromPixel = (fromSize *) ((char *) fromPixel + step); \
  } \
}

// The following macros address pixels consisting of three bytes.  For
// efficiency's sake, there are versions specific to each endian.
#if BYTE_ORDER == LITTLE_ENDIAN

#define OddBits2Bits(toSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char *   fromPixel = (unsigned char *)   i->memory + i->stride * image.height - step - 3; \
  unsigned toSize * begin     = (unsigned toSize *) o->memory; \
  unsigned toSize * toPixel   = begin + result.width * result.height; \
  unsigned toSize * rowBegin  = toPixel - result.width; \
  toPixel--; \
  Int2Char t; \
  t.piece0 = fromPixel[0]; \
  t.piece1 = fromPixel[1]; \
  t.piece2 = fromPixel[2]; \
  fromPixel -= 3; \
  unsigned int r = t.all & fromRed; \
  unsigned int g = t.all & fromGreen; \
  unsigned int b = t.all & fromBlue; \
  unsigned int a = fromAlpha ? t.all & fromAlpha : 0xFFFFFFFF; \
  *toPixel-- =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
               | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
               | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
               | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  while (toPixel >= begin) \
  { \
	while (toPixel >= rowBegin) \
	{ \
	  unsigned int t = * (unsigned int *) fromPixel; \
	  fromPixel -= 3; \
	  r = t & fromRed; \
	  g = t & fromGreen; \
	  b = t & fromBlue; \
	  a = fromAlpha ? t & fromAlpha : 0xFFFFFFFF; \
	  *toPixel-- =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	               | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	               | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	               | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	} \
	fromPixel -= step; \
	rowBegin -= result.width; \
  } \
}

#define Bits2OddBits(fromSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned fromSize * fromPixel = (unsigned fromSize *) i->memory; \
  unsigned char *     toPixel   = (unsigned char *)     o->memory; \
  unsigned char *     end       = toPixel + o->stride * result.height - 3; \
  unsigned int r; \
  unsigned int g; \
  unsigned int b; \
  unsigned int a; \
  while (toPixel < end) \
  { \
	unsigned char * rowEnd = min (toPixel + o->stride, end); \
	while (toPixel < rowEnd) \
	{ \
	  r = *fromPixel & fromRed; \
	  g = *fromPixel & fromGreen; \
	  b = *fromPixel & fromBlue; \
	  a = fromAlpha ? *fromPixel & fromAlpha : 0xFFFFFFFF; \
	  fromPixel++; \
	  * (unsigned int *) toPixel =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		                           | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		                           | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		                           | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	  toPixel += 3; \
	} \
	fromPixel = (unsigned fromSize *) ((char *) fromPixel + step); \
  } \
  fromPixel = (unsigned fromSize *) ((char *) fromPixel - step); \
  r = *fromPixel & fromRed; \
  g = *fromPixel & fromGreen; \
  b = *fromPixel & fromBlue; \
  a = fromAlpha ? *fromPixel & fromAlpha : 0xFFFFFFFF; \
  Int2Char t; \
  t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	      | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	      | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	      | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  toPixel[0] = t.piece0; \
  toPixel[1] = t.piece1; \
  toPixel[2] = t.piece2; \
}

#define OddBits2OddBits(fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char * fromPixel = (unsigned char *) i->memory; \
  unsigned char * toPixel   = (unsigned char *) o->memory; \
  unsigned char * end       = toPixel + o->stride * result.height - 3; \
  unsigned int r; \
  unsigned int g; \
  unsigned int b; \
  unsigned int a; \
  while (toPixel < end) \
  { \
	unsigned char * rowEnd = min (toPixel + o->stride, end); \
	while (toPixel < rowEnd) \
	{ \
	  unsigned int t = * (unsigned int *) fromPixel; \
	  fromPixel += 3; \
	  r = t & fromRed; \
	  g = t & fromGreen; \
	  b = t & fromBlue; \
	  a = fromAlpha ? t & fromAlpha : 0xFFFFFFFF; \
	  * (unsigned int *) toPixel =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	                               | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	                               | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	                               | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	  toPixel += 3; \
	} \
	fromPixel += step; \
  } \
  fromPixel -= step; \
  Int2Char t; \
  t.piece0 = fromPixel[0]; \
  t.piece1 = fromPixel[1]; \
  t.piece2 = fromPixel[2]; \
  r = t.all & fromRed; \
  g = t.all & fromGreen; \
  b = t.all & fromBlue; \
  a = fromAlpha ? t.all & fromAlpha : 0xFFFFFFFF; \
  t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
          | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
          | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
          | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  toPixel[0] = t.piece0; \
  toPixel[1] = t.piece1; \
  toPixel[2] = t.piece2; \
}

#define GrayFloat2OddBits(fromSize) \
{ \
  fromSize *      fromPixel = (fromSize *)      i->memory; \
  unsigned char * toPixel   = (unsigned char *) o->memory; \
  unsigned char * end       = toPixel + o->stride * result.height - 3; \
  fromSize v; \
  while (toPixel < end) \
  { \
	unsigned char * rowEnd = min (toPixel + o->stride, end); \
	while (toPixel < rowEnd) \
	{ \
	  v = min (max (*fromPixel++, (fromSize) 0.0), (fromSize) 1.0); \
	  unsigned int t = lutFloat2Char[(unsigned short) (65535 * v)]; \
	  * (unsigned int *) toPixel =   ((redShift   > 0 ? t << redShift   : t >> -redShift)   & redMask) \
	                               | ((greenShift > 0 ? t << greenShift : t >> -greenShift) & greenMask) \
	                               | ((blueShift  > 0 ? t << blueShift  : t >> -blueShift)  & blueMask) \
	                               | alphaMask; \
	  toPixel += 3; \
	} \
	fromPixel = (fromSize *) ((char *) fromPixel + step); \
  } \
  fromPixel = (fromSize *) ((char *) fromPixel - step); \
  v = min (max (*fromPixel, (fromSize) 0.0), (fromSize) 1.0); \
  Int2Char t; \
  t.all = lutFloat2Char[(unsigned short) (65535 * v)]; \
  t.all =   ((redShift   > 0 ? t.all << redShift   : t.all >> -redShift)   & redMask) \
          | ((greenShift > 0 ? t.all << greenShift : t.all >> -greenShift) & greenMask) \
          | ((blueShift  > 0 ? t.all << blueShift  : t.all >> -blueShift)  & blueMask) \
          | alphaMask; \
  toPixel[0] = t.piece0; \
  toPixel[1] = t.piece1; \
  toPixel[2] = t.piece2; \
}

#elif BYTE_ORDER == BIG_ENDIAN

#define OddBits2Bits(toSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char *   fromPixel = (unsigned char *)   i->memory; \
  unsigned toSize * toPixel   = (unsigned toSize *) o->memory; \
  unsigned toSize * end       = toPixel + result.width * result.height; \
  unsigned toSize * rowEnd    = toPixel + result.width; \
  Int2Char t; \
  t.piece0 = fromPixel[0]; \
  t.piece1 = fromPixel[1]; \
  t.piece2 = fromPixel[2]; \
  fromPixel += 2; \
  unsigned int r = t.all & fromRed; \
  unsigned int g = t.all & fromGreen; \
  unsigned int b = t.all & fromBlue; \
  unsigned int a = fromAlpha ? t.all & fromAlpha : 0xFFFFFFFF; \
  *toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
               | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
               | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
               | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  while (toPixel < end) \
  { \
	while (toPixel < rowEnd) \
	{ \
	  unsigned int t = * (unsigned int *) fromPixel; \
	  fromPixel += 3; \
	  r = t & fromRed; \
	  g = t & fromGreen; \
	  b = t & fromBlue; \
	  a = fromAlpha ? t & fromAlpha : 0xFFFFFFFF; \
	  *toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	               | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	               | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	               | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	} \
	fromPixel += step; \
	rowEnd += result.width; \
  } \
}

#define Bits2OddBits(fromSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned fromSize * fromPixel = (unsigned fromSize *) ((char *) i->memory + i->stride * image.height - step) - 1; \
  unsigned char *     begin     = (unsigned char *) o->memory; \
  unsigned char *     toPixel   = begin + o->stride * result.height - 4; \
  unsigned int r; \
  unsigned int g; \
  unsigned int b; \
  unsigned int a; \
  while (toPixel > begin) \
  { \
	unsigned char * rowBegin = max (toPixel - o->stride, begin); \
	while (toPixel > rowBegin) \
	{ \
	  r = *fromPixel & fromRed; \
	  g = *fromPixel & fromGreen; \
	  b = *fromPixel & fromBlue; \
	  a = fromAlpha ? *fromPixel & fromAlpha : 0xFFFFFFFF; \
	  fromPixel--; \
	  * (unsigned int *) toPixel =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		                           | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		                           | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		                           | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	  toPixel -= 3; \
	} \
	fromPixel = (unsigned fromSize *) ((char *) fromSize + step); \
  } \
  fromPixel = (unsigned fromSize *) ((char *) fromSize - step); \
  toPixel++; \
  r = *fromPixel & fromRed; \
  g = *fromPixel & fromGreen; \
  b = *fromPixel & fromBlue; \
  a = fromAlpha ? *fromPixel & fromAlpha : 0xFFFFFFFF; \
  Int2Char t; \
  t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	      | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	      | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	      | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  toPixel[0] = t.piece0; \
  toPixel[1] = t.piece1; \
  toPixel[2] = t.piece2; \
}

#define OddBits2OddBits(fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char * fromPixel = (unsigned char *) i->memory + i->stride * image.height - step - 4; \
  unsigned char * begin     = (unsigned char *) o->memory; \
  unsigned char * toPixel   = begin + o->stride * result.height - 4; \
  unsigned int r; \
  unsigned int g; \
  unsigned int b; \
  unsigned int a; \
  while (toPixel > begin) \
  { \
	unsigned char * rowBegin = max (toPixel - o->stride, begin); \
	while (toPixel > rowBegin) \
	{ \
	  unsigned int t = * (unsigned int *) fromPixel; \
	  fromPixel -= 3; \
	  r = t & fromRed; \
	  g = t & fromGreen; \
	  b = t & fromBlue; \
	  a = fromAlpha ? t & fromAlpha : 0xFFFFFFFF; \
	  * (unsigned int *) toPixel =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
	                               | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
	                               | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
	                               | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
	  toPixel -= 3; \
	} \
	fromPixel -= step; \
  } \
  fromPixel += step + 1; \
  toPixel++; \
  Int2Char t; \
  t.piece0 = fromPixel[0]; \
  t.piece1 = fromPixel[1]; \
  t.piece2 = fromPixel[2]; \
  r = t.all & fromRed; \
  g = t.all & fromGreen; \
  b = t.all & fromBlue; \
  a = fromAlpha ? t.all & fromAlpha : 0xFFFFFFFF; \
  t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
          | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
          | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
          | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  toPixel[0] = t.piece0; \
  toPixel[1] = t.piece1; \
  toPixel[2] = t.piece2; \
}

#define GrayFloat2OddBits(fromSize) \
{ \
  fromSize *      fromPixel = (fromSize *) ((char *) i->memory + i->stride * image.height - step) - 1; \
  unsigned char * begin     = (unsigned char *) o->memory; \
  unsigned char * toPixel   = begin + o->stride * result.height; \
  while (toPixel > begin) \
  { \
	unsigned char * rowBegin = max (toPixel - o->stride, begin); \
	while (toPixel > rowBegin) \
	{ \
	  fromSize v = min (max (*fromPixel--, (fromSize) 0.0), (fromSize) 1.0); \
	  unsigned int t = lutFloat2Char[(unsigned short) (65535 * v)]; \
	  * (unsigned int *) toPixel =   ((redShift   > 0 ? t << redShift   : t >> -redShift)   & redMask) \
	                               | ((greenShift > 0 ? t << greenShift : t >> -greenShift) & greenMask) \
	                               | ((blueShift  > 0 ? t << blueShift  : t >> -blueShift)  & blueMask) \
	                               | alphaMask; \
	  toPixel -= 3; \
	} \
	fromPixel = (unsigned fromSize *) ((char *) fromPixel - step); \
  } \
  fromPixel = (unsigned fromSize *) ((char *) fromPixel + step); \
  toPixel++; \
  fromSize v = min (max (*fromPixel, (fromSize) 0.0), (fromSize) 1.0); \
  Int2Char t; \
  t.all = lutFloat2Char[(unsigned short) (65535 * v)]; \
  t.all =   ((redShift   > 0 ? t.all << redShift   : t.all >> -redShift)   & redMask) \
          | ((greenShift > 0 ? t.all << greenShift : t.all >> -greenShift) & greenMask) \
          | ((blueShift  > 0 ? t.all << blueShift  : t.all >> -blueShift)  & blueMask) \
          | alphaMask; \
  toPixel[0] = t.piece0; \
  toPixel[1] = t.piece1; \
  toPixel[2] = t.piece2; \
}

#endif

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

  const int step = i->stride - image.width;

  switch ((int) depth)
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

  const int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift;
  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFF, 0xFF, 0xFF, 0, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;

  const int step = i->stride - image.width * sizeof (short);

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
	  fromPixel = (unsigned short *) ((char *) fromPixel + step); \
	} \
  }

  switch ((int) depth)
  {
    case 1:
	  GrayShort2Bits (char);
	  break;
    case 2:
	  GrayShort2Bits (short);
	  break;
    case 3:
	{
#     if BYTE_ORDER == LITTLE_ENDIAN
	  unsigned short * fromPixel = (unsigned short *) i->memory;
	  unsigned char *  toPixel   = (unsigned char *)  o->memory;
	  unsigned char *  end       = toPixel + o->stride * result.height - 3;
	  while (toPixel < end)
	  {
		unsigned char * rowEnd = min (toPixel + o->stride, end);
		while (toPixel < rowEnd)
		{
		  unsigned int v = lutFloat2Char[*fromPixel++ << grayShift];
		  * (unsigned int *) toPixel =   ((redShift   > 0 ? v << redShift   : v >> -redShift)   & redMask)
		                               | ((greenShift > 0 ? v << greenShift : v >> -greenShift) & greenMask)
		                               | ((blueShift  > 0 ? v << blueShift  : v >> -blueShift)  & blueMask)
		                               | alphaMask;
		  toPixel += 3;
		}
		fromPixel = (unsigned short *) ((char *) fromPixel + step);
	  }
	  fromPixel = (unsigned short *) ((char *) fromPixel - step);
	  unsigned int v = lutFloat2Char[*fromPixel++ << grayShift];
	  Int2Char t;
	  t.all =   ((redShift   > 0 ? v << redShift   : v >> -redShift)   & redMask)
		      | ((greenShift > 0 ? v << greenShift : v >> -greenShift) & greenMask)
		      | ((blueShift  > 0 ? v << blueShift  : v >> -blueShift)  & blueMask)
		      | alphaMask;
	  toPixel[0] = t.piece0;
	  toPixel[1] = t.piece1;
	  toPixel[2] = t.piece2;
#     elif BYTE_ORDER == BIG_ENDIAN
	  unsigned short * fromPixel = (unsigned short *) ((char *) i->memory + i->stride * image.height - step) - 1;
	  unsigned char *  begin     = (unsigned char *) o->memory;
	  unsigned char *  toPixel   = end + o->stride * result.height - 4;
	  while (toPixel > begin)
	  {
		unsigned char * rowBegin = max (toPixel - o->stride, begin);
		while (toPixel > rowBegin)
		{
		  unsigned int v = lutFloat2Char[*fromPixel-- << grayShift];
		  * (unsigned int *) toPixel =   ((redShift   > 0 ? v << redShift   : v >> -redShift)   & redMask)
		                               | ((greenShift > 0 ? v << greenShift : v >> -greenShift) & greenMask)
		                               | ((blueShift  > 0 ? v << blueShift  : v >> -blueShift)  & blueMask)
		                               | alphaMask;
		  toPixel -= 3;
		}
		fromPixel = (unsigned short *) ((char *) fromPixel - step);
	  }
	  fromPixel = (unsigned short *) ((char *) fromPixel + step);
	  toPixel++;
	  unsigned int v = lutFloat2Char[*fromPixel << grayShift];
	  Int2Char t;
	  t.all =   ((redShift   > 0 ? v << redShift   : v >> -redShift)   & redMask)
		      | ((greenShift > 0 ? v << greenShift : v >> -greenShift) & greenMask)
		      | ((blueShift  > 0 ? v << blueShift  : v >> -blueShift)  & blueMask)
		      | alphaMask;
	  toPixel[0] = t.piece0;
	  toPixel[1] = t.piece1;
	  toPixel[2] = t.piece2;
#     endif
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

  const int step = i->stride - image.width * sizeof (float);

  switch ((int) depth)
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

  const int step = i->stride - image.width * sizeof (double);

  switch ((int) depth)
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

  const int thatDepth = (int) that->depth;
  const int step = i->stride - image.width * thatDepth;

  switch ((int) depth)
  {
    case 1:
	  switch (thatDepth)
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
	  switch (thatDepth)
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
	  switch (thatDepth)
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
	  switch (thatDepth)
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
  int rowWidth = result.width;
  int blockRowWidth = i->ratioH;
  int blockSwath = result.width * i->ratioV;
  int step12 = i->stride12 - image.width / i->ratioH;
  int toStep   = result.width * i->ratioV - result.width;
  int fromStep = i->stride0   * i->ratioV - image.width;
  int toBlockStep   = i->ratioH - result.width * i->ratioV;
  int fromBlockStep = i->ratioH - i->stride0   * i->ratioV;
  int toBlockRowStep   = result.width - i->ratioH;
  int fromBlockRowStep = i->stride0   - i->ratioH;

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

  switch ((int) depth)
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
	  unsigned char * end     = toPixel + result.width * result.height * 3;
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

  switch ((int) depth)
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
  switch ((int) depth)
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

  switch ((int) depth)
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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
  unsigned char * end       = toPixel + result.width * result.height * 4;
  const int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + o->stride;
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
  unsigned char * end       = toPixel + result.width * result.height * 4;
  const int step = i->stride - image.width * sizeof (float);
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 4;
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
	fromPixel = (float *) ((char *) fromPixel + step);
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
  unsigned char * end       = toPixel + result.width * result.height * 4;
  const int step = i->stride - image.width * sizeof (double);
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 4;
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
	fromPixel = (double *) ((char *) fromPixel + step);
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
  unsigned char * end       = toPixel + result.width * result.height * 4;
  const int step = i->stride - image.width * 3;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 4;
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
  if (result.width <= 0  ||  result.height <= 0) return result;

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
  unsigned char * end       = toPixel + result.width * result.height * 3;
  const int step = i->stride - image.width;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 3;
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
  unsigned char *  end       = toPixel + result.width * result.height * 3;
  const int grayShift = ((PixelFormatGrayShort *) image.format)->grayShift;
  const int step = i->stride - image.width * sizeof (short);
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
	fromPixel = (unsigned short *) ((char *) fromPixel + step);
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
  unsigned char * end       = toPixel + result.width * result.height * 3;
  const int step = i->stride - image.width * sizeof (float);
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 3;
	while (toPixel < rowEnd)
	{
	  float v = min (max (*fromPixel++, 0.0f), 1.0f);
	  unsigned char t = lutFloat2Char[(unsigned short) (65535 * v)];
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
	fromPixel = (float *) ((char *) fromPixel + step);
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
  unsigned char * end       = toPixel + result.width * result.height * 3;
  const int step = i->stride - image.width * sizeof (double);
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 3;
	while (toPixel < rowEnd)
	{
	  double v = min (max (*fromPixel++, 0.0), 1.0);
	  unsigned char t = lutFloat2Char[(unsigned short) (65535 * v)];
	  toPixel[0] = t;
	  toPixel[1] = t;
	  toPixel[2] = t;
	  toPixel += 3;
	}
	fromPixel = (double *) ((char *) fromPixel + step);
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
  unsigned char * end       = toPixel + result.width * result.height * 3;
  const int step = i->stride - image.width * 4;
  while (toPixel < end)
  {
	unsigned char * rowEnd = toPixel + result.width * 3;
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


// class PixelFormatPackedYUV -------------------------------------------------

PixelFormatPackedYUV::PixelFormatPackedYUV (YUVindex * table)
: PixelFormatYUV (1, 1)
{
  planes     = -1;
  precedence = 1;
  monochrome = false;
  hasAlpha   = false;

  // Analyze table to initialize remaining members
  this->table = table;
  pixels = 0;
  bytes = 0;
  set<int> Usamples;
  set<int> Vsamples;
  YUVindex * i = table;
  while (i->y >= 0)
  {
	pixels++;
	bytes = max (bytes, i->y, i->u, i->v);
	Usamples.insert (i->u);
	Vsamples.insert (i->v);
	i++;
  }
  bytes++;  // Up to now, bytes is the index of highest byte in group.  Must convert to quantity of bytes.
  depth = (float) bytes / pixels;
  ratioH = pixels / min (Usamples.size (), Vsamples.size ());
}

Image
PixelFormatPackedYUV::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.resize (image.width, image.height);
  result.timestamp = image.timestamp;
  if (result.width <= 0  ||  result.height <= 0) return result;

  if (dynamic_cast<const PixelFormatYUV *> (image.format))
  {
	fromYUV (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatPackedYUV::fromAny (const Image & image, Image & result) const
{
  const PixelFormat * sourceFormat = image.format;
  PixelBuffer *       sourceBuffer = (PixelBuffer *) image.buffer;
  const int           sourceDepth  = (int) sourceFormat->depth;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferGroups * o = (PixelBufferGroups *) result.buffer;
  assert (o);

  unsigned char *       address  = (unsigned char *) o->memory;
  const unsigned char * end      = address + o->stride * result.height;
  const int             rowWidth = (int) (result.width * depth);
  const int             toStep   = o->stride - rowWidth;

  const unsigned int shift = 16 + (int) rint (log ((double) ratioH) / log (2.0));
  const int bias           = 0x808 << (shift - 4);  // include both bias and rounding in single constant
  const int maximum        = (~(unsigned int) 0) >> (24 - shift);

  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	const int fromStep = i->stride - image.width * sourceDepth;

	while (address < end)
	{
	  const unsigned char * rowEnd = address + rowWidth;
	  while (address < rowEnd)
	  {
		YUVindex * index = table;
		while (index->y >= 0)
		{
		  int r = 0;
		  int g = 0;
		  int b = 0;
		  for (int p = 0; p < ratioH; p++)
		  {
			unsigned int rgba = sourceFormat->getRGBA (source);
			source += sourceDepth;
			int sr =  rgba             >> 24;  // assumes 32-bit int
			int sg = (rgba & 0xFF0000) >> 16;
			int sb = (rgba &   0xFF00) >>  8;
			r += sr;
			g += sg;
			b += sb;
			address[(*index++).y] = min (max (0x4C84 * sr + 0x962B * sg + 0x1D4F * sb + 0x8000, 0), 0xFFFFFF) >> 16;
		  }
		  // This assumes that all ratioH pixels share the same U and V values,
		  // and that ratioH accurately represents the structure of the group.
		  index--;
		  address[index->u] = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
		  address[index->v] = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
		  index++;
		}
		address += bytes;
	  }
	  address += toStep;
	  source += fromStep;
	}
  }
  else
  {
	int y = 0;
	while (address < end)
	{
	  int x = 0;
	  const unsigned char * rowEnd = address + rowWidth;
	  while (address < rowEnd)
	  {
		YUVindex * index = table;
		while (index->y >= 0)
		{
		  int r = 0;
		  int g = 0;
		  int b = 0;
		  for (int p = 0; p < ratioH; p++)
		  {
			unsigned int rgba = sourceFormat->getRGBA (sourceBuffer->pixel (x++, y));
			int sr =  rgba             >> 24;
			int sg = (rgba & 0xFF0000) >> 16;
			int sb = (rgba &   0xFF00) >>  8;
			r += sr;
			g += sg;
			b += sb;
			address[(*index++).y] = min (max (0x4C84 * sr + 0x962B * sg + 0x1D4F * sb + 0x8000, 0), 0xFFFFFF) >> 16;
		  }
		  index--;
		  address[index->u] = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
		  address[index->v] = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
		  index++;
		}
		address += bytes;
	  }
	  address += toStep;
	  y++;
	}
  }
}

void
PixelFormatPackedYUV::fromYUV (const Image & image, Image & result) const
{
  const PixelFormat * sourceFormat = image.format;
  PixelBuffer *       sourceBuffer = (PixelBuffer *) image.buffer;
  const int           sourceDepth  = (int) sourceFormat->depth;

  PixelBufferGroups * o = (PixelBufferGroups *) result.buffer;
  assert (o);
  unsigned char *       address  = (unsigned char *) o->memory;
  const unsigned char * end      = address + o->stride * result.height;
  const int             rowWidth = (int) (result.width * depth);
  const int             toStep   = o->stride - rowWidth;

  const unsigned int shift = 8 + (int) rint (log ((double) ratioH) / log (2.0));
  const unsigned int roundup = 0x80 << (shift - 8);

  int y = 0;
  while (address < end)
  {
	int x = 0;
	const unsigned char * rowEnd = address + rowWidth;
	while (address < rowEnd)
	{
	  YUVindex * index = table;
	  while (index->y >= 0)
	  {
		unsigned int u = 0;
		unsigned int v = 0;
		for (int p = 0; p < ratioH; p++)
		{
		  unsigned int yuv = sourceFormat->getYUV (sourceBuffer->pixel (x++, y));
		  u +=  yuv & 0xFF00;
		  v += (yuv &   0xFF) << 8;
		  address[(*index++).y] = yuv >> 16;  // don't mask, on assumption that higher order bits of yuv are 0
		}
		index--;
		address[index->u] = (u + roundup) >> shift;
		address[index->v] = (v + roundup) >> shift;
		index++;
	  }
	  address += bytes;
	}
	address += toStep;
	y++;
  }
}

PixelBuffer *
PixelFormatPackedYUV::attach (void * block, int width, int height, bool copy) const
{
  PixelBufferGroups * result = new PixelBufferGroups (block, (int) ceil (width / pixels) * bytes, height, pixels, bytes);
  if (copy) result->memory.copyFrom (block);
  return result;
}

bool
PixelFormatPackedYUV::operator == (const PixelFormat & that) const
{
  const PixelFormatPackedYUV * p = dynamic_cast<const PixelFormatPackedYUV *> (&that);
  if (! p  ||  p->pixels != pixels) return false;
  for (int i = 0; i < pixels; i++)
  {
	YUVindex & me = table[i];
	YUVindex & thee = p->table[i];
	if (me.y != thee.y  ||  me.u != thee.u  ||  me.v != thee.v) return false;
  }
  return true;
}

unsigned int
PixelFormatPackedYUV::getRGBA (void * pixel) const
{
  PixelBufferGroups::PixelData * data = (PixelBufferGroups::PixelData *) pixel;
  register unsigned char * address = data->address;
  YUVindex & index = table[data->index];

  int y = address[index.y] << 16;
  int u = address[index.u] - 128;
  int v = address[index.v] - 128;

  unsigned int r = min (max (y               + 0x166F7 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v + 0x8000, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u               + 0x8000, 0), 0xFFFFFF);

  return ((r << 8) & 0xFF000000) | (g & 0xFF0000) | ((b >> 8) & 0xFF00) | 0xFF;
}

unsigned int
PixelFormatPackedYUV::getYUV (void * pixel) const
{
  PixelBufferGroups::PixelData * data = (PixelBufferGroups::PixelData *) pixel;
  register unsigned char * address = data->address;
  YUVindex & index = table[data->index];

  return (address[index.y] << 16) | (address[index.u] << 8) | address[index.v];
}

unsigned char
PixelFormatPackedYUV::getGray (void * pixel) const
{
  PixelBufferGroups::PixelData * data = (PixelBufferGroups::PixelData *) pixel;
  return data->address[table[data->index].y];
}

void
PixelFormatPackedYUV::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF000000) >> 24;
  int g = (rgba &   0xFF0000) >> 16;
  int b = (rgba &     0xFF00) >>  8;

  unsigned char y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
  unsigned char v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

  PixelBufferGroups::PixelData * data = (PixelBufferGroups::PixelData *) pixel;
  register unsigned char * address = data->address;
  YUVindex & index = table[data->index];

  address[index.y] = y;
  address[index.u] = u;
  address[index.v] = v;
}

void
PixelFormatPackedYUV::setYUV (void * pixel, unsigned int yuv) const
{
  PixelBufferGroups::PixelData * data = (PixelBufferGroups::PixelData *) pixel;
  register unsigned char * address = data->address;
  YUVindex & index = table[data->index];

  address[index.y] =  yuv           >> 16;
  address[index.u] = (yuv & 0xFF00) >>  8;
  address[index.v] =  yuv &   0xFF;
}


// class PixelFormatPlanarYUV -------------------------------------------------

PixelFormatPlanarYUV::PixelFormatPlanarYUV (int ratioH, int ratioV)
: PixelFormatYUV (ratioH, ratioV)
{
  planes     = 3;
  depth      = sizeof (char);
  precedence = 1;
  monochrome = false;
  hasAlpha   = false;
}

void
PixelFormatPlanarYUV::fromAny (const Image & image, Image & result) const
{
  assert (image.width % ratioH == 0  &&  image.height % ratioV == 0);

  const PixelFormat * sourceFormat = image.format;
  PixelBuffer *       sourceBuffer = (PixelBuffer *) image.buffer;
  const int           sourceDepth  = (int) sourceFormat->depth;

  PixelBufferPacked * i = (PixelBufferPacked *) image.buffer;
  PixelBufferPlanar * o = (PixelBufferPlanar *) result.buffer;

  int rowWidth      = result.width;
  int blockRowWidth = ratioH;

  const unsigned int shift = 16 + (int) rint (log ((double) ratioH * ratioV) / log (2.0));
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

	  const int fromStep         = i->stride * ratioV - image.width * sourceDepth;
	  const int fromBlockStep    = ratioH * sourceDepth - i->stride * ratioV;
	  const int fromBlockRowStep = i->stride - ratioH * sourceDepth;

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
	  const int blockSwath       = i->stride * ratioV;
	  const int fromStep         = i->stride * ratioV - image.width * sourceDepth;
	  const int fromBlockStep    = ratioH * sourceDepth - i->stride * ratioV;
	  const int fromBlockRowStep = i->stride - ratioH * sourceDepth;

	  int y = 0;
	  unsigned char * end = source + i->stride * image.height;
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

PixelBuffer *
PixelFormatPlanarYUV::attach (void * block, int width, int height, bool copy) const
{
  int size = width * height;
  char * buffer1 = (char *) block + size;
  char * buffer2 = buffer1 + size / (ratioH * ratioV);
  PixelBufferPlanar * result = new PixelBufferPlanar (block, buffer1, buffer2, width, width / ratioH, height, ratioH, ratioV);

  if (copy)
  {
	PixelBufferPlanar * temp = result;
	result = (PixelBufferPlanar *) temp->duplicate ();
	delete temp;
  }

  return result;
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

  // Y =  0.2989*R +0.5866*G +0.1145*B
  // U = -0.1687*R -0.3312*G +0.5000*B
  // V =  0.5000*R -0.4183*G -0.0816*B
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

  const unsigned int shift = 16 + (int) rint (log ((double) ratioH * ratioV) / log (2.0));
  const int bias = 0x808 << (shift - 4);

  if (i)
  {
	unsigned char * source = (unsigned char *) i->memory;
	const int sourceDepth = (int) sourceFormat->depth;

	const int fromStep         = i->stride * ratioV - image.width * sourceDepth;
	const int fromBlockStep    = ratioH * sourceDepth - i->stride * ratioV;
	const int fromBlockRowStep = i->stride - ratioH * sourceDepth;

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

PixelBuffer *
PixelFormatPlanarYCbCr::attach (void * block, int width, int height, bool copy) const
{
  int size = width * height;
  char * buffer1 = (char *) block + size;
  char * buffer2 = buffer1 + size / (ratioH * ratioV);
  PixelBufferPlanar * result = new PixelBufferPlanar (block, buffer1, buffer2, width, width / ratioH, height, ratioH, ratioV);

  if (copy)
  {
	PixelBufferPlanar * temp = result;
	result = (PixelBufferPlanar *) temp->duplicate ();
	delete temp;
  }

  return result;
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
