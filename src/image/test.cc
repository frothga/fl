/*
Author: Fred Rothganger
Created 2/24/2006 to perform regression testing on the image library.
*/


#include "fl/image.h"
#include "fl/convolve.h"
#include "fl/canvas.h"
#include "fl/descriptor.h"
#include "fl/random.h"
#include "fl/video.h"
#include "fl/interest.h"

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

void
testFormat (const Image & test, const vector<fl::PixelFormat *> & formats, fl::PixelFormat * targetFormat)
{
  cerr << typeid (*targetFormat).name () << endl;

  const float thresholdRatio         = 1.01f;
  const float thresholdDifference    = 0.01f;
  const int   thresholdLuma          = 1;
  const int   thresholdChroma        = 1;
  const int   thresholdLumaClipped   = 3;
  const int   thresholdChromaClipped = 4;
  const int   thresholdLumaAccessor  = 2;  // higher because errors in YUV<->RGB conversion get magnified by gray conversion

  // Convert from every other format into the target one.
  for (int i = 0; i < formats.size (); i++)
  {
	fl::PixelFormat * fromFormat = formats[i];
	cerr << "  from " << typeid (*fromFormat).name () << endl;

	Image fromImage = test * *fromFormat;
	// Make stride larger than width to verify PixelBufferPacked operation
	// and correct looping in converters.
	fromImage.buffer->resize (fromImage.width + 16, fromImage.height, *fromFormat, true);

	Image toImage = fromImage * *targetFormat;

	if (toImage.width != test.width  ||  toImage.height != test.height)
	{
	  throw "PixelFormat conversion failed to produce same size of image";
	}

	// Verify
	if (fromFormat->monochrome  ||  targetFormat->monochrome)
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
			  cout << x << " " << y << " unexpected change in gray level: " << difference << " = | " << original << " - " << converted << " |" << endl;
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

	  const unsigned int shift = 16 + (int) rint (log (ratioH * ratioV) / log (2));
	  const int bias = 0x808 << (shift - 4);
	  const int maximum = (~(unsigned int) 0) >> (24 - shift);

	  for (int y = 0; y < fromImage.height; y += ratioV)
	  {
		for (int x = 0; x < fromImage.width; x += ratioH)
		{
		  // We need the average RGB value to detect cases where the pixel
		  // value passes outside the RGB volume in YUV space.  In such
		  // cases, the pixel value gets clipped, which can introduce
		  // errors larger than a comfortable threshold.
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
		  int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + bias, 0), maximum) >> shift;
		  int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + bias, 0), maximum) >> shift;
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
			  r = (ty                + 0x166F7 * tv + 0x8000) >> 16;
			  g = (ty -  0x5879 * tu -  0xB6E9 * tv + 0x8000) >> 16;
			  b = (ty + 0x1C560 * tu                + 0x8000) >> 16;
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
		  if (PixelFormatPlanarYCbCrChar * pfycbcr = dynamic_cast<PixelFormatPlanarYCbCrChar *> (targetFormat))
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
			  throw "PixelFormat fails";
			}
		  }
		}
	  }
	}
  }

  // Verify all accessors
  cerr << "  checking accessors";
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
		  float grayExact = rgbaFloatIn[0] * 0.2126 + rgbaFloatIn[1] * 0.7152 + rgbaFloatIn[2] * 0.0722;
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
	for (int r = 0; r < 256; r++)
	{
	  for (int g = 0; g < 256; g++)
	  {
		for (int b = 0; b < 256; b++)
		{
		  // get/setRGBA

		  // get/setRGBA(float)

		  // get/setXYZ

		  // get/setYUV

		  // get/setGray

		  // get/setGray(float)
		}
	  }
	}
  }

  // get/setAlpha
  if (targetFormat->hasAlpha)
  {
	for (int a = 0; a < 256; a++)
	{
	  target.setAlpha (0, 0, a);
	  int alphaOut = target.getAlpha (0, 0);
	  if (abs (alphaOut - a) > thresholdLuma)
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
	  formats.push_back (&UYVYChar);
	  formats.push_back (&YUYVChar);
	  formats.push_back (&UYVChar);
	  formats.push_back (&YUV420);
	  formats.push_back (&YUV411);
	  formats.push_back (&HLSFloat);
	  formats.push_back (&BGRChar);

	  Image test ("test.jpg");

	  for (int i = 0; i < formats.size (); i++)
	  {
		testFormat (test, formats, formats[i]);
	  }

	  cout << "PixelFormat passes" << endl;
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

	// ConvolutionDiscrete1D::filter -- zerofill X {float double}, usezeros X {float double} X {horz vert}, boost X {float double}  X {horz vert}, crop X {float double}
	// ConvolutionDiscrete1D::response -- boost X {float double}, any other X {float double}
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
		assert (image.width == 320  &&  image.height == 240);
		for (int y = 0; y < image.height; y++)
		{
		  for (int x = 0; x < image.width; x++)
		  {
			int g = image.getGray (x, y);
			if (abs (g - i) > 1)
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
