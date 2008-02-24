/*
Author: Fred Rothganger
Created 2/24/2006 to perform regression testing on the image library.


Revisions 1.1 thru 1.13 Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.13  2008/02/24 14:20:35  Fred
Fix MSVC 2005 compilation issue.

Revision 1.12  2007/08/26 12:10:29  Fred
Update SNL copyright notice.

Revision 1.11  2007/08/26 12:09:08  Fred
Fine-tune thresholds to work with dublicate().  Use finer granularity in float
thresholds when testing accessors.  Better calculation of bit-loss.

Time the entire pixel format test in order to compare speed cost for
implementing dublicate().  It seems to only cost about 2.5% on whole image
conversions (didn't test accessors because they take too long to run).

Revision 1.10  2007/08/13 04:23:00  Fred
Add test for Convolution1D.

Move threshold constants outside of testFormat() so they can be used by other
parts of program.  Make additional stride 13 so that no naturally occuring
format fits in it by coincidence.  When source and dest formats are both YUV,
average the UV values for a macro pixel directly, rather than converting to
RGB, averaging, and converting back to UV.

Add UYVYUYVYYYYY to format tests.

Force generation of timestamp in video file.  Change assert() about image size
to an explicit test.  Use thresholdLumaAccessor rather than a magic number
for luma consistency check when reading video.

Revision 1.9  2007/03/23 02:32:07  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/04/08 15:16:47  Fred
Strip "Char" from all YUV-type formats.  It is ugly, and doesn't add any
information in this case.

Revision 1.7  2006/04/08 14:46:52  Fred
Add PixelFormatUYYVYY.

Revision 1.6  2006/04/07 04:32:49  Fred
Finish color version of PixelFormat accessor tests.

Revision 1.5  2006/04/06 03:46:06  Fred
Create some weird RGBABits formats to ensure that all Bits converters are
tested.  Added a case to the converter test to soften thresholds when bits are
lost.

Minor fixes to be consistent with the idea that all verification of conversions
is relative to the "fromImage", not the original test image.  Specifically, no
need to check if fromImage is monochrome, as it doesn't represent any loss of
information.

Revision 1.4  2006/04/05 02:37:13  Fred
Add BGRChar to list of formats to test.

Modify fromImage so it has a stride different from width.  Requires converters
to be more general.

Add test for accessors to monochrome formats.  Still need to add accessors to
color formats.

Revision 1.3  2006/04/02 02:36:04  Fred
Add test for PixelFormats.  Currently it just tests conversions, but plan to
test accessors as well.

Revision 1.2  2006/03/20 05:42:39  Fred
Output results and errors to cout rather than cerr.  This allows one to use
pipes to select just the results of the test and avoid seeing the more detailed
traces of activities.

Revision 1.1  2006/02/25 22:50:55  Fred
Replace junk test.cc by a real regression test, complete with
self-verification.  The old test.cc is now called junk.cc
-------------------------------------------------------------------------------
*/


#include "fl/image.h"
#include "fl/convolve.h"
#include "fl/canvas.h"
#include "fl/descriptor.h"
#include "fl/random.h"
#include "fl/video.h"
#include "fl/interest.h"
#include "fl/time.h"

#include <float.h>

// For debugging only
#include "fl/slideshow.h"


using namespace std;
using namespace fl;


void
testAbsoluteValue (Image & image)
{
  // Fill with some negative and some positive numbers
  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  float value = pow (-1.0f, (float) x) * x / image.width;
	  image.setGray (x, y, value);
	}
  }

  image *= AbsoluteValue ();

  // Verify all numbers are now positive
  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  float pixel;
	  image.getGray (x, y, pixel);
	  float expected = (float) x / image.width;
	  if (fabs (pixel - expected) > 1e-6)
	  {
		cout << x << " " << y << " " << pixel << " - " << expected << " = " << pixel - expected << endl;
		throw "AbsoluteValue failed";
	  }
	}
  }
}

void
testTransform (Image & image)
{
  CanvasImage ci (image);
  ci.drawFilledRectangle (Point (300, 200), Point (500, 300));

  // 8-dof
  Matrix<double> S (3, 3);
  S.identity ();
  S(0,2) = -400;
  S(1,2) = -250;
  S(0,0) = 1;
  S(1,1) = 1;
  S(2,0) = 1e-4;
  S(2,1) = 0;
  Transform t8 (S);
  t8.setWindow (0, 0, 200, 100);
  Image disp = image * t8;
  assert (disp.getGray (100, 50)  &&  ! disp.getGray (199, 0)  &&  ! disp.getGray (199, 99));

  // 6-dof
  S(2,0) = 0;
  Transform t6 (S);
  t6.setWindow (0, 0, 200, 100);
  disp = image * t6;
  assert (disp.width == 200  &&  disp.height == 100);
  for (int y = 0; y < 100; y++)
  {
	for (int x = 0; x < 200; x++)
	{
	  if (disp.getGray (x, y) < 254)
	  {
		cout << x << " " << y << " expected white, got " << (int) disp.getGray (x, y) << endl;
		throw "Tranform fails";
	  }
	}
  }
}

/**
   This is kind of a mini ConvolutionDiscrete1D.  It is meant to be a reliable
   comparison point for that code.
 **/
class Convoluter1D
{
public:
  Convoluter1D (const Image & image, const Image & kernel, BorderMode mode, bool horizontal)
  {
	doubleImage = image * GrayDouble;  // assumes image is not double already, and that PixelFormat conversion results in packed buffer with gap between rows
	this->image  = (double *) ((PixelBufferPacked *) doubleImage.buffer) ->memory;
	doubleKernel = kernel * GrayDouble;
	this->kernel = (double *) ((PixelBufferPacked *) doubleKernel.buffer)->memory;

	imageWidth = image.width;
	lastX = image.width - 1;
	lastY = image.height - 1;
	last = kernel.width - 1;
	mid = kernel.width / 2;
	offset = last - mid;

	this->mode = mode;
	this->horizontal = horizontal;
	stride = horizontal ? 1 : image.width;
  }

  double response (int x, int y)
  {
	int low;
	int high;
	if (horizontal)
	{
	  low  = max (0,    x + mid - lastX);
	  high = min (last, x + mid);
	}
	else  // vertical
	{
	  low  = max (0,    y + mid - lastY);
	  high = min (last, y + mid);
	}

	if (low > 0  ||  high < last)
	{
	  if (mode == Crop  ||  mode == Undefined) return NAN;
	  if (mode == ZeroFill) return 0;
	  if (mode == Copy) return image[y * imageWidth + x];
	}

	double * a = kernel + low;
	double * b = image + (y * imageWidth + x);
	b += (mid - low) * stride;

	double result = 0;
	double weight = 0;
	for (int i = low; i <= high; i++)
	{
	  result += *a * *b;
	  weight += *a++;
	  b -= stride;
	}
	if (mode == Boost  &&  (low  ||  high < last)) result /= weight;
	return result;
  }

  Image doubleImage;
  double * image;
  Image doubleKernel;
  double * kernel;
  int imageWidth;
  int lastX;
  int lastY;
  int last;
  int mid;
  int offset;
  int stride;
  BorderMode mode;
  bool horizontal;
};

void
testConvolutionDiscrete1D (const Image & image, const ConvolutionDiscrete1D & kernel)
{
  cerr << typeid (*kernel.format).name () << " image=" << image.width << "x" << image.height << " kernel=" << kernel.width << " mode=" << kernel.mode << " direction=" << kernel.direction << endl;

  const double threshold = 3e-6;  // for regular C version
  //const double threshold = 1e-3; // for assembly version (it must be broken somewhere, but nearly correct)

  Image result = image * kernel;
  Convoluter1D conv (image, kernel, kernel.mode, kernel.direction == Horizontal);

  // Check dimensions
  int expectedWidth = image.width;
  int expectedHeight = image.height;
  int offsetX = 0;
  int offsetY = 0;
  if (kernel.mode == Crop)
  {
	if (kernel.direction == Horizontal)
	{
	  expectedWidth = max (0, expectedWidth - conv.last);
	  offsetX = conv.offset;
	}
	else
	{
	  expectedHeight = max (0, expectedHeight - conv.last);
	  offsetY = conv.offset;
	}
  }
  if (result.width != expectedWidth  ||  result.height != expectedHeight)
  {
	cout << "Expected size = " << expectedWidth << "x" << expectedHeight << "   got " << result.width << "x" << result.height << endl;
	throw "Convolution1D fails";
  }

  // Check contents
  for (int y = 0; y < expectedHeight; y++)
  {
	int fromY = y + offsetY;
	for (int x = 0; x < expectedWidth; x++)
	{
	  int fromX = x + offsetX;

	  double t = conv.response (fromX, fromY);
	  if (isnan (t)) continue;  // indicates border when mode == Undefined

	  float fr;
	  result.getGray (x, y, fr);
	  double e = fabs (fr - t);
	  if (e > threshold)
	  {
		cout << "Unexpected result: " << x << " " << y << " " << e << " = |" << fr << " - " << t << "|" << endl;
		throw "Convolution1D fails";
	  }

	  double dr = kernel.response (image, Point (fromX, fromY));
	  e = fabs (dr - t);
	  if (e > threshold)
	  {
		cout << "Unexpected response: " << x << " " << y << " " << e << " = |" << dr << " - " << t << "|" << endl;
		throw "Convolution1D fails";
	  }
	}
	//cerr << ".";
  }
  //cerr << endl;
}

const float thresholdRatio         = 1.01f;
const float thresholdDifference    = 0.013f;
const int   thresholdLuma          = 1;
const int   thresholdChroma        = 1;
const int   thresholdLumaClipped   = 3;
const int   thresholdChromaClipped = 4;
const int   thresholdLumaAccessor  = 2;  // higher because errors in YUV<->RGB conversion get magnified by gray conversion

extern void reshapeBuffer (Pointer & memory, int oldStride, int newStride, int newHeight);

void
testFormat (const Image & test, const vector<fl::PixelFormat *> & formats, fl::PixelFormat * targetFormat)
{
  cerr << typeid (*targetFormat).name () << endl;

  // Convert from every other format into the target one.
  for (int i = 0; i < formats.size (); i++)
  {
	fl::PixelFormat * fromFormat = formats[i];
	cerr << "  from " << typeid (*fromFormat).name () << endl;

	Image fromImage = test * *fromFormat;
	// Make stride larger than width to verify PixelBufferPacked operation
	// and correct looping in converters.
	int skew = 13;  // 16 works, but doesn't test hard enough
	if (PixelBufferPacked * pbp = (PixelBufferPacked *) fromImage.buffer)
	{
	  // The advantage of reshapeBuffer() is that we can specify the stide directly in bytes...
	  reshapeBuffer (pbp->memory, pbp->stride, pbp->stride + skew, fromImage.height);
	  pbp->stride += skew;
	}
	else
	{
	  // ...whereas resize() can only change the number of pixels.
	  if (PixelFormatYUV * pfyuv = (PixelFormatYUV *) fromFormat)
	  {
		skew = (skew / pfyuv->ratioH + 1) * pfyuv->ratioH;
	  }
	  fromImage.buffer->resize (fromImage.width + skew, fromImage.height, *fromFormat, true);
	}

	Image toImage = fromImage * *targetFormat;

	if (toImage.width != test.width  ||  toImage.height != test.height)
	{
	  throw "PixelFormat conversion failed to produce same size of image";
	}

	// Verify
	if (targetFormat->monochrome)
	{
	  bool approximate = true;
	  for (int y = 0; y < fromImage.height; y++)
	  {
		for (int x = 0; x < fromImage.width; x++)
		{
		  float original;
		  if (approximate)
		  {
			fromImage.getGray (x, y, original);
		  }
		  else
		  {
			float rgba[4];
			fromImage.getRGBA (x, y, rgba);
			original = rgba[0] * 0.2126 + rgba[1] * 0.7152 + rgba[2] * 0.0722;
		  }
		  float converted;
		  toImage.getGray (x, y, converted);
		  float ratio = original / converted;
		  if (ratio < 1.0f) ratio = 1.0f / ratio;
		  float difference = fabs (original - converted);
		  if (ratio > thresholdRatio  &&  difference > thresholdDifference)
		  {
			if (approximate)
			{
			  approximate = false;
			  cerr << "    switching to exact gray" << endl;
			  // Technically, we let this one pixel go by without re-evaluating
			  // to see if it matches the exact gray level.  However, if one
			  // pixel is bad, almost certainly a bunch of others will be also.
			}
			else
			{
			  cout << x << " " << y << " unexpected change in gray level: " << difference << " = | " << original << " -> " << converted << " |" << endl;
			  throw "PixelFormat fails";
			}
		  }
		}
	  }
	}
	else if (PixelFormatYUV * pfyuv = dynamic_cast<PixelFormatYUV *> (targetFormat))
	{
	  int ratioH = pfyuv->ratioH;
	  int ratioV = pfyuv->ratioV;
	  //cerr << "    ratioH,V = " << ratioH << " " << ratioV << endl;
	  PixelFormatYUV * frompfyuv = dynamic_cast<PixelFormatYUV *> (fromFormat);

	  const unsigned int shift = 16 + (int) rint (log ((double) ratioH * ratioV) / log (2.0));
	  const int roundup = 0x8000 << (shift - 16);
	  const int bias = 0x808 << (shift - 4);  // also includes roundup
	  const int maximum = (~(unsigned int) 0) >> (24 - shift);

	  for (int y = 0; y < fromImage.height; y += ratioV)
	  {
		for (int x = 0; x < fromImage.width; x += ratioH)
		{
		  // "Clipping" occurs when the average UV value for a block of pixels,
		  // when combined with the Y value from a particular pixel in the
		  // block, results in a YUV value that falls outside the RGB volume
		  // in YUV space.  When converting this YUV to RGB, the resulting
		  // values are clipped to their min or max, producing errors larger
		  // than the usual threshold.
		  int u = 0;
		  int v = 0;
		  if (frompfyuv)  // average the UV values directly
		  {
			for (int yy = y; yy < y + ratioV; yy++)
			{
			  for (int xx = x; xx < x + ratioH; xx++)
			  {
				unsigned int yuv = fromImage.getYUV (xx, yy);
				u += (yuv & 0xFF00) << 8;
				v += (yuv &   0xFF) << 16;
			  }
			}
			u = (u + roundup) >> shift;
			v = (v + roundup) >> shift;
		  }
		  else  // average RGB values and then convert to UV
		  {
			int r = 0;
			int g = 0;
			int b = 0;
			for (int yy = y; yy < y + ratioV; yy++)
			{
			  for (int xx = x; xx < x + ratioH; xx++)
			  {
				unsigned int rgba = fromImage.getRGBA (xx, yy);
				r +=  rgba             >> 24;
				g += (rgba & 0xFF0000) >> 16;
				b += (rgba &   0xFF00) >>  8;
			  }
			}
			u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
			v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
		  }
		  int tu = u - 128;
		  int tv = v - 128;

		  // Check for clipping
		  int clip = 0;
		  for (int yy = y; yy < y + ratioV; yy++)
		  {
			for (int xx = x; xx < x + ratioH; xx++)
			{
			  unsigned int oyuv = fromImage.getYUV (xx, yy);
			  int ty = oyuv  &  0xFF0000;
			  int r = (ty                + 0x166F7 * tv + 0x8000) >> 16;
			  int g = (ty -  0x5879 * tu -  0xB6E9 * tv + 0x8000) >> 16;
			  int b = (ty + 0x1C560 * tu                + 0x8000) >> 16;
			  clip = max (clip, 0 - r);
			  clip = max (clip, 0 - g);
			  clip = max (clip, 0 - b);
			  clip = max (clip, r - 0xFF);
			  clip = max (clip, g - 0xFF);
			  clip = max (clip, b - 0xFF);
			}
		  }
		  //if (clip) cerr << "    " << x << " " << y << " clip = " << clip << endl;
		  int tLuma   = clip ? thresholdLumaClipped   : thresholdLuma;
		  int tChroma = clip ? thresholdChromaClipped : thresholdChroma;
		  if (PixelFormatPlanarYCbCr * pfycbcr = dynamic_cast<PixelFormatPlanarYCbCr *> (targetFormat))
		  {
			tChroma = max (tChroma, 2);  // The additional quantization due to shortened excursion of YCbCr sometimes produces a 2-level difference.
		  }

		  // Check consistency
		  for (int yy = y; yy < y + ratioV; yy++)
		  {
			for (int xx = x; xx < x + ratioH; xx++)
			{
			  unsigned int oyuv = fromImage.getYUV (xx, yy);
			  unsigned int cyuv = toImage.getYUV (xx, yy);
			  int oy = oyuv >> 16;
			  int cy = cyuv >> 16;
			  int error = abs (oy - cy);
			  if (error > tLuma)
			  {
				cout << "    " << xx << " " << yy << " unexpected change in luma: " << error << " = | " << oy << " - " << cy << " |  clip=" << clip << endl;
				throw "PixelFormat fails";
			  }

			  int cu = (cyuv & 0xFF00) >> 8;
			  int cv = (cyuv &   0xFF);
			  error = max (abs (u - cu), abs (v - cv));
			  if (error > tChroma)
			  {
				cout << "    " << xx << " " << yy << " unexpected change in chroma: " << error << " = |(" << u << " " << v << ") - (" << cu << " " << cv << ")|  clip=" << clip << endl;
				throw "PixelFormat fails";
			  }
			}
		  }

		}
	  }

	}
	else if (const PixelFormatRGBABits * pfbits = dynamic_cast<const PixelFormatRGBABits *> (targetFormat))
	{
	  // Determine mask for significant bits, that is, bits that can/should be
	  // compared.  Eventually change this to determine threshold based on bits
	  // lost in each channel.
	  int rbits = min (8, pfbits->redBits);
	  int gbits = min (8, pfbits->greenBits);
	  int bbits = min (8, pfbits->blueBits);
	  int abits = min (8, pfbits->alphaBits ? pfbits->alphaBits : 8);  // if no alpha channel, then expect it to be faked as 0xFF by getRGBA()
	  if (pfbits = dynamic_cast<const PixelFormatRGBABits *> (fromFormat))
	  {
		rbits = min (rbits, pfbits->redBits);
		gbits = min (gbits, pfbits->greenBits);
		bbits = min (bbits, pfbits->blueBits);
		abits = min (abits, pfbits->alphaBits ? pfbits->alphaBits : 8);
	  }
	  rbits = 8 - rbits;
	  gbits = 8 - gbits;
	  bbits = 8 - bbits;
	  abits = 8 - abits;
	  unsigned int mask =   ((0xFF >> rbits) << (24 + rbits))
	                      | ((0xFF >> gbits) << (16 + gbits))
	                      | ((0xFF >> bbits) << ( 8 + bbits))
	                      | ((0xFF >> abits) <<       abits );

	  for (int y = 0; y < fromImage.height; y++)
	  {
		for (int x = 0; x < fromImage.width; x++)
		{
		  unsigned int original  = fromImage.getRGBA (x, y);
		  unsigned int converted = toImage.getRGBA (x, y);
		  original &= mask;
		  converted &= mask;

		  int r =  original             >> 24;
		  int g = (original & 0xFF0000) >> 16;
		  int b = (original &   0xFF00) >>  8;
		  int a =  original &     0xFF;

		  int cr =  converted             >> 24;
		  int cg = (converted & 0xFF0000) >> 16;
		  int cb = (converted &   0xFF00) >>  8;
		  int ca =  converted &     0xFF;

		  int er = abs (r - cr);
		  int eg = abs (g - cg);
		  int eb = abs (b - cb);
		  int ea = abs (a - ca);
		  int error = max (er, eg, eb, ea);
		  if (error > thresholdLuma)
		  {
			cout << x << " " << y << " unexpected change in color value: " << error << " " << hex << original << " -> " << converted << endl;
			cout << "  mask = " << mask << "  " << fromImage.getRGBA (x, y) << " -> " << toImage.getRGBA (x, y) << dec << endl;
			throw "PixelFormat fails";
		  }
		}
	  }
	}
	else
	{
	  Vector<float> original (4);
	  Vector<float> converted (4);
	  for (int y = 0; y < fromImage.height; y++)
	  {
		for (int x = 0; x < fromImage.width; x++)
		{
		  fromImage.getRGBA (x, y, &original[0]);
		  toImage.getRGBA (x, y, &converted[0]);
		  for (int r = 0; r < original.rows (); r++)
		  {
			float ratio = original[r] / converted[r];
			if (ratio < 1.0f) ratio = 1.0f / ratio;
			float difference = fabs (original[r] - converted[r]);
			if (ratio > thresholdRatio  &&  difference > thresholdDifference)
			{
			  cout << x << " " << y << " unexpected change in color value: " << ratio << endl << original << endl << converted << endl;
			  cout << hex << fromImage.getRGBA (x, y) << dec << endl;
			  throw "PixelFormat fails";
			}
		  }
		}
	  }
	}
  }


  // Verify all accessors
  cerr << "  checking accessors";

  //   Expectations for alpha channel  (independent of whether format is color or monochrome)
  int tAlpha = thresholdLuma;
  if (const PixelFormatRGBABits * pfbits = dynamic_cast<const PixelFormatRGBABits *> (targetFormat))
  {
	int abits = 8 - min (8, pfbits->alphaBits ? pfbits->alphaBits : 8);
	tAlpha = max (tAlpha, 1 << abits);
  }

  Image target (16, 16, *targetFormat);
  if (targetFormat->monochrome)
  {
	bool approximate = true;
	Vector<float> rgbaFloatIn (4);
	rgbaFloatIn[3] = 1.0f;
	for (int r = 0; r < 256; r++)
	{
	  rgbaFloatIn[0] = fl::PixelFormat::lutChar2Float[r];
	  for (int g = 0; g < 256; g++)
	  {
		rgbaFloatIn[1] = fl::PixelFormat::lutChar2Float[g];
		for (int b = 0; b < 256; b++)
		{
		  rgbaFloatIn[2] = fl::PixelFormat::lutChar2Float[b];

		  // Expectations
		  float grayExact = rgbaFloatIn[0] * 0.2126f + rgbaFloatIn[1] * 0.7152f + rgbaFloatIn[2] * 0.0722f;
		  float grayFloat;
		  int grayChar;
		  if (approximate)
		  {
			grayChar = (((76 << 8) * r + (150 << 8) * g + (29 << 8) * b) / 255 + 0x80) >> 8;
			grayFloat = fl::PixelFormat::lutChar2Float[grayChar];
		  }
		  else
		  {
			grayFloat = grayExact;
			grayChar = fl::PixelFormat::lutFloat2Char[(unsigned short) (65535 * grayFloat)];
		  }

		  // get/setRGBA
		  unsigned int rgbaIn = ((unsigned int) r << 24) | (g << 16) | (b << 8) | 0xFF;
		  target.setRGBA (0, 0, rgbaIn);
		  unsigned int rgbaOut = target.getRGBA (0, 0);
		  bool allEqual =    rgbaOut >> 24 == (rgbaOut & 0xFF0000) >> 16
		                  && rgbaOut >> 24 == (rgbaOut & 0xFF00) >> 8;
		  if (! allEqual)
		  {
			cout << r << " " << g << " " << b << " getRGBA returned non-gray value: " << hex << grayChar << " " << rgbaOut << dec << endl;
			throw "PixelFormat fails";
		  }
		  if (abs (grayChar - (int) (rgbaOut >> 24)) > thresholdLuma)
		  {
			if (approximate)
			{
			  approximate = false;
			  cerr << "    switching to exact gray" << endl;
			  continue;
			}
			cout << r << " " << g << " " << b << " getRGBA returned unexpected gray value: " << hex << grayChar << " " << rgbaOut << dec << endl;
			throw "PixelFormat fails";
		  }

		  // get/setRGBA(float)
		  target.setRGBA (0, 0, &rgbaFloatIn[0]);
		  Vector<float> rgbaFloatOut (4);
		  target.getRGBA (0, 0, &rgbaFloatOut[0]);
		  allEqual =    rgbaFloatOut[0] == rgbaFloatOut[1]
		             && rgbaFloatOut[0] == rgbaFloatOut[2];
		  if (! allEqual)
		  {
			cout << r << " " << g << " " << b << " getRGBA returned non-gray value: " << hex << grayChar << dec << endl << rgbaFloatOut << endl;
			throw "PixelFormat fails";
		  }
		  float ratio = rgbaFloatOut[0] / grayFloat;
		  if (ratio < 1.0f) ratio = 1.0f / ratio;
		  float difference = fabs (rgbaFloatOut[0] - grayFloat);
		  if (ratio > thresholdRatio  &&  difference > thresholdDifference)
		  {
			cout << r << " " << g << " " << b << " getRGBA returned unexpected gray value: " << grayFloat << " " << rgbaFloatOut[0] << endl;
			throw "PixelFormat fails";
		  }

		  // get/setXYZ
		  Vector<float> xyzIn (3);
		  xyzIn[0] = 0.4124f * rgbaFloatIn[0] + 0.3576f * rgbaFloatIn[1] + 0.1805f * rgbaFloatIn[2];
		  xyzIn[1] = grayExact;
		  xyzIn[2] = 0.0193f * rgbaFloatIn[0] + 0.1192f * rgbaFloatIn[1] + 0.9505f * rgbaFloatIn[2];
		  target.setXYZ (0, 0, &xyzIn[0]);
		  Vector<float> xyzOut (3);
		  target.getXYZ (0, 0, &xyzOut[0]);
		  if (xyzOut[0] != 0  ||  xyzOut[2] != 0)
		  {
			cout << r << " " << g << " " << b << " getXYZ returned non-gray value: " << xyzOut << endl;
			throw "PixelFormat failed";
		  }
		  ratio = xyzOut[1] / grayExact;
		  if (ratio < 1.0f) ratio = 1.0f / ratio;
		  difference = xyzOut[1] - grayExact;
		  if (ratio > thresholdRatio  &&   difference > thresholdDifference)
		  {
			cout << r << " " << g << " " << b << " getXYZ returned unexpected gray value: " << grayExact << " " << xyzOut[1] << endl;
			throw "PixelFormat failed";
		  }

		  // get/setYUV
		  unsigned int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
		  unsigned int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
		  unsigned int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
		  unsigned int yuvIn = y | (u >> 8) | (v >> 16);
		  target.setYUV (0, 0, yuvIn);
		  unsigned int yuvOut = target.getYUV (0, 0);
		  u = (yuvOut & 0xFF00) >> 8;
		  v =  yuvOut &   0xFF;
		  if (abs ((int) u - 128) > thresholdChroma  ||  abs ((int) v - 128) > thresholdChroma)
		  {
			cout << r << " " << g << " " << b << " getYUV returned non-gray value: " << hex << yuvOut << dec << endl;
			throw "PixelFormat failed";
		  }
		  y = yuvOut >> 16;
		  if (abs (grayChar - (int) y) > thresholdLumaAccessor)
		  {
			cout << r << " " << g << " " << b << " getYUV returned unexpected gray value: " << hex << grayChar << " " << yuvOut << dec << endl;
			throw "PixelFormat failed";
		  }

		  // get/setGray
		  target.setGray (0, 0, (unsigned char) grayChar);
		  int grayOut = target.getGray (0, 0);
		  if (abs (grayOut - grayChar) > thresholdLuma)
		  {
			cout << r << " " << g << " " << b << " getGray returned unexpected value: " << hex << grayChar << " " << grayOut << dec << endl;
			throw "PixelFormat failed";
		  }

		  // get/setGray(float)
		  target.setGray (0, 0, grayFloat);
		  float grayOutFloat;
		  target.getGray (0, 0, grayOutFloat);
		  ratio = grayOutFloat / grayFloat;
		  if (ratio < 1.0f) ratio = 1.0f / ratio;
		  difference = fabs (grayOutFloat - grayFloat);
		  if (ratio > thresholdRatio  &&   difference > thresholdDifference)
		  {
			cout << r << " " << g << " " << b << " getGray returned unexpected value: " << grayFloat << " " << grayOutFloat << endl;
			throw "PixelFormat failed";
		  }
		}
	  }
	  cerr << ".";
	}
	cerr << endl;
  }
  else
  {
	// Expectations
	float tRatio      = thresholdRatio;
	int tLuma         = thresholdLuma;
	int tChroma       = thresholdChroma;
	int tRed          = thresholdChroma;
	int tGreen        = thresholdChroma;
	int tBlue         = thresholdChroma;
	Vector<float> tDifference (7);  // RGBAXYZ
	tDifference.clear (thresholdDifference);

	if (PixelFormatPlanarYCbCr * pfycbcr = dynamic_cast<PixelFormatPlanarYCbCr *> (targetFormat))
	{
	  tChroma = max (tChroma, 2);
	  tRed    = tChroma;
	  tGreen  = tChroma;
	  tBlue   = tChroma;
	  tRatio  = max (tRatio,  1.035f);
	  tDifference.clear (1.0 - fl::PixelFormat::lutChar2Float[255 - tChroma]);
	  tDifference[3] = tAlpha / 255.0;
	}
	else if (const PixelFormatHLSFloat * hls = dynamic_cast<const PixelFormatHLSFloat *> (targetFormat))
	{
	  // Very loose thresholds because HLS has abysmal color fidelity due to
	  // singularities.
	  tChroma = max (tChroma, 6);
	  tRed    = tChroma;
	  tGreen  = tChroma;
	  tBlue   = tChroma;
	  tLuma   = max (tLuma, 6);
	  tDifference.clear (max (thresholdDifference, 0.06f));
	  tDifference[3] = tAlpha / 255.0;
	}
	else if (const PixelFormatRGBABits * pfbits = dynamic_cast<const PixelFormatRGBABits *> (targetFormat))
	{
	  int rbits = 8 - min (8, pfbits->redBits);
	  int gbits = 8 - min (8, pfbits->greenBits);
	  int bbits = 8 - min (8, pfbits->blueBits);
	  int xbits = (int) ceil (0.4124f * rbits + 0.3576f * gbits + 0.1805f * bbits);
	  int ybits = (int) ceil (0.2126f * rbits + 0.7152f * gbits + 0.0722f * bbits);
	  int zbits = (int) ceil (0.0193f * rbits + 0.1192f * gbits + 0.9505f * bbits);

	  tRed    = max (tRed,   1 << rbits);
	  tGreen  = max (tGreen, 1 << gbits);
	  tBlue   = max (tBlue,  1 << bbits);
	  tChroma = max (tRed, max (tGreen, tBlue));
	  tLuma   = max (tLuma,  1 << ybits);

	  tDifference[0] = 1.0 - fl::PixelFormat::lutChar2Float[255 - tRed];
	  tDifference[1] = 1.0 - fl::PixelFormat::lutChar2Float[255 - tGreen];
	  tDifference[2] = 1.0 - fl::PixelFormat::lutChar2Float[255 - tBlue];
	  tDifference[3] = tAlpha / 255.0;
	  tDifference[4] = 1.0 - fl::PixelFormat::lutChar2Float[255 - (1 << xbits)];
	  tDifference[5] = 1.0 - fl::PixelFormat::lutChar2Float[255 - tLuma];
	  tDifference[6] = 1.0 - fl::PixelFormat::lutChar2Float[255 - (1 << zbits)];

	  //cerr << "thresholds: " << tRed << " " << tGreen << " " << tBlue << " " << tAlpha << " " << tChroma << " " << tLuma << endl;
	  //cerr << "difference: " << tDifference << endl;
	  //cerr << "bits: " << rbits << " " << gbits << " " << bbits << " " << xbits << " " << ybits << " " << zbits << endl;
	}

	Vector<float> rgbaFloatIn (4);
	rgbaFloatIn[3] = 1.0f;
	for (int r = 0; r < 256; r++)
	{
	  rgbaFloatIn[0] = fl::PixelFormat::lutChar2Float[r];
	  for (int g = 0; g < 256; g++)
	  {
		rgbaFloatIn[1] = fl::PixelFormat::lutChar2Float[g];
		for (int b = 0; b < 256; b++)
		{
		  rgbaFloatIn[2] = fl::PixelFormat::lutChar2Float[b];

		  // get/setRGBA
		  unsigned int rgbaIn = ((unsigned int) r << 24) | (g << 16) | (b << 8) | 0xFF;
		  target.setRGBA (0, 0, rgbaIn);
		  unsigned int rgbaOut = target.getRGBA (0, 0);
		  int cr =  rgbaOut             >> 24;
		  int cg = (rgbaOut & 0xFF0000) >> 16;
		  int cb = (rgbaOut &   0xFF00) >>  8;
		  int ca =  rgbaOut &     0xFF;
		  int er = abs (r    - cr);
		  int eg = abs (g    - cg);
		  int eb = abs (b    - cb);
		  int ea = abs (0xFF - ca);
		  if (er > tRed  ||  eg > tGreen  ||  eb > tBlue  ||  ea > tAlpha)
		  {
			cout << r << " " << g << " " << b << " getRGBA returned unexpected value: {";
			cout << er << " " << eg << " " << eb << " " << ea << "} > {" << tRed << " " << tGreen << " " << tBlue << " " << tAlpha << "} ";
			cout << hex << " " << rgbaIn << " -> " << rgbaOut << dec << endl;
			throw "PixelFormat fails";
		  }

		  // get/setRGBA(float)
		  target.setRGBA (0, 0, &rgbaFloatIn[0]);
		  Vector<float> rgbaFloatOut (4);
		  target.getRGBA (0, 0, &rgbaFloatOut[0]);
		  for (int j = 0; j < 4; j++)
		  {
			float ratio = rgbaFloatOut[j] / rgbaFloatIn[j];
			if (ratio < 1.0f) ratio = 1.0f / ratio;
			float difference = fabs (rgbaFloatOut[j] - rgbaFloatIn[j]);
			if (ratio > tRatio  &&  difference > tDifference[j])
			{
			  cout << r << " " << g << " " << b << " getRGBA returned unexpected value: " << ratio << endl;
			  cout << rgbaFloatIn << endl;
			  cout << rgbaFloatOut << endl;
			  throw "PixelFormat fails";
			}
		  }

		  // get/setXYZ
		  Vector<float> xyzIn (3);
		  xyzIn[0] = 0.4124f * rgbaFloatIn[0] + 0.3576f * rgbaFloatIn[1] + 0.1805f * rgbaFloatIn[2];
		  xyzIn[1] = 0.2126f * rgbaFloatIn[0] + 0.7152f * rgbaFloatIn[1] + 0.0722f * rgbaFloatIn[2];
		  xyzIn[2] = 0.0193f * rgbaFloatIn[0] + 0.1192f * rgbaFloatIn[1] + 0.9505f * rgbaFloatIn[2];
		  target.setXYZ (0, 0, &xyzIn[0]);
		  Vector<float> xyzOut (3);
		  target.getXYZ (0, 0, &xyzOut[0]);
		  for (int j = 0; j < 3; j++)
		  {
			float ratio = xyzOut[j] / xyzIn[j];
			if (ratio < 1.0f) ratio = 1.0f / ratio;
			float difference = fabs (xyzOut[j] - xyzIn[j]);
			if (ratio > tRatio  &&  difference > tDifference[4+j])
			{
			  cout << r << " " << g << " " << b << " getXYZ returned unexpected value: " << ratio << endl << xyzIn << endl << xyzOut << endl;
			  throw "PixelFormat fails";
			}
		  }

		  // get/setYUV
		  int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
		  int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
		  int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) & 0xFF0000;
		  int yuvIn = y | (u >> 8) | (v >> 16);
		  target.setYUV (0, 0, yuvIn);
		  int yuvOut = target.getYUV (0, 0);
		  y >>= 16;
		  u >>= 16;
		  v >>= 16;
		  int cy =  yuvOut           >> 16;
		  int cu = (yuvOut & 0xFF00) >>  8;
		  int cv =  yuvOut &   0xFF;
		  int ey = abs (y - cy);
		  int eu = abs (u - cu);
		  int ev = abs (v - cv);
		  int error = max (eu, ev);
		  if (ey > tLuma  ||  error > tChroma)
		  {
			cout << r << " " << g << " " << b << " getYUV returned unexpected value: " << hex << yuvIn << " " << yuvOut << dec << endl;
			throw "PixelFormat failed";
		  }

		  // get/setGray
		  target.setGray (0, 0, (unsigned char) y);
		  int grayOut = target.getGray (0, 0);
		  if (abs (y - grayOut) > tLuma)
		  {
			cout << r << " " << g << " " << b << " getGray returned unexpected value: " << hex << y << " " << grayOut << dec << endl;
			throw "PixelFormat failed";
		  }

		  // get/setGray(float)
		  target.setGray (0, 0, xyzIn[1]);
		  float grayFloatOut;
		  target.getGray (0, 0, grayFloatOut);
		  float ratio = grayFloatOut / xyzIn[1];
		  if (ratio < 1.0f) ratio = 1.0f / ratio;
		  float difference = fabs (grayFloatOut - xyzIn[1]);
		  if (ratio > tRatio  &&  difference > tDifference[5])
		  {
			cout << r << " " << g << " " << b << " getGray returned unexpected value: ratio=" << ratio << " difference=" << difference << "  " << xyzIn[1] << " -> " << grayFloatOut << endl;
			cout << "tRatio = " << tRatio << "  tDifference = " << tDifference << endl;
			throw "PixelFormat fails";
		  }
		}
	  }
	  cerr << ".";
	}
	cerr << endl;
  }

  // get/setAlpha
  if (targetFormat->hasAlpha)
  {
	for (int a = 0; a < 256; a++)
	{
	  target.setAlpha (0, 0, a);
	  int alphaOut = target.getAlpha (0, 0);
	  if (abs (alphaOut - a) > tAlpha)
	  {
		cout << "Unexpected alpha value: " << hex << a << " " << alphaOut << dec << endl;
		throw "PixelFormat fails";
	  }
	}
  }
}


int
main (int argc, char * argv[])
{
  try
  {
	new ImageFileFormatJPEG;

	// PixelFormat
	{
	  // Create some formats to more fully test RGBABits
	  PixelFormatRGBABits R2G3B2A0 (1, 0x03, 0x1C, 0x60, 0);
	  PixelFormatRGBABits R5G6B5A0 (2, 0xF800, 0x07E0, 0x001F, 0);
	  PixelFormatRGBABits R8G8B8A0 (3, 0xFF0000, 0x00FF00, 0x0000FF, 0);
	  PixelFormatRGBABits R9G9B9A5 (4, 0xFF800000, 0x007FC000, 0x00003FE0, 0x0000001F);

	  vector<fl::PixelFormat *> formats;
	  formats.push_back (&GrayChar);
	  formats.push_back (&GrayShort);
	  formats.push_back (&GrayFloat);
	  formats.push_back (&GrayDouble);
	  formats.push_back (&RGBAChar);
	  formats.push_back (&RGBAShort);
	  formats.push_back (&RGBAFloat);
	  formats.push_back (&RGBChar);
	  formats.push_back (&RGBShort);
	  formats.push_back (&UYV);
	  formats.push_back (&UYVY);
	  formats.push_back (&YUYV);
	  formats.push_back (&UYYVYY);
	  formats.push_back (&UYVYUYVYYYYY);
	  formats.push_back (&YUV420);
	  formats.push_back (&YUV411);
	  formats.push_back (&HLSFloat);
	  formats.push_back (&BGRChar);
	  formats.push_back (&R2G3B2A0);
	  formats.push_back (&R5G6B5A0);
	  formats.push_back (&R8G8B8A0);
	  formats.push_back (&R9G9B9A5);

	  Image test ("test.jpg");

	  Stopwatch timer;
	  for (int i = 0; i < formats.size (); i++)
	  {
		testFormat (test, formats, formats[i]);
	  }
	  timer.stop ();

	  cout << "PixelFormat passes " << timer << endl;
	}

	// AbsoluteValue -- float and double
	{
	  Image image (640, 480, GrayFloat);
	  testAbsoluteValue (image);
	  image.format = &GrayDouble;
	  image.resize (640, 480);
	  testAbsoluteValue (image);
	  cout << "AbsoluteValue passes" << endl;
	}

	// CanvasImage::drawFilledRectangle
	{
	  CanvasImage ci (640, 480);
	  ci.clear ();
	  ci.drawFilledRectangle (Point (-10, -10), Point (10, 10));
	  assert (ci.getGray (10, 10)  &&  ! ci.getGray (11, 11)  &&  ! ci.getGray (10, 11)  &&  ! ci.getGray (11, 10));
	  ci.clear ();
	  ci.drawFilledRectangle (Point (650, 470), Point (630, 490));
	  assert (ci.getGray (630, 470)  &&  ! ci.getGray (629, 469)  &&  ! ci.getGray (629, 470)  &&  ! ci.getGray (630, 469));
	  ci.clear ();
	  ci.drawFilledRectangle (Point (-100, 0), Point (-90, 10));
	  for (int y = 0; y < ci.height; y++)
	  {
		for (int x = 0; x < ci.width; x++)
		{
		  if (ci.getGray (x, y))
		  {
			cout << x << " " << y << " not zero!" << endl;
			throw "CanvasImage::drawFilledRectangle fails";
		  }
		}
	  }
	  cout << "CanvasImage::drawFilledRectangle passes" << endl;
	}

	// ConvolutionDiscrete1D
	{
	  vector<ConvolutionDiscrete1D *> kernels;
	  Gaussian1D oddKernel (1, Crop, GrayDouble);
	  ConvolutionDiscrete1D evenKernel = oddKernel * Transform ((oddKernel.width + 1.0) / oddKernel.width, 1.0);
	  kernels.push_back (&oddKernel);
	  kernels.push_back (&evenKernel);

	  vector<Image *> images;
	  Image test ("test.jpg");
	  //Image test ("mars.jpg");
	  Image onebigger (*test.format);
	  Image same (*test.format);
	  Image onesmaller (*test.format);
	  Image twosmaller (*test.format);
	  Image one (1, 1, *test.format);
	  Image zero (0, 0, *test.format);
	  onebigger. bitblt (test, 0, 0, test.width / 2, test.height / 2, evenKernel.width + 1, evenKernel.width + 1);
	  same.      bitblt (test, 0, 0, test.width / 2, test.height / 2, evenKernel.width,     evenKernel.width    );
	  onesmaller.bitblt (test, 0, 0, test.width / 2, test.height / 2, evenKernel.width - 1, evenKernel.width - 1);
	  twosmaller.bitblt (test, 0, 0, test.width / 2, test.height / 2, evenKernel.width - 2, evenKernel.width - 2);
	  images.push_back (&test);
	  images.push_back (&onebigger);
	  images.push_back (&same);
	  images.push_back (&onesmaller);
	  images.push_back (&twosmaller);
	  images.push_back (&one);
	  images.push_back (&zero);

	  vector<BorderMode> modes;
	  modes.push_back (Crop);
	  modes.push_back (ZeroFill);
	  modes.push_back (Boost);
	  modes.push_back (UseZeros);
	  modes.push_back (Copy);
	  modes.push_back (Undefined);

	  vector<fl::PixelFormat *> formats;
	  formats.push_back (&GrayFloat);
	  formats.push_back (&GrayDouble);

	  for (int f = 0; f < formats.size (); f++)
	  {
		fl::PixelFormat & format = *formats[f];

		for (int i = 0; i < images.size (); i++)
		{
		  Image formattest = *images[i] * format;

		  for (int k = 0; k < kernels.size (); k++)
		  {
			ConvolutionDiscrete1D kernel = *kernels[k] * format;

			for (int m = 0; m < modes.size (); m++)
			{
			  kernel.mode = modes[m];

			  kernel.direction = Vertical;
			  testConvolutionDiscrete1D (formattest, kernel);
			  kernel.direction = Horizontal;
			  testConvolutionDiscrete1D (formattest, kernel);
			}
		  }
		}
	  }

	  cout << "ConvlutionDiscrete1D passes" << endl;
	}


	// ConvolutionDiscrete2D::filter -- {zerofill, usezeros, boost, crop} X {float double}
	// ConvolutionDiscrete2D::response -- {boost  etc} X {float double}


	// ConvolutionDiscrete1D::normalFloats -- float double
	{
	  ImageOf<float> imagef (1, 1, GrayFloat);
	  imagef(0,0) = FLT_MIN / 2.0f;
	  ConvolutionDiscrete1D cf (imagef);
	  cf.normalFloats ();
	  if (imagef(0,0))
	  {
		cout << "pixel is " << imagef(0,0) << endl;
		throw "Convolution1D::normalFloats(float) failed";
	  }

	  imagef(0,0) = FLT_MIN;
	  cf.normalFloats ();
	  if (! imagef(0,0))
	  {
		cout << "pixel should have been nonzero" << endl;
		throw "Convolution1D::normalFloats(float) failed";
	  }

	  ImageOf<double> imaged (1, 1, GrayDouble);
	  imaged(0,0) = DBL_MIN / 2.0;
	  ConvolutionDiscrete1D cd (imaged);
	  cd.normalFloats ();
	  if (imaged(0,0))
	  {
		cout << "pixel is " << imaged(0,0) << endl;
		throw "Convolution1D::normalFloats(double) failed";
	  }

	  cout << "ConvolutionDiscrete1D::normalFloats passes" << endl;
	}

	// ConvolutionDiscrete2D::normalFloats -- float double
	{
	  ImageOf<float> imagef (1, 1, GrayFloat);
	  imagef(0,0) = FLT_MIN / 2.0f;
	  ConvolutionDiscrete2D cf (imagef);
	  cf.normalFloats ();
	  if (imagef(0,0))
	  {
		cout << "pixel is " << imagef(0,0) << endl;
		throw "Convolution2D::normalFloats(float) failed";
	  }

	  imagef(0,0) = FLT_MIN;
	  cf.normalFloats ();
	  if (! imagef(0,0))
	  {
		cout << "pixel should have been nonzero" << endl;
		throw "Convolution2D::normalFloats(float) failed";
	  }

	  ImageOf<double> imaged (1, 1, GrayDouble);
	  imaged(0,0) = DBL_MIN / 2.0;
	  ConvolutionDiscrete2D cd (imaged);
	  cd.normalFloats ();
	  if (imaged(0,0))
	  {
		cout << "pixel is " << imaged(0,0) << endl;
		throw "Convolution2D::normalFloats(double) failed";
	  }

	  cout << "ConvolutionDiscrete2D::normalFloats passes" << endl;
	}

	// DescriptorFilters::prepareFilterMatrix
	// DescriptorFilters::read
	// DescriptorFilters::write
	// Rescale::Rescale
	// Rescale::filter
	// Rotate180
	{
	  DescriptorFilters desc;

	  CanvasImage circle (11, 11, GrayFloat);
	  circle.clear ();
	  circle.drawCircle (Point (5, 5), 5);
	  desc.filters.push_back (circle);

	  CanvasImage square (21, 21, GrayFloat);
	  square.clear ();
	  PointAffine pa;
	  pa.x = 10;
	  pa.y = 10;
	  pa.A(0,0) = 10;
	  pa.A(1,1) = 10;
	  square.drawParallelogram (pa);
	  desc.filters.push_back (square);

	  Point target (320, 240);
	  CanvasImage image (640, 480, GrayFloat);
	  image.drawCircle (target, 5);

	  Vector<float> value = desc.value (image, target);
	  if (! value[0]  ||  value[1])
	  {
		cout << "value = " << value << endl;
		throw "DescriptorFilters fails";
	  }

	  ofstream ofs ("test.filters");
	  desc.write (ofs, false);
	  ofs.close ();

	  ifstream ifs ("test.filters");
	  DescriptorFilters desc2 (ifs);
	  ifs.close ();

	  Vector<float> value2 = desc2.value (image, target);
	  if (value != value2)
	  {
		cout << "values don't match" << endl;
		throw "DescriptorFilters fails";
	  }

	  Image disp = desc2.patch (value2);
	  disp *= Rescale (disp);
	  for (int y = 0; y < circle.height; y++)
	  {
		for (int x = 0; x < circle.width; x++)
		{
		  float a;
		  float c;
		  circle.getGray (x, y, c);
		  disp.getGray (x + 5, y + 5, a);
		  if (fabs (a - c) > 1e-6)
		  {
			cout << "computed patch is wrong " << a - c << endl;
			throw "DescriptorFilters or Rescale or Rotate180 fails";
		  }
		}
	  }

	  cout << "DescriptorFilters, Rescale and Rotate180 pass" << endl;
	}

	// DescriptorLBP
	// DescriptorPatch
	// DescriptorTextonScale
	{
	  CanvasImage image (360, 240, GrayFloat);
	  image.clear ();
	  image.drawFilledRectangle (Point (160, 120), Point (165, 125));

	  // a 10x10 patch at the center of the image
	  PointAffine pa;
	  pa.x = 160;
	  pa.y = 120;
	  pa.A(0,0) = 5;
	  pa.A(1,1) = 5;

	  DescriptorLBP lbp;
	  Vector<float> value = lbp.value (image, pa);

	  DescriptorPatch patch (10, 1);
	  value = patch.value (image, pa);
	  if (value.rows () != 100  ||  value[0] != 0  ||  value[78] < 0.9)
	  {
		cout << "unexpected value: " << value << endl;
		throw "DescriptorPatch fails";
	  } 

	  DescriptorTextonScale ts;
	  value = ts.value (image, pa);

	  cout << "DescriptorPatch passes" << endl;
	  cerr << "more work needed to verify results for DescriptorLBP and DescriptorTextonScale" << endl;
	}

	// IntensityAverage
	// IntensityDeviation
	// IntensityHistogram
	{
	  // Fill an image with a random pattern with known statistics
	  Image image (640, 480, GrayFloat);
	  for (int y = 0; y < image.height; y++)
	  {
		for (int x = 0; x < image.width; x++)
		{
		  float value = randGaussian ();  // avg = 0, std = 1
		  image.setGray (x, y, value);
		}
	  }

	  // Measure statistics and verify
	  IntensityAverage avg;
	  image * avg;
	  IntensityDeviation std (avg.average);
	  image * std;
	  IntensityHistogram hist (avg.minimum, avg.maximum, 20);
	  image * hist;

	  if (fabs (avg.average) > 0.01)
	  {
		cout << "average too far from zero " << avg.average << endl;
		throw "IntensityAverage fails";
	  }
	  if (fabs (std.deviation - 1.0f) > 0.01)
	  {
		cout << "deviation too far from one " << std.deviation << endl;
		throw "IntensityDeviation fails";
	  }
	  if (hist.counts[10] < 50000  ||  hist.counts[0] > 100)
	  {
		cout << "histogram has unexpected distribution:" << endl;
		hist.dump (cout);
		throw "IntensityHistogram fails";
	  }

	  cout << "IntensityAverage, IntensityDeviation and IntensityHistogram pass" << endl;
	}

	// Gaussian1D
	// GaussianDerivative1D
	// GaussianDerivativeSecond1D
	// InterestDOG
	// InterestMSER
	// InterestHarrisLaplacian
	// InterestHessian
	{
	  Image image ("test.jpg");
	  image *= GrayChar;

	  InterestMSER mser;
	  InterestHarrisLaplacian hl;
	  InterestDOG dog;
	  InterestHessian s;

	  InterestPointSet points;
	  mser.run (image, points);
	  hl  .run (image, points);
	  dog .run (image, points);
	  s   .run (image, points);

	  const int expected = 5808;
	  int count = points.size ();
	  if (abs (count - expected) > 50)
	  {
		cout << "unexpected point count " << count << "   rather than " << expected << endl;
		throw "failure in one or more of {InterestDOG, InterestMSER, InterestHarrisLaplacian, InterestHessian} or their dependencies";
	  }

	  cout << "InterestDOG, InterestMSER, InterestHarrisLaplacian and InterestHessian pass" << endl;
	}

	// Transform -- {8dof 6dof} X {float double}
	{
	  Image image (640, 480, GrayFloat);
	  testTransform (image);
	  image.format = &GrayDouble;
	  image.resize (640, 480);
	  testTransform (image);
	  cout << "Transform passes" << endl;
	}

	// VideoFileFormatFFMPEG
	{
	  new VideoFileFormatFFMPEG;

	  {
		VideoOut vout ("test.mpg");
		Image image (320, 240, RGBAChar);
		image.timestamp = NAN;  // force auto-generation of PTS
		for (unsigned int i = 128; i < 255; i++)
		{
		  if (! vout.good ())
		  {
			cout << "vout is bad" << endl;
			throw "VideoFileFormatFFMPEG::write fails";
		  }
		  image.clear ((i << 24) | (i << 16) | (i << 8));
		  vout << image;
		}
	  }

	  VideoIn vin ("test.mpg");
	  int i;
	  for (i = 128; i < 255; i++)
	  {
		Image image;
		vin >> image;
		if (! vin.good ()) break;
		if (image.width != 320  ||  image.height != 240)
		{
		  cerr << "Unexpected image size: " << image.width << " x " << image.height << endl;
		  throw "VideoFileFormatFFMPEG::read fails";
		}
		for (int y = 0; y < image.height; y++)
		{
		  for (int x = 0; x < image.width; x++)
		  {
			int g = image.getGray (x, y);
			if (abs (g - i) > thresholdLumaAccessor)
			{
			  cout << x << " " << y << " expected " << i << " but got " << g << endl;
			  throw "VideoFileFormatFFMPEG::read fails";
			}
		  }
		}
	  }
	  if (i < 250)
	  {
		cout << "didn't read enough frames " << i << endl;
		throw "VideoFileFormatFFMPEG::read fails";
	  }

	  cout << "VideoFileFormatFFMPEG passes" << endl;
	}

  }
  catch (const char * error)
  {
	cout << "Exception: " << error << endl;
	return 1;
  }

  return 0;
}
