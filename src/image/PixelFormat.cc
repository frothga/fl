#include "fl/image.h"
#include "fl/pi.h"
#include "fl/math.h"

#include <algorithm>


// Include for tracing
//#include <iostream>


#ifdef _MSC_VER
  // MSVC compiles to little endian 99.9999% of the time for practical purposes.  Don't
  // know what the exceptions are, but imagine there could be some.
  #define __LITTLE_ENDIAN 1234
  #define __BYTE_ORDER __LITTLE_ENDIAN
#else
  #include <sys/param.h>
#endif

#if __BYTE_ORDER != __LITTLE_ENDIAN
#warning This code assumes the system is little endian.
#warning To fix: need to write big-endian versions of
#warning bit-twiddling macros and wrap both versions
#warning in an ifdef structure.
#endif


using namespace std;
using namespace fl;


PixelFormatGrayChar   fl::GrayChar;
PixelFormatGrayFloat  fl::GrayFloat;
PixelFormatGrayDouble fl::GrayDouble;
PixelFormatRGBAChar   fl::RGBAChar;
PixelFormatRGBABits   fl::BGRChar (3, 0xFF, 0xFF00, 0xFF0000, 0x0);
PixelFormatRGBABits   fl::ABGRChar (4, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
PixelFormatRGBAFloat  fl::RGBAFloat;
PixelFormatYVYUChar   fl::YVYUChar;
PixelFormatVYUYChar   fl::VYUYChar;
PixelFormatHLSFloat   fl::HLSFloat;


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
  unsigned char piece[4];
};


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


// class Pixel ----------------------------------------------------------------

Pixel::Pixel ()
{
  format = & RGBAFloat;
  pixel = & data;
}

Pixel::Pixel (unsigned int rgba)
{
  format = & RGBAChar;
  pixel = & data;
  format->setRGBA (pixel, rgba);
}

Pixel::Pixel (const Pixel & that)
{
  format = that.format;
  if (that.pixel == & that.data)
  {
	pixel = & data;
	data = that.data;
  }
  else
  {
	pixel = that.pixel;
	// Ignore the data
  }
}

Pixel::Pixel (const PixelFormat & format, void * pixel)
{
  this->format = & format;
  this->pixel = pixel;
}

unsigned int
Pixel::getRGBA () const
{
  return format->getRGBA (pixel);
}

void
Pixel::getRGBA (float values[]) const
{
  format->getRGBA (pixel, values);
}

void
Pixel::getXYZ (float values[]) const
{
  format->getXYZ (pixel, values);
}

void
Pixel::setRGBA (unsigned int rgba) const
{
  format->setRGBA (pixel, rgba);
}

void
Pixel::setRGBA (float values[]) const
{
  format->setRGBA (pixel, values);
}

void
Pixel::setXYZ (float values[]) const
{
  format->setXYZ (pixel, values);
}

Pixel &
Pixel::operator = (const Pixel & that)
{
  float values[4];
  that.format->getRGBA (that.pixel, values);
  format->setRGBA (pixel, values);
  return *this;
}

Pixel &
Pixel::operator += (const Pixel & that)
{
  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  thisValue[0] += thatValue[0];
  thisValue[1] += thatValue[1];
  thisValue[2] += thatValue[2];
  thisValue[3] += thatValue[3];

  format->setRGBA (pixel, thisValue);

  return *this;
}

Pixel
Pixel::operator + (const Pixel & that) const
{
  Pixel result;

  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  result.data.rgbafloat[0] = thisValue[0] + thatValue[0];
  result.data.rgbafloat[1] = thisValue[1] + thatValue[1];
  result.data.rgbafloat[2] = thisValue[2] + thatValue[2];
  result.data.rgbafloat[3] = thisValue[3] + thatValue[3];

  return result;
}

Pixel
Pixel::operator * (const Pixel & that) const
{
  Pixel result;

  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  result.data.rgbafloat[0] = thisValue[0] * thatValue[0];
  result.data.rgbafloat[1] = thisValue[1] * thatValue[1];
  result.data.rgbafloat[2] = thisValue[2] * thatValue[2];
  result.data.rgbafloat[3] = thisValue[3] * thatValue[3];

  return result;
}

Pixel
Pixel::operator * (float scalar) const
{
  Pixel result;

  float values[4];
  format->getRGBA (pixel, values);

  result.data.rgbafloat[0] = values[0] * scalar;
  result.data.rgbafloat[1] = values[1] * scalar;
  result.data.rgbafloat[2] = values[2] * scalar;
  result.data.rgbafloat[4] = values[3] * scalar;

  return result;
}

Pixel
Pixel::operator / (float scalar) const
{
  Pixel result;

  float values[4];
  format->getRGBA (pixel, values);

  result.data.rgbafloat[0] = values[0] / scalar;
  result.data.rgbafloat[1] = values[1] / scalar;
  result.data.rgbafloat[2] = values[2] / scalar;
  result.data.rgbafloat[4] = values[3] / scalar;

  return result;
}

Pixel
Pixel::operator << (const Pixel & that)
{
  Pixel result;

  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  float a = thatValue[3];
  float a1 = 1.0f - a;
  result.data.rgbafloat[0] = a1 * thisValue[0] + a * thatValue[0];
  result.data.rgbafloat[1] = a1 * thisValue[1] + a * thatValue[1];
  result.data.rgbafloat[2] = a1 * thisValue[2] + a * thatValue[2];
  result.data.rgbafloat[3] = thisValue[3];  // Don't know what to do for alpha values themselves

  return result;
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

  result.width     = image.width;
  result.height    = image.height;
  result.timestamp = image.timestamp;

  fromAny (image, result);

  return result;
}

void
PixelFormat::fromAny (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  unsigned char * dest = (unsigned char *) result.buffer;
  unsigned char * end = dest + result.buffer.size ();
  unsigned char * source = (unsigned char *) image.buffer;
  const PixelFormat * sourceFormat = image.format;
  int sourceDepth = sourceFormat->depth;
  while (dest < end)
  {
	// Kinda fast and definitely dirty.  XYZ would be more precise, but this
	// is also accurate, since RGB values are well defined (as non-linear sRGB).
	setRGBA (dest, sourceFormat->getRGBA (source));
	source += sourceDepth;
	dest += depth;
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
  values[0] = ((rgba &   0xFF0000) >> 16) / 255.0f;
  values[1] = ((rgba &     0xFF00) >>  8) / 255.0f;
  values[2] = ((rgba &       0xFF)      ) / 255.0f;
  values[3] = ((rgba & 0xFF000000) >> 24) / 255.0f;
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
  values[3] = rgbValues[3];
}

/**
   See PixelFormatYVYUChar::setRGBA() for more details on the conversion matrix.
 **/
unsigned int
PixelFormat::getYUV  (void * pixel) const
{
  unsigned int rgba = getRGBA (pixel);

  int r = (rgba & 0xFF0000) >> 16;
  int g = (rgba &   0xFF00) >> 8;
  int b =  rgba &     0xFF;

  unsigned int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b,            0), 0xFFFFFF) & 0xFF0000;
  unsigned int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000, 0), 0xFFFFFF) & 0xFF0000;
  unsigned int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000, 0), 0xFFFFFF) & 0xFF0000;

  return y | (u >> 8) | (v >> 16);
}

unsigned char
PixelFormat::getGray (void * pixel) const
{
  unsigned int rgba = getRGBA (pixel);
  unsigned int r = (rgba & 0xFF0000) >> 8;
  unsigned int g =  rgba & 0xFF00;
  unsigned int b = (rgba & 0xFF)     << 8;
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
  return 0xFF;
}

void
PixelFormat::setRGBA (void * pixel, float values[]) const
{
  unsigned int rgba = ((unsigned int) (values[3] * 255)) << 24;
  for (int i = 0; i < 3; i++)
  {
	float v = values[i];
	v = min (v, 1.0f);
	v = max (v, 0.0f);
	delinearize (v);
	rgba |= ((unsigned int) (v * 255)) << ((2 - i) * 8);
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
  rgbValues[3] = values[3];

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

  // See PixelFormatYVYU::getRGB() for an explanation of this arithmetic.
  unsigned int r = min (max (y               + 0x166F7 * v, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u,               0), 0xFFFFFF);

  setRGBA (pixel, 0xFF000000 | (r & 0xFF0000) | ((g >> 8) & 0xFF00) | (b >> 16));
}

void
PixelFormat::setGray (void * pixel, unsigned char gray) const
{
  register unsigned int iv = gray;
  setRGBA (pixel, 0xFF000000 | (iv << 16) | (iv << 8) | iv);
}

void
PixelFormat::setGray (void * pixel, float gray) const
{
  gray = min (gray, 1.0f);
  gray = max (gray, 0.0f);
  delinearize (gray);
  unsigned int iv = (unsigned int) (gray * 255);
  unsigned int rgba = 0xFF000000 | (iv << 16) | (iv << 8) | iv;
  setRGBA (pixel, rgba);
}

void
PixelFormat::setAlpha (void * pixel, unsigned char alpha) const
{
  // Do nothing.
  // Could getRGBA(), replace alpha value, and then setRGBA().  However,
  // a better plan is simply to let clases that actually have an alpha
  // channel override this method.
}


// class PixelFormatGrayChar --------------------------------------------------

PixelFormatGrayChar::PixelFormatGrayChar ()
{
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

  result.width     = image.width;
  result.height    = image.height;
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayFloat))
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
PixelFormatGrayChar::fromGrayFloat (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  float * fromPixel = (float *) image.buffer;
  unsigned char * toPixel = (unsigned char *) result.buffer;
  unsigned char * end = toPixel + result.width * result.height;
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
  result.buffer.grow (result.width * result.height * depth);

  double * fromPixel = (double *) image.buffer;
  unsigned char * toPixel = (unsigned char *) result.buffer;
  unsigned char * end = toPixel + result.width * result.height;
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
  result.buffer.grow (result.width * result.height * depth);

  unsigned int * fromPixel = (unsigned int *) image.buffer;
  unsigned char * toPixel  = (unsigned char *) result.buffer;
  unsigned char * end      = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	unsigned int r = (*fromPixel & 0xFF0000) >> 8;
	unsigned int g =  *fromPixel & 0xFF00;
	unsigned int b = (*fromPixel & 0xFF)     << 8;
	fromPixel++;
	*toPixel++ = ((redWeight * r + greenWeight * g + blueWeight * b) / totalWeight) >> 8;
  }
}

void
PixelFormatGrayChar::fromRGBABits (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, redShift, greenShift, blueShift, alphaShift);

  #define RGBBits2GrayChar(fromSize) \
  { \
    unsigned fromSize * fromPixel = (unsigned fromSize *) image.buffer; \
    unsigned char * toPixel       = (unsigned char *)     result.buffer; \
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
	  unsigned char * fromPixel = (unsigned char *) image.buffer;
	  unsigned char * toPixel   = (unsigned char *) result.buffer;
	  unsigned char * end       = toPixel + result.width * result.height;
	  while (toPixel < end)
	  {
		Int2Char t;
		t.piece[0] = *fromPixel++;
		t.piece[1] = *fromPixel++;
		t.piece[2] = *fromPixel++;
		unsigned int r = t.all & that->redMask;
		unsigned int g = t.all & that->greenMask;
		unsigned int b = t.all & that->blueMask;
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
  result.buffer.grow (result.width * result.height * depth);

  unsigned char * dest = (unsigned char *) result.buffer;
  unsigned char * end = dest + result.buffer.size ();
  unsigned char * source = (unsigned char *) image.buffer;
  const PixelFormat * sourceFormat = image.format;
  int sourceDepth = sourceFormat->depth;
  while (dest < end)
  {
	*dest++ = sourceFormat->getGray (source);
	source += sourceDepth;
  }
}

unsigned int
PixelFormatGrayChar::getRGBA (void * pixel) const
{
  unsigned int t = *((unsigned char *) pixel);
  return 0xFF000000 | (t << 16) | (t << 8) | t;
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
  unsigned int r = (rgba &   0xFF0000) >> 8;
  unsigned int g =  rgba &     0xFF00;
  unsigned int b = (rgba &       0xFF) << 8;

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


// class PixelFormatGrayFloat -------------------------------------------------

PixelFormatGrayFloat::PixelFormatGrayFloat ()
{
  depth       = 4;
  precedence  = 3;  // Above all integer formats and below GrayDouble
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

  result.width     = image.width;
  result.height    = image.height;
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
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
PixelFormatGrayFloat::fromGrayChar (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  unsigned char * fromPixel = (unsigned char *) image.buffer;
  float * toPixel = (float *) result.buffer;
  float * end = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float v = *fromPixel++ / 255.0f;
	linearize (v);
	*toPixel++ = v;
  }
}

void
PixelFormatGrayFloat::fromGrayDouble (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  double * fromPixel = (double *) image.buffer;
  float * toPixel = (float *) result.buffer;
  float * end = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	*toPixel++ = (float) *fromPixel++;
  }
}

void
PixelFormatGrayFloat::fromRGBAChar (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  unsigned int * fromPixel = (unsigned int *) image.buffer;
  float *        toPixel   = (float *)        result.buffer;
  float *        end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float r = ((*fromPixel & 0xFF0000) >> 16) / 255.0f;
	float g = ((*fromPixel &   0xFF00) >>  8) / 255.0f;
	float b = ((*fromPixel &     0xFF)      ) / 255.0f;
    fromPixel++;
	linearize (r);
	linearize (g);
	linearize (b);
	*toPixel++ = redToY * r + greenToY * g + blueToY * b;
  }
}

void
PixelFormatGrayFloat::fromRGBABits (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (0xFF, 0xFF, 0xFF, 0xFF, redShift, greenShift, blueShift, alphaShift);

  #define RGBBits2GrayFloat(imageSize) \
  { \
    unsigned imageSize * fromPixel = (unsigned imageSize *) image.buffer; \
    float *              toPixel   = (float *)              result.buffer; \
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
	  unsigned char * fromPixel = (unsigned char *) image.buffer;
	  float *         toPixel   = (float *)         result.buffer;
	  float *         end       = toPixel + result.width * result.height;
	  while (toPixel < end)
	  {
		Int2Char t;
		t.piece[0] = *fromPixel++;
		t.piece[1] = *fromPixel++;
		t.piece[2] = *fromPixel++;
		unsigned int r = t.all & that->redMask;
		unsigned int g = t.all & that->greenMask;
		unsigned int b = t.all & that->blueMask;
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
  result.buffer.grow (result.width * result.height * depth);

  float * dest = (float *) result.buffer;
  float * end = dest + result.width * result.height;
  unsigned char * source = (unsigned char *) image.buffer;
  const PixelFormat * sourceFormat = image.format;
  int sourceDepth = sourceFormat->depth;
  while (dest < end)
  {
	sourceFormat->getGray (source, *dest++);
	source += sourceDepth;
  }
}

unsigned int
PixelFormatGrayFloat::getRGBA (void * pixel) const
{
  float v = min (max (*((float *) pixel), 0.0f), 1.0f);
  delinearize (v);
  unsigned int t = (unsigned int) (v * 255);
  return 0xFF000000 | (t << 16) | (t << 8) | t;
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
  float r = ((rgba & 0xFF0000) >> 16) / 255.0f;
  float g = ((rgba &   0xFF00) >>  8) / 255.0f;
  float b = ((rgba &     0xFF)      ) / 255.0f;
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
  depth       = 8;
  precedence  = 4;  // above all integer formats and above GrayFloat
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

  result.width     = image.width;
  result.height    = image.height;
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatGrayChar))
  {
	fromGrayChar (image, result);
  }
  else if (typeid (* image.format) == typeid (PixelFormatGrayFloat))
  {
	fromGrayFloat (image, result);
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
PixelFormatGrayDouble::fromGrayChar (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  unsigned char * fromPixel = (unsigned char *) image.buffer;
  double * toPixel = (double *) result.buffer;
  double * end = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double v = *fromPixel++ / 255.0;
	linearize (v);
	*toPixel++ = v;
  }
}

void
PixelFormatGrayDouble::fromGrayFloat (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  float * fromPixel = (float *) image.buffer;
  double * toPixel = (double *) result.buffer;
  double * end = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	*toPixel++ = *fromPixel++;
  }
}

void
PixelFormatGrayDouble::fromRGBAChar (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  unsigned int * fromPixel = (unsigned int *) image.buffer;
  double *       toPixel   = (double *)       result.buffer;
  double *       end       = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double r = ((*fromPixel & 0xFF0000) >> 16) / 255.0;
	double g = ((*fromPixel &   0xFF00) >>  8) / 255.0;
	double b = ((*fromPixel &     0xFF)      ) / 255.0;
    fromPixel++;
	linearize (r);
	linearize (g);
	linearize (b);
	*toPixel++ = redToY * r + greenToY * g + blueToY * b;
  }
}

void
PixelFormatGrayDouble::fromRGBABits (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (0xFF, 0xFF, 0xFF, 0xFF, redShift, greenShift, blueShift, alphaShift);

  #define RGBBits2GrayDouble(imageSize) \
  { \
    unsigned imageSize * fromPixel = (unsigned imageSize *) image.buffer; \
    double *             toPixel   = (double *)             result.buffer; \
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
	  unsigned char * fromPixel = (unsigned char *) image.buffer;
	  double *        toPixel   = (double *)        result.buffer;
	  double *        end       = toPixel + result.width * result.height;
	  while (toPixel < end)
	  {
		Int2Char t;
		t.piece[0] = *fromPixel++;
		t.piece[1] = *fromPixel++;
		t.piece[2] = *fromPixel++;
		unsigned int r = t.all & that->redMask;
		unsigned int g = t.all & that->greenMask;
		unsigned int b = t.all & that->blueMask;
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
  result.buffer.grow (result.width * result.height * depth);

  double * dest = (double *) result.buffer;
  double * end = dest + result.width * result.height;
  unsigned char * source = (unsigned char *) image.buffer;
  const PixelFormat * sourceFormat = image.format;
  int sourceDepth = sourceFormat->depth;
  while (dest < end)
  {
	float value;
	sourceFormat->getGray (source, value);
	*dest++ = value;
	source += sourceDepth;
  }
}

unsigned int
PixelFormatGrayDouble::getRGBA (void * pixel) const
{
  double v = min (max (*((double *) pixel), 0.0), 1.0);
  delinearize (v);
  unsigned int t = (unsigned int) (v * 255);
  return 0xFF000000 | (t << 16) | (t << 8) | t;
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
  double r = ((rgba & 0xFF0000) >> 16) / 255.0;
  double g = ((rgba &   0xFF00) >>  8) / 255.0;
  double b = ((rgba &     0xFF)      ) / 255.0;
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


// class PixelFormatRGBAChar ---------------------------------------------------

PixelFormatRGBAChar::PixelFormatRGBAChar ()
{
  depth      = 4;
  precedence = 2;  // Above GrayChar and below all floating point formats
  monochrome = false;
  hasAlpha   = true;
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

  result.width     = image.width;
  result.height    = image.height;
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
  result.buffer.grow (result.width * result.height * depth);

  unsigned char * fromPixel = (unsigned char *) image.buffer;
  unsigned int * toPixel = (unsigned int *) result.buffer;
  unsigned int * end = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	unsigned int t = *fromPixel++;
	*toPixel++ = 0xFF000000 | (t << 16) | (t << 8) | t;
  }
}

void
PixelFormatRGBAChar::fromGrayFloat (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  float * fromPixel = (float *) image.buffer;
  unsigned int * toPixel = (unsigned int *) result.buffer;
  unsigned int * end = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	float v = min (max (*fromPixel++, 0.0f), 1.0f);
	delinearize (v);
	unsigned int t = (unsigned int) (v * 255);
	*toPixel++ = 0xFF000000 | (t << 16) | (t << 8) | t;
  }
}

void
PixelFormatRGBAChar::fromGrayDouble (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  double * fromPixel = (double *) image.buffer;
  unsigned int * toPixel = (unsigned int *) result.buffer;
  unsigned int * end = toPixel + result.width * result.height;
  while (toPixel < end)
  {
	double v = min (max (*fromPixel++, 0.0), 1.0);
	delinearize (v);
	unsigned int t = (unsigned int) (v * 255);
	*toPixel++ = 0xFF000000 | (t << 16) | (t << 8) | t;
  }
}


#define Bits2Bits(fromSize,toSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned fromSize * fromPixel = (unsigned fromSize *) image.buffer; \
  unsigned toSize *   toPixel   = (unsigned toSize *)   result.buffer; \
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

// In the following macros, we assume that "Odd" implies a stride of 3.
// We also assume that the machine is little endian, which means that we
// can directly compute byte positions relative to the first byte of the pixel.
// If we need something more general, then these macros must be updated...

#define OddBits2Bits(toSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char *   fromPixel = (unsigned char *)   image.buffer; \
  unsigned toSize * toPixel   = (unsigned toSize *) result.buffer; \
  unsigned toSize * end       = toPixel + result.width * result.height; \
  while (toPixel < end) \
  { \
    Int2Char t; \
    t.piece[0] = *fromPixel++; \
    t.piece[1] = *fromPixel++; \
    t.piece[2] = *fromPixel++; \
    unsigned int r = t.all & fromRed; \
	unsigned int g = t.all & fromGreen; \
	unsigned int b = t.all & fromBlue; \
	unsigned int a = t.all & fromAlpha; \
	*toPixel++ =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		         | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		         | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		         | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
  } \
}

#define Bits2OddBits(fromSize,fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned fromSize * fromPixel = (unsigned fromSize *) image.buffer; \
  unsigned char *     toPixel   = (unsigned char *)     result.buffer; \
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
    *toPixel++ = t.piece[0]; \
    *toPixel++ = t.piece[1]; \
    *toPixel++ = t.piece[2]; \
  } \
}

#define OddBits2OddBits(fromRed,fromGreen,fromBlue,fromAlpha,toRed,toGreen,toBlue,toAlpha) \
{ \
  unsigned char * fromPixel = (unsigned char *) image.buffer; \
  unsigned char * toPixel   = (unsigned char *) result.buffer; \
  unsigned char * end       = toPixel + result.width * result.height * 3; \
  while (toPixel < end) \
  { \
    Int2Char t; \
    t.piece[0] = *fromPixel++; \
    t.piece[1] = *fromPixel++; \
    t.piece[2] = *fromPixel++; \
    unsigned int r = t.all & fromRed; \
	unsigned int g = t.all & fromGreen; \
	unsigned int b = t.all & fromBlue; \
	unsigned int a = t.all & fromAlpha; \
	t.all =   ((redShift   > 0 ? r << redShift   : r >> -redShift)   & toRed) \
		    | ((greenShift > 0 ? g << greenShift : g >> -greenShift) & toGreen) \
		    | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  & toBlue) \
		    | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & toAlpha); \
    *toPixel++ = t.piece[0]; \
    *toPixel++ = t.piece[1]; \
    *toPixel++ = t.piece[2]; \
  } \
}


void
PixelFormatRGBAChar::fromRGBABits (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  PixelFormatRGBABits * that = (PixelFormatRGBABits *) image.format;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  that->shift (0xFF0000, 0xFF00, 0xFF, 0xFF000000, redShift, greenShift, blueShift, alphaShift);

  switch (that->depth)
  {
    case 1:
	  Bits2Bits (char, int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
	  break;
    case 2:
	  Bits2Bits (short, int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
	  break;
    case 3:
	  OddBits2Bits (int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
	  break;
    case 4:
    default:
	  Bits2Bits (int, int, that->redMask, that->greenMask, that->blueMask, that->alphaMask, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
  }
}

bool
PixelFormatRGBAChar::operator == (const PixelFormat & that) const
{
  if (const PixelFormatRGBAChar * other = dynamic_cast<const PixelFormatRGBAChar *> (& that))
  {
	return true;
  }
  if (const PixelFormatRGBABits * other = dynamic_cast<const PixelFormatRGBABits *> (& that))
  {
	return    depth      == other->depth
	       && 0xFF0000   == other->redMask
	       && 0xFF00     == other->greenMask
	       && 0xFF       == other->blueMask
	       && 0xFF000000 == other->alphaMask;
  }
  return false;
}

void
PixelFormatRGBAChar::shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift)
{
  redShift = -23;
  while (redMask >>= 1) {redShift++;}

  greenShift = -15;
  while (greenMask >>= 1) {greenShift++;}

  blueShift = -7;
  while (blueMask >>= 1) {blueShift++;}

  alphaShift = -31;
  while (alphaMask >>= 1) {alphaShift++;}
}

unsigned int
PixelFormatRGBAChar::getRGBA (void * pixel) const
{
  return *((unsigned int *) pixel);
}

unsigned char
PixelFormatRGBAChar::getAlpha (void * pixel) const
{
  return ((unsigned char *) pixel)[3];
}

void
PixelFormatRGBAChar::setRGBA (void * pixel, unsigned int rgba) const
{
  *((unsigned int *) pixel) = rgba;
}

void
PixelFormatRGBAChar::setAlpha (void * pixel, unsigned char alpha) const
{
  ((unsigned char *) pixel)[3] = alpha;
}


// class PixelFormatRGBABits ---------------------------------------------------

PixelFormatRGBABits::PixelFormatRGBABits (int depth, unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask)
{
  this->depth     = depth;
  this->redMask   = redMask;
  this->greenMask = greenMask;
  this->blueMask  = blueMask;
  this->alphaMask = alphaMask;

  precedence = 2;  // on par with RGBAChar
  monochrome = false;
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

  result.width     = image.width;
  result.height    = image.height;
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

// fromGrayChar () produces a bogus alpha channel!  Fix it.
void
PixelFormatRGBABits::fromGrayChar (const Image & image, Image & result) const
{
  if (redMask == 0xFF  &&  greenMask == 0xFF  &&  blueMask == 0xFF  &&  depth == 1)
  {
	result.buffer = image.buffer;
	return;
  }

  result.buffer.grow (result.width * result.height * depth);

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFF, 0xFF, 0xFF, 0xFF, redShift, greenShift, blueShift, alphaShift);
  redShift *= -1;
  greenShift *= -1;
  blueShift *= -1;
  alphaShift *= -1;

  switch (depth)
  {
    case 1:
	  Bits2Bits (char, char, 0xFF, 0xFF, 0xFF, 0xFF, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 2:
	  Bits2Bits (char, short, 0xFF, 0xFF, 0xFF, 0xFF, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 3:
	  Bits2OddBits (char, 0xFF, 0xFF, 0xFF, 0xFF, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 4:
    default:
	  Bits2Bits (char, int, 0xFF, 0xFF, 0xFF, 0xFF, redMask, greenMask, blueMask, alphaMask);
  }
}

#define GrayFloat2Bits(fromSize,toSize) \
{ \
  fromSize *        fromPixel = (fromSize *)        image.buffer; \
  unsigned toSize * toPixel   = (unsigned toSize *) result.buffer; \
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
  fromSize *      fromPixel = (fromSize *)      image.buffer; \
  unsigned char * toPixel   = (unsigned char *) result.buffer; \
  unsigned char * end       = toPixel + result.width * result.height * 3; \
  while (toPixel < end) \
  { \
	fromSize v = min (max (*fromPixel++, (fromSize) 0.0), (fromSize) 1.0); \
	delinearize (v); \
	unsigned int t = (unsigned int) (v * (255 << 8)); \
	t =   ((redShift   > 0 ? t << redShift   : t >> -redShift)   & redMask) \
	    | ((greenShift > 0 ? t << greenShift : t >> -greenShift) & greenMask) \
        | ((blueShift  > 0 ? t << blueShift  : t >> -blueShift)  & blueMask) \
        | alphaMask; \
    *toPixel++ =  t        & 0xFF; \
    *toPixel++ = (t >>= 8) & 0xFF; \
    *toPixel++ = (t >>  8) & 0xFF; \
  } \
}

void
PixelFormatRGBABits::fromGrayFloat (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

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
  result.buffer.grow (result.width * result.height * depth);

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
PixelFormatRGBABits::fromRGBAChar (const Image & image, Image & result) const
{
  if (redMask == 0xFF0000  &&  greenMask == 0xFF00  &&  blueMask == 0xFF  &&  alphaMask == 0xFF000000  &&  depth == 4)
  {
	result.buffer = image.buffer;
	return;
  }

  result.buffer.grow (result.width * result.height * depth);

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  PixelFormatRGBAChar::shift (redMask, greenMask, blueMask, alphaMask, redShift, greenShift, blueShift, alphaShift);

  switch (depth)
  {
    case 1:
	  Bits2Bits (int, char, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 2:
	  Bits2Bits (int, short, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 3:
	  Bits2OddBits (int, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, redMask, greenMask, blueMask, alphaMask);
	  break;
    case 4:
    default:
	  Bits2Bits (int, int, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, redMask, greenMask, blueMask, alphaMask);
  }
}

void
PixelFormatRGBABits::fromRGBABits (const Image & image, Image & result) const
{
  if (*image.format == *this)
  {
	result.buffer = image.buffer;
	return;
  }

  result.buffer.grow (result.width * result.height * depth);

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
  if (const PixelFormatRGBAChar * other = dynamic_cast<const PixelFormatRGBAChar *> (& that))
  {
	return    redMask   == 0xFF0000
	       && greenMask == 0xFF00
	       && blueMask  == 0xFF
	       && alphaMask == 0xFF000000;
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
  shift (0xFF0000, 0xFF00, 0xFF, 0xFF000000, redShift, greenShift, blueShift, alphaShift);

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
	  value.piece[0] = ((unsigned char *) pixel)[0];
	  value.piece[1] = ((unsigned char *) pixel)[1];
	  value.piece[2] = ((unsigned char *) pixel)[2];
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

  return   ((redShift   > 0 ? r << redShift   : r >> -redShift)   &   0xFF0000)
		 | ((greenShift > 0 ? g << greenShift : g >> -greenShift) &     0xFF00)
		 | ((blueShift  > 0 ? b << blueShift  : b >> -blueShift)  &       0xFF)
		 | ((alphaShift > 0 ? a << alphaShift : a >> -alphaShift) & 0xFF000000);
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
	  value.piece[0] = ((unsigned char *) pixel)[0];
	  value.piece[1] = ((unsigned char *) pixel)[1];
	  value.piece[2] = ((unsigned char *) pixel)[2];
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
  unsigned int r = rgba &   0xFF0000;
  unsigned int g = rgba &     0xFF00;
  unsigned int b = rgba &       0xFF;
  unsigned int a = rgba & 0xFF000000;

  int redShift;
  int greenShift;
  int blueShift;
  int alphaShift;
  shift (0xFF0000, 0xFF00, 0xFF, 0xFF000000, redShift, greenShift, blueShift, alphaShift);
  // The shifts must be negated, since we are going from the 24-bit rgb format
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
	  ((unsigned char *) pixel)[0] = value.piece[0];
	  ((unsigned char *) pixel)[1] = value.piece[1];
	  ((unsigned char *) pixel)[2] = value.piece[2];
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


// class PixelFormatRGBAFloat -------------------------------------------------

PixelFormatRGBAFloat::PixelFormatRGBAFloat ()
{
  depth      = 4 * sizeof (float);
  precedence = 5;  // Above everything
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
  unsigned int r = ((unsigned int) (rgbaValues[0] * 255)) << 16;
  unsigned int g = ((unsigned int) (rgbaValues[1] * 255)) <<  8;
  unsigned int b = ((unsigned int) (rgbaValues[2] * 255));
  unsigned int a = ((unsigned int) (rgbaValues[3] * 255)) << 24;
  return a | r | g | b;
}

void
PixelFormatRGBAFloat::getRGBA (void * pixel, float values[]) const
{
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
  float rgbaValues[4];
  rgbaValues[0] = ((rgba &   0xFF0000) >> 16) / 255.0f;
  rgbaValues[1] = ((rgba &     0xFF00) >>  8) / 255.0f;
  rgbaValues[2] = ((rgba &       0xFF)      ) / 255.0f;
  rgbaValues[3] = ((rgba & 0xFF000000) >> 24) / 255.0f;
  linearize (rgbaValues[0]);
  linearize (rgbaValues[1]);
  linearize (rgbaValues[2]);
  // Don't linearize alpha, because it is always linear
  ((float *) pixel)[0] = rgbaValues[0];
  ((float *) pixel)[1] = rgbaValues[1];
  ((float *) pixel)[2] = rgbaValues[2];
  ((float *) pixel)[3] = rgbaValues[3];
}

void
PixelFormatRGBAFloat::setRGBA (void * pixel, float values[]) const
{
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


// class PixelFormatYVYUChar --------------------------------------------------

// Notes...
// YUV <-> RGB conversion matrices are specified by the standards in terms of
// non-linear RGB.  IE: even though the conversion matrices are linear ops,
// they work on non-linear inputs.  Therefore, even though YUV is essentially
// non-linear, it should not be linearized until after it is converted into
// RGB.  The matrices output non-linear RGB.

PixelFormatYVYUChar::PixelFormatYVYUChar ()
{
  depth      = 2;
  precedence = 1;  // above GrayChar and below RGBAChar
  monochrome = false;
  hasAlpha   = false;
}

Image
PixelFormatYVYUChar::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.width     = image.width;
  result.height    = image.height;
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatVYUYChar))
  {
	fromVYUYChar (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatYVYUChar::fromVYUYChar (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  unsigned int * fromPixel = (unsigned int *) image.buffer;
  unsigned int * toPixel = (unsigned int *) result.buffer;
  unsigned int * end = toPixel + result.width * result.height / 2;  // width *must* be a multiple of 2!
  while (toPixel < end)
  {
	register unsigned int p = *fromPixel++;
	*toPixel++ = ((p & 0xFF0000) << 8) | ((p & 0xFF000000) >> 8) | ((p & 0xFF) << 8) | ((p & 0xFF00) >> 8);
  }
}

unsigned int
PixelFormatYVYUChar::getRGBA (void * pixel) const
{
  int y;
  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = (*((unsigned int *) pixel) & 0xFF000000) >> 8;
  }
  else  // on 32-bit word boundary
  {
	y = (*((unsigned int *) pixel) & 0xFF00) << 8;
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

  return 0xFF000000 | (r & 0xFF0000) | ((g >> 8) & 0xFF00) | (b >> 16);
}

unsigned int
PixelFormatYVYUChar::getYUV (void * pixel) const
{
  unsigned int y;
  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = (*((unsigned int *) pixel) & 0xFF000000) >> 8;
  }
  else  // on 32-bit word boundary
  {
	y = (*((unsigned int *) pixel) & 0xFF00) << 8;
  }

  return y | (((unsigned char *) pixel)[0] << 8) | ((unsigned char *) pixel)[2];
}

unsigned char
PixelFormatYVYUChar::getGray (void * pixel) const
{
  return ((unsigned char *) pixel)[1];
}

void
PixelFormatYVYUChar::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF0000) >> 16;
  int g = (rgba &   0xFF00) >> 8;
  int b =  rgba &     0xFF;

  // Y =  0.2989*R +0.5866*G +0.1145*B
  // U = -0.1687*R -0.3312*G +0.5000*B
  // V =  0.5000*R -0.4183*G -0.0816*B
  unsigned int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b,            0), 0xFFFFFF) & 0xFF0000;
  unsigned int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000, 0), 0xFFFFFF) >> 16;
  unsigned int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000, 0), 0xFFFFFF) & 0xFF0000;

  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	*((unsigned int *) pixel) = (y << 8) | v | (*((unsigned int *) pixel) & 0xFF00) | u;
  }
  else  // on 32-bit word boundary
  {
	*((unsigned int *) pixel) = (*((unsigned int *) pixel) & 0xFF000000) | v | (y >> 8) | u;
  }
}

void
PixelFormatYVYUChar::setYUV (void * pixel, unsigned int yuv) const
{
  unsigned int u = (yuv &   0xFF00) >> 8;
  unsigned int v = (yuv &     0xFF) << 16;

  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	*((unsigned int *) pixel) = ((yuv & 0xFF0000) << 8) | v | (*((unsigned int *) pixel) & 0xFF00) | u;
  }
  else  // on 32-bit word boundary
  {
	*((unsigned int *) pixel) = (*((unsigned int *) pixel) & 0xFF000000) | v | ((yuv & 0xFF0000) >> 8) | u;
  }
}


// class PixelFormatVYUYChar --------------------------------------------------

PixelFormatVYUYChar::PixelFormatVYUYChar ()
{
  depth      = 2;
  precedence = 1;  // same as YVYU
  monochrome = false;
  hasAlpha   = false;
}

Image
PixelFormatVYUYChar::filter (const Image & image)
{
  Image result (*this);

  if (*image.format == *this)
  {
	result = image;
	return result;
  }

  result.width     = image.width;
  result.height    = image.height;
  result.timestamp = image.timestamp;

  if (typeid (* image.format) == typeid (PixelFormatYVYUChar))
  {
	fromYVYUChar (image, result);
  }
  else
  {
	fromAny (image, result);
  }

  return result;
}

void
PixelFormatVYUYChar::fromYVYUChar (const Image & image, Image & result) const
{
  result.buffer.grow (result.width * result.height * depth);

  unsigned int * fromPixel = (unsigned int *) image.buffer;
  unsigned int * toPixel = (unsigned int *) result.buffer;
  unsigned int * end = toPixel + result.width * result.height / 2;
  while (toPixel < end)
  {
	register unsigned int p = *fromPixel++;
	*toPixel++ = ((p & 0xFF0000) << 8) | ((p & 0xFF000000) >> 8) | ((p & 0xFF) << 8) | ((p & 0xFF00) >> 8);
  }
}

unsigned int
PixelFormatVYUYChar::getRGBA (void * pixel) const
{
  int y;
  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = *((unsigned int *) pixel) & 0xFF0000;
  }
  else  // on 32-bit word boundary
  {
	y = (*((unsigned int *) pixel) & 0xFF) << 16;
  }
  int u = ((unsigned char *) pixel)[1] - 128;
  int v = ((unsigned char *) pixel)[3] - 128;

  unsigned int r = min (max (y               + 0x166F7 * v, 0), 0xFFFFFF);
  unsigned int g = min (max (y -  0x5879 * u -  0xB6E9 * v, 0), 0xFFFFFF);
  unsigned int b = min (max (y + 0x1C560 * u,               0), 0xFFFFFF);

  return 0xFF000000 | (r & 0xFF0000) | ((g >> 8) & 0xFF00) | (b >> 16);
}

unsigned int
PixelFormatVYUYChar::getYUV (void * pixel) const
{
  unsigned int y;
  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	y = *((unsigned int *) pixel) & 0xFF0000;
  }
  else  // on 32-bit word boundary
  {
	y = (*((unsigned int *) pixel) & 0xFF) << 16;
  }

  return y | (((unsigned char *) pixel)[1] << 8) | ((unsigned char *) pixel)[3];
}

unsigned char
PixelFormatVYUYChar::getGray (void * pixel) const
{
  return * (unsigned char *) pixel;
}

void
PixelFormatVYUYChar::setRGBA (void * pixel, unsigned int rgba) const
{
  int r = (rgba & 0xFF0000) >> 16;
  int g = (rgba &   0xFF00) >> 8;
  int b =  rgba &     0xFF;

  unsigned int y =  min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b,            0), 0xFFFFFF) & 0xFF0000;
  unsigned int u = (min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000, 0), 0xFFFFFF) & 0xFF0000) >> 8;
  unsigned int v = (min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000, 0), 0xFFFFFF) & 0xFF0000) << 8;

  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	*((unsigned int *) pixel) = v | y | u | (*((unsigned int *) pixel) & 0xFF);
  }
  else  // on 32-bit word boundary
  {
	*((unsigned int *) pixel) = v | (*((unsigned int *) pixel) & 0xFF0000) | u | (y >> 16);
  }
}

void
PixelFormatVYUYChar::setYUV (void * pixel, unsigned int yuv) const
{
  unsigned int u = (yuv &   0xFF00);
  unsigned int v = (yuv &     0xFF) << 24;

  if (((unsigned int) pixel) % 4)  // in middle of 32-bit word
  {
	pixel = & ((short *) pixel)[-1];  // Move backward in memory 16 bits.
	*((unsigned int *) pixel) = v | (yuv & 0xFF0000) | u | (*((unsigned int *) pixel) & 0xFF);
  }
  else  // on 32-bit word boundary
  {
	*((unsigned int *) pixel) = v | (*((unsigned int *) pixel) & 0xFF0000) | u | ((yuv & 0xFF0000) >> 16);
  }
}


// class PixelFormatHLSFloat --------------------------------------------------

const float root32 = sqrtf (3.0f) / 2.0f;
const float onesixth = 1.0f / 6.0f;
const float onethird = 1.0f / 3.0f;
const float twothirds = 2.0f / 3.0f;

PixelFormatHLSFloat::PixelFormatHLSFloat ()
{
  depth      = 3 * sizeof (float);
  precedence = 5;  // on par with RGBAFloat
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
  unsigned int r = ((unsigned int) (rgbaValues[0] * 255)) << 16;
  unsigned int g = ((unsigned int) (rgbaValues[1] * 255)) <<  8;
  unsigned int b = ((unsigned int) (rgbaValues[2] * 255));
  unsigned int a = ((unsigned int) (rgbaValues[3] * 255)) << 24;
  return a | r | g | b;
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
  rgbaValues[0] = ((rgba &   0xFF0000) >> 16) / 255.0f;
  rgbaValues[1] = ((rgba &     0xFF00) >>  8) / 255.0f;
  rgbaValues[2] = ((rgba &       0xFF)      ) / 255.0f;
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
