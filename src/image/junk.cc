/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/slideshow.h"
#include "fl/canvas.h"
#include "fl/color.h"
#include "fl/convolve.h"
#include "fl/video.h"
#include "fl/interest.h"
#include "fl/descriptor.h"
#include "fl/lapack.h"
#include "fl/random.h"
#include "fl/track.h"
#include "fl/imagecache.h"
#include "fl/time.h"
#include "fl/endian.h"

#include <math.h>
#include <fstream>

#include <tiff.h>


using namespace std;
using namespace fl;


extern bool useNew;

int
main (int argc, char * argv[])
{
  #define parmChar(n,d) (argc > n ? argv[n] : d)
  #define parmInt(n,d) (argc > n ? atoi (argv[n]) : d)
  #define parmFloat(n,d) (argc > n ? atof (argv[n]) : d)

  try
  {
	SlideShow window;


	// Analyze HLS <-> RGB round trip
	/*
	float maxError = 0;
	double avgError = 0;
	float hls[3];
	for (int r = 0; r < 256; r++)
	{
	  for (int g = 0; g < 256; g++)
	  {
		for (int b = 0; b < 256; b++)
		{
		  unsigned int rgba = (r << 24) | (g << 16) | (b << 8) | 0xFF;
		  HLSFloat.setRGBA (hls, rgba);
		  unsigned int rgbaOut = HLSFloat.getRGBA (hls);

		  int cr =  rgbaOut             >> 24;
		  int cg = (rgbaOut & 0xFF0000) >> 16;
		  int cb = (rgbaOut &   0xFF00) >>  8;
		  int er = abs (r - cr);
		  int eg = abs (g - cg);
		  int eb = abs (b - cb);

		  int error = max (er, max (eg, eb));
		  if (error >= maxError)
		  {
			cerr << r << " " << g << " " << b << " = " << error << " " << er << " " << eg << " " << eb << endl;
			maxError = error;
		  }
		  avgError += error;
		}
	  }
	}
	cerr << "maxError = " << maxError << endl;
	cerr << "avgError = " << avgError / 0x1000000 << endl;
	*/


	// Analyze RGB to gray via YUV
	/*
	float maxError = 0;
	double avgError = 0;
	for (int r = 0; r < 256; r++)
	{
	  for (int g = 0; g < 256; g++)
	  {
		for (int b = 0; b < 256; b++)
		{
		  int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
		  int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
		  int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

		  int cy = y << 16;
		  int cu = u - 128;
		  int cv = v - 128;

		  int rr = min (max (cy                + 0x166F7 * cv + 0x8000, 0), 0xFFFFFF) >> 16;
		  int gg = min (max (cy -  0x5879 * cu -  0xB6E9 * cv + 0x8000, 0), 0xFFFFFF) >> 16;
		  int bb = min (max (cy + 0x1C560 * cu                + 0x8000, 0), 0xFFFFFF) >> 16;

		  float fr = fl::PixelFormat::lutChar2Float[rr];
		  float fg = fl::PixelFormat::lutChar2Float[gg];
		  float fb = fl::PixelFormat::lutChar2Float[bb];
		  float gray = fr * 0.2126 + fg * 0.7152 + fb * 0.0722;
		  int grayChar = fl::PixelFormat::lutFloat2Char[(unsigned short) (65535 * gray)];

		  fr = fl::PixelFormat::lutChar2Float[r];
		  fg = fl::PixelFormat::lutChar2Float[g];
		  fb = fl::PixelFormat::lutChar2Float[b];
		  float grayExact = fr * 0.2126 + fg * 0.7152 + fb * 0.0722;
		  int grayExactChar = fl::PixelFormat::lutFloat2Char[(unsigned short) (65535 * grayExact)];

		  int error = abs (grayChar - grayExactChar);
		  if (error > maxError)
		  {
			cerr << r << " " << g << " " << b << " = " << error << endl;
			maxError = error;
		  }
		  avgError += error;
		}
	  }
	}
	cerr << "maxError = " << maxError << endl;
	cerr << "avgError = " << avgError / 0x1000000 << endl;
	*/


	// Analyze RGB to YCbCr conversions
	/*
	float maxError = 0;
	double avgError = 0;
	for (int r = 0; r < 256; r++)
	{
	  for (int g = 0; g < 256; g++)
	  {
		for (int b = 0; b < 256; b++)
		{
		  int y1 = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
		  int u  = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
		  int v  = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

		  int y2 = (  0x41BD * r + 0x810F * g + 0x1910 * b + 0x100000 + 0x8000) >> 16;
		  int cb = (- 0x25F2 * r - 0x4A7E * g + 0x7070 * b + 0x800000 + 0x8000) >> 16;
		  int cr = (  0x7070 * r - 0x5E28 * g - 0x1248 * b + 0x800000 + 0x8000) >> 16;

		  int yy = fl::PixelFormatPlanarYCbCrChar::lutYout[y2];
		  int uu = fl::PixelFormatPlanarYCbCrChar::lutUVout[cb];
		  int vv = fl::PixelFormatPlanarYCbCrChar::lutUVout[cr];

		  //int yy = fl::PixelFormatPlanarYCbCrChar::lutYout [fl::PixelFormatPlanarYCbCrChar::lutYin [y1]];
		  //int uu = fl::PixelFormatPlanarYCbCrChar::lutUVout[fl::PixelFormatPlanarYCbCrChar::lutUVin[u ]];
		  //int vv = fl::PixelFormatPlanarYCbCrChar::lutUVout[fl::PixelFormatPlanarYCbCrChar::lutUVin[v ]];

		  int ey = yy - y1;
		  int eu = uu - u;
		  int ev = vv - v;

		  //int error = max (abs (ey), max (abs (eu), abs (ev)));
		  int error = max (abs (eu), abs (ev));
		  //int error = abs (ey);
		  //if (error > maxError)
		  if (abs (eu) > 1  ||  abs (ev) > 1)
		  {
			//cerr << r << " " << g << " " << b << " = " << error << endl;
			cerr << y1 << " " << u << " " << v << " = " << ey << " " << eu << " " << ev << endl;
			maxError = error;
		  }
		  avgError += error;
		}
	  }
	}
	cerr << "maxError = " << maxError << endl;
	cerr << "avgError = " << avgError / 0x1000000 << endl;
	*/


	// Analyze YUV to RGB conversions
	/*
	int maxError = 0;
	double avgError = 0;
	int count = 0;
	for (int u = 0; u < 256; u++)
	{
	  for (int v = 0; v < 256; v++)
	  {
		for (int y = 0; y < 256; y++)
		{
		  int cy = y << 16;
		  int cu = u - 128;
		  int cv = v - 128;
		  int r = cy                + 0x166F7 * cv + 0x8000;
		  int g = cy -  0x5879 * cu -  0xB6E9 * cv + 0x8000;
		  int b = cy + 0x1C560 * cu                + 0x8000;
		  //if (r < 0  ||  r > 0xFFFFFF  ||  g < 0  ||  g > 0xFFFFFF  ||  b < 0  ||  b > 0xFFFFFF) continue;
		  r >>= 16;
		  g >>= 16;
		  b >>= 16;
		  count++;

		  int yy = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
		  int uu = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
		  int vv = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
		  cerr << hex << r << " " << g << " " << b << dec << endl;
		  cerr << hex << yy << " " << uu << " " << vv << dec << endl;

		  int ey = yy - y;
		  int eu = uu - u;
		  int ev = vv - v;

		  int error = max (abs (ey), max (abs (eu), abs (ev)));
		  maxError = max (maxError, error);
		  if (ey != 0  ||  eu != 0  ||  ev != 0)
		  {
			cerr << y << " " << u << " " << v << " = " << ey << " " << eu << " " << ev << endl;
		  }
		  avgError += error;
		}
	  }
	}
	cerr << "maxError = " << maxError << endl;
	cerr << "avgError = " << avgError / count << endl;
	*/


	// Analyze RGB to YUV conversions
	/*
	float maxError = 0;
	double avgError = 0;
	for (int r = 0; r < 256; r++)
	{
	  for (int g = 0; g < 256; g++)
	  {
		for (int b = 0; b < 256; b++)
		{
		  int y = min (max (  0x4C84 * r + 0x962B * g + 0x1D4F * b            + 0x8000, 0), 0xFFFFFF) >> 16;
		  int u = min (max (- 0x2B2F * r - 0x54C9 * g + 0x8000 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
		  int v = min (max (  0x8000 * r - 0x6B15 * g - 0x14E3 * b + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

		  int cy = y << 16;
		  int cu = u - 128;
		  int cv = v - 128;

		  int rr = min (max (cy                + 0x166F7 * cv + 0x8000, 0), 0xFFFFFF) >> 16;
		  int gg = min (max (cy -  0x5879 * cu -  0xB6E9 * cv + 0x8000, 0), 0xFFFFFF) >> 16;
		  int bb = min (max (cy + 0x1C560 * cu                + 0x8000, 0), 0xFFFFFF) >> 16;

		  int yy = min (max (  0x4C84 * rr + 0x962B * gg + 0x1D4F * bb            + 0x8000, 0), 0xFFFFFF) >> 16;
		  int uu = min (max (- 0x2B2F * rr - 0x54C9 * gg + 0x8000 * bb + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;
		  int vv = min (max (  0x8000 * rr - 0x6B15 * gg - 0x14E3 * bb + 0x800000 + 0x8000, 0), 0xFFFFFF) >> 16;

		  //float er = abs (yy - y);
		  //float eg = abs (uu - u);
		  //float eb = abs (vv - v);

		  float er = abs (rr - r);
		  float eg = abs (gg - g);
		  float eb = abs (bb - b);

		  //float er = fabs (fl::PixelFormat::lutChar2Float[rr] - fl::PixelFormat::lutChar2Float[r]);
		  //float eg = fabs (fl::PixelFormat::lutChar2Float[gg] - fl::PixelFormat::lutChar2Float[g]);
		  //float eb = fabs (fl::PixelFormat::lutChar2Float[bb] - fl::PixelFormat::lutChar2Float[b]);

		  //float er = fl::PixelFormat::lutChar2Float[rr] / fl::PixelFormat::lutChar2Float[r];
		  //float eg = fl::PixelFormat::lutChar2Float[gg] / fl::PixelFormat::lutChar2Float[g];
		  //float eb = fl::PixelFormat::lutChar2Float[bb] / fl::PixelFormat::lutChar2Float[b];
		  //if (er < 1.0) er = 1.0 / er;
		  //if (eg < 1.0) eg = 1.0 / eg;
		  //if (eb < 1.0) eb = 1.0 / eb;
		  //if (isinf (er)) er = 0;
		  //if (isinf (eg)) eg = 0;
		  //if (isinf (eb)) eb = 0;

		  //float error = max (er, max (eg, eb));
		  float error = er + eg + eb;
		  if (error > maxError)
		  {
			//cerr << r << " " << g << " " << b << " = " << error << endl;
			cerr << y << " " << u << " " << v << " = " << error << endl;
			maxError = error;
		  }
		  avgError += error;
		}
	  }
	}
	cerr << "maxError = " << maxError << endl;
	cerr << "avgError = " << avgError / 0x1000000 << endl;
	*/


	// Analyze color to gray conversions
	/*
	const int redWeight   =  76;
	const int greenWeight = 150;
	const int blueWeight  =  29;
	const int totalWeight = 255;
	const float redToY   = 0.2126;
	const float greenToY = 0.7152;
	const float blueToY  = 0.0722;
	float maxError = 0;
	double avgError = 0;
	for (int r = 0; r < 256; r++)
	{
	  for (int g = 0; g < 256; g++)
	  {
		for (int b = 0; b < 256; b++)
		{
		  int gray1 = ((redWeight * (r << 8) + greenWeight * (g << 8) + blueWeight * (b << 8)) / totalWeight) >> 8;
		  float linear1 = fl::PixelFormat::lutChar2Float[gray1];
		  float lr = fl::PixelFormat::lutChar2Float[r];
		  float lg = fl::PixelFormat::lutChar2Float[g];
		  float lb = fl::PixelFormat::lutChar2Float[b];
		  float linear2 = lr * redToY + lg * greenToY + lb * blueToY;
		  int gray2 = fl::PixelFormat::lutFloat2Char[(unsigned short) (65535 * linear2)];
		  float error = fabs (linear1 - linear2);
		  if (error > maxError)
		  {
			cerr << r << " " << g << " " << b << " = " << gray1 << " " << linear1 << " " << linear2 << " " << " " << gray2 << " " << error << endl;
			maxError = error;
		  }
		  avgError += error;
		}
	  }
	}
	cerr << "maxError = " << maxError << endl;
	cerr << "avgError = " << avgError / 0x1000000 << endl;
	*/


	// Analyze YCbCr to gray conversion
	/*
	float maxError = 0;
	double avgError = 0;
	for (int gray1 = 0; gray1 < 256; gray1++)
	{
	  //float linear1 = (fl::PixelFormat::lutChar2Float[gray1] - fl::PixelFormatPlanarYCbCrChar::floatOffset) / fl::PixelFormatPlanarYCbCrChar::floatRange;
	  float linear1 = (gray1 - 16) / 219.0;
	  if (linear1 <= 0.04045f)
	  {
		linear1 /= 12.92f;
	  }
	  else
	  {
		linear1 = powf ((linear1 + 0.055f) / 1.055f, 2.4f);
	  }

	  int gray2 = fl::PixelFormatPlanarYCbCrChar::lutYout[gray1];
	  float linear2 = fl::PixelFormat::lutChar2Float[gray2];

	  float error = fabs (linear1 - linear2);
	  cerr << gray1 << " " << linear1 << " " << linear2 << " " << " " << gray2 << " " << error << endl;
	  if (error > maxError)
	  {
		maxError = error;
	  }
	  avgError += error;
	}
	cerr << "maxError = " << maxError << endl;
	cerr << "avgError = " << avgError / 0x100 << endl;
	*/


	// Test bitblt
	/*
	new ImageFileFormatJPEG;
	Image test ("test.jpg");
	Image image (200, 200, RGBChar);
	image.clear (WHITE);
	image.bitblt (test, 0, image.height, 0, 0, image.width);
	window.show (image);
	window.waitForClick ();
	*/


	// Test bswap
	/*
	unsigned long long bob[] =
	{
	  0x1122334455667788ll,
	  0x1222334455667788ll,
	  0x1322334455667788ll,
	  0x1422334455667788ll,
	  0x1522334455667788ll,
	  0x1622334455667788ll,
	  0x1722334455667788ll,
	  0x1822334455667788ll,
	  0x1922334455667788ll
	};
	for (int i = 0; i < 9; i++) cerr << hex << bob[i] << endl;
	bswap (bob, 9);
	//bswap ((unsigned int *) bob, 18);
	//bswap ((unsigned short *) bob, 36);
	for (int i = 0; i < 9; i++) cerr << hex << bob[i] << endl;
	//cerr << "direct = " << hex << bswap (0x11223344) << endl;
	*/


	// Test KLT
	/*
	new ImageFileFormatPGM;
	Image image0 ("/home/rothgang/software/klt/img0.pgm");
	Image image1 ("/home/rothgang/software/klt/img1.pgm");
	KLT tracker (3, 27);
	tracker.nextImage (image0);
	tracker.nextImage (image1);
	ifstream ifs ("/home/rothgang/software/klt/points");
	while (ifs.good ())
	{
	  float x0;
	  float y0;
	  float x1;
	  float y1;
	  ifs >> x0 >> y0 >> x1 >> y1;
	  if (! ifs.good ()) break;
	  Point p0 (x0, y0);
	  Point p1 (x1, y1);
	  cerr << p0;
	  tracker.track (p0);
	  cerr  << ": (" << p0 << ") - (" << p1 << ") = " << p0.distance (p1) << endl;
	}
	*/


	// Convert squashed comparison values
	/*
	int m = DescriptorColorHistogram2D (10).dimension;
	double result = acosh (1 / parmFloat (1, 1)) * m / 100;
	cerr << result << endl;
	cerr << 1 / cosh (result * 100 / m) << endl;
	cerr << 1 - result / 2 << endl;
	//cerr << acosh (1 / parmFloat (1, 1)) << endl;
	//cerr << 1 - acosh (1 / parmFloat (1, 1)) / parmFloat (2, 2) << endl;
	//cerr << 1 / cosh (parmFloat (1, 0)) << endl;
	*/


	// Test effect of removing intensity information from image
	/*
	new ImageFileFormatJPEG;
	new ImageFileFormatPGM;
	new ImageFileFormatTIFF;
	Image image (parmChar (1, "test.jpg"));
	image *= YUYVChar;
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		unsigned int yuv = image.getYUV (x, y);
		yuv = (yuv & 0xFFFF) | 0x800000;
		image.setYUV (x, y, yuv);
	  }
	}
	window.show (image);
	window.waitForClick ();
	*/


	// Test rotation in HLS space
	/*
	new ImageFileFormatJPEG;
	new ImageFileFormatPGM;
	new ImageFileFormatTIFF;
	Image image (parmChar (1, "test.jpg"));
	ImageOf<float[3]> image2 = image * HLSFloat;
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		image2(x,y)[0] += parmFloat (2, 0);
	  }
	}
	window.show (image2);
	window.waitForClick ();
	*/


	// Test colorization
	/*
	new ImageFileFormatJPEG;
	new ImageFileFormatPGM;
	new ImageFileFormatTIFF;
	Image image (parmChar (1, "test.jpg"));
	float xyz[3];
	float rgba[4];
	float hls[3];
	hls[0] = 0;  // hue == red
	hls[2] = 1.0f;  // saturation
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		image(x,y).getXYZ (xyz);
		hls[1] = xyz[1];  // lightness
		HLSFloat.getRGBA (hls, rgba);
		image(x,y).setRGBA (rgba);
	  }
	}
	window.show (image);
	window.waitForClick ();
	*/


	// Test white balance on YUV firewire camera images
	/*
	new VideoFileFormatFFMPEG;
	VideoIn vin (parmChar (1, "test.avi"));
	float r = 1;
	float g = 1;
	float b = 1;
	bool first = true;
	while (true)
	{
	  Image image;
	  vin >> image;
	  if (! vin.good ()) break;
	  cerr << image.timestamp << " " << image.width << " " << image.height << endl;
	  image *= RGBAFloat;
	  float rgba[4];
	  if (first)
	  {
		first = false;
		float avgRGBA[4] = {0, 0, 0, 0};
		for (int y = 0; y < 480; y++)
		{
		  for (int x = 500; x < 640; x++)
		  {
			image(x,y).getRGBA (rgba);
			avgRGBA[0] += rgba[0];
			avgRGBA[1] += rgba[1];
			avgRGBA[2] += rgba[2];
		  }
		}
		float count = 240 * 240;
		r = avgRGBA[1] / avgRGBA[0];
		b = avgRGBA[1] / avgRGBA[2];
		cerr << avgRGBA[0] << " " << avgRGBA[1] << " " << avgRGBA[2] << endl;
		cerr << "r=" << r << " b=" << b << endl;
		float avg = (r + b) / 2;
		r /= avg;
		g /= avg;
		b /= avg;
		cerr << "r=" << r << " g=" << g << " b=" << b << endl;
		r = 1.124971875;
		g = 0.5;  //0.84375;
		b = 1;
	  }
	  for (int y = 0; y < image.height; y++)
	  {
		for (int x = 0; x < image.width; x++)
		{
		  image(x,y).getRGBA (rgba);
		  rgba[0] *= r;
		  rgba[1] *= g;
		  rgba[2] *= b;
		  image(x,y).setRGBA (rgba);
		}
	  }
	  window.show (image);
	  window.waitForClick ();
	}
	*/


	// Test angle detector
	/*
	new ImageFileFormatJPEG;
	Image patch (parmChar (1, "test.jpg"));
	DescriptorOrientationHistogram angler (6, 32);
	PointAffine p;
	p.x = patch.width / 2;
	p.y = patch.height / 2;
	p.scale = 2;
	p.A(0,0) = 1.01;
	cerr << "angles = " << angler.value (patch, p) << endl;
	*/


	/*
	new ImageFileFormatJPEG;
	DescriptorTextonScale descriptor;
	Image image (parmChar (1, "test.jpg"));
	cerr << "loaded" << endl;
	image *= RGBAChar;
	cerr << "converted" << endl;
	for (int x = 0; x < image.width; x++)
	{
	  for (int y = 0; y < image.height; y++)
	  {
		image.setAlpha (x, y, 1);
	  }
	}
	cerr << "alpha set" << endl;
	Vector<float> value = descriptor.value (image);
	cerr << "value = " << value << endl;
	Image disp = descriptor.patch (value);
	disp *= Rescale (disp);
	window.show (disp);
	window.waitForClick ();
	*/


	// Demo of derivative of Gaussian convolutions
	/*
	new ImageFileFormatJPEG;
	new ImageFileFormatPGM;
	IntensityAverage ave;
	Image patch;

	GaussianDerivativeSecond Gx2 (0, 0, 80);
	patch = Gx2;
	patch *= ave;
	patch *= Rescale (0.5 / max (ave.maximum, - ave.minimum), 0.5);
	window.show (patch);
	window.waitForClick ();

	GaussianDerivativeFirst Gx1 (0, 80);
	patch = Gx1;
	patch *= ave;
	patch *= Rescale (0.5 / ave.maximum, 0.5);
	window.show (patch);
	window.waitForClick ();

	GaussianDerivativeFirst Gx3 (0, 80, 80, PI * 0.2);
	patch = Gx3;
	patch *= ave;
	patch *= Rescale (0.5 / ave.maximum, 0.5);
	window.show (patch);
	window.waitForClick ();

	GaussianDerivativeFirst Gx4 (0, 80, 80, PI * 0.4);
	patch = Gx4;
	patch *= ave;
	patch *= Rescale (0.5 / ave.maximum, 0.5);
	window.show (patch);
	window.waitForClick ();

	// Now to convolve some images.
	Image image ("test.ppm");
	window.show (image);
	window.waitForClick ();

	for (int i = 0; i < 4; i++)
	{
	  GaussianDerivativeFirst g (0, 5, 5, PI * i / 8);
	  Image disp = image * g;
	  window.show (disp);
	  window.waitForClick ();
	}

	for (int i = 0; i < 4; i++)
	{
	  GaussianDerivativeFirst g (0, 3 * i);
	  Image disp = image * g;
	  window.show (disp);
	  window.waitForClick ();
	}
	*/


	// Test HLS color space
	/*
	float values[4];
	values[0] = parmFloat (1, 0);
	values[1] = parmFloat (2, 0.5);
	values[2] = parmFloat (3, 1);
	cerr << hex << HLSFloat.getRGBA (values) << endl;
	*/


	// Test file format reading
	/*
	new ImageFileFormatMatlab;
	new ImageFileFormatJPEG;
	new ImageFileFormatTIFF;
	new ImageFileFormatPGM;
	new ImageFileFormatNITF;
	ImageFile file (parmChar (1, "test.jpg"));
	Image image;
	file.read (image, parmInt (2, 0), parmInt (3, 0), parmInt (4, 0), parmInt (5, 0));
	string compression;
	file.get ("Compression", compression);
	cerr << "compression = " << compression << endl;
	window.show (image);
	window.waitForClick ();
	*/


	// Test ImageFile metadata reading and writing
	new ImageFileFormatMatlab;
	new ImageFileFormatJPEG;
	new ImageFileFormatTIFF;
	new ImageFileFormatPGM;
	new ImageFileFormatNITF;
	Image test ("test.jpg");
	ImageFile file ("test.tif", "w");
	Matrix<double> xform (4, 4);
	xform.clear ();
	xform(3,3) = 1;
	xform(0,0) = 3;
	xform(1,1) = 3;
	xform(0,3) = 100;
	xform(1,3) = 200;
	file.set ("GeoTransformationMatrix", xform);
	file.set ("PageName", "bob 47");
	file.set ("GTCitation", "my stuff");
	file.set ("GTRasterTypeGeoKey", "RasterPixelIsArea");
	file.write (test);
	file.close ();
	ImageFile file2;
	file2.open ("test.tif", "r");
	string sv;
	Matrix<double> mv;
	string cit;
	string rt;
	file2.get ("PageName", sv);
	file2.get ("GeoTransformationMatrix", mv);
	file2.get ("GTCitation", cit);
	file2.get ("GTRasterTypeGeoKey", rt);
	cerr << "PageName = " << sv << endl;
	cerr << "GeoTransformationMatrix: " << mv.rows () << " " << mv.columns () << endl << mv << endl;
	cerr << "GTCitation = " << cit << endl;
	cerr << "raster type = " << rt << endl;



	// Write a tiled file
	/*
	new ImageFileFormatJPEG;
	new ImageFileFormatTIFF;
	Image test ("test.jpg");
	ImageFile file ("test.tif", "w");
	int blockWidth = test.width; //128;
	int blockHeight = 48; //128;
	file.set ("Compression", "LZW");
	//file.set ("blockWidth", blockWidth);
	//file.set ("blockHeight", blockHeight);
	file.set ("width", test.width);
	file.set ("height", test.height);
	for (int y = 0; y < test.height; y += blockHeight)
	{
	  for (int x = 0; x < test.width; x += blockWidth)
	  {
		Image block (*test.format);
		block.bitblt (test, 0, 0, x, y, blockWidth, blockHeight);
		cerr << "block = " << x << " " << y << " " << block.width << " " << block.height << endl;
		file.write (block, x, y);
	  }
	}
	*/


	// Find average color (for redball demo).  Integrate this with that code.
	/*
	new ImageFileFormatPGM;
	Image color ("/home/rothgang/tmp/color2.ppm");
	Vector<float> sum (4);
	sum.clear ();
	Vector<float> vals (4);
	float count = 0;
	for (int x = 0; x < color.width; x++)
	  {
		for (int y = 0; y < color.height; y++)
		  {
			color(x,y).getRGBA ((float *) vals.data);
			if (vals[0] != 1  ||  vals[1] != 1  ||  vals[2] != 1)
			  {
				count++;
				sum += vals;
			  }
		  }
	  }
	sum /= count;
	cerr << count << " " << sum << endl;
	*/


	// Generate pattern for video camera resolution test
	// The basic idea is to generate a sine wave pattern at two spatial
	// frequencies 1 octave apart.
	/*
	new ImageFileFormatPGM;
	ImageOf<float> pattern (640, 480, GrayFloat);
	pattern.clear (0xFFFFFF);
	float cycles = 40;
	for (int x = 0; x < pattern.width; x++)
	{
	  float value = sin (2 * PI * x * cycles / pattern.width);
	  for (int y = 0; y < pattern.height / 2 - 10; y++)
	  {
		pattern(x,y) = value;
	  }
	}
	cycles = 80;
	for (int x = 0; x < pattern.width; x++)
	{
	  float value = sin (2 * PI * x * cycles / pattern.width);
	  for (int y = pattern.height / 2 + 10; y < pattern.height; y++)
	  {
		pattern(x,y) = value;
	  }
	}
	pattern *= Rescale (0.5, 0.5);
	pattern.write ("pattern.pgm");
	window.show (pattern);
	window.waitForClick ();
	*/


	// Test video reading
	/*
	new VideoFileFormatFFMPEG;
	VideoIn vin (parmChar (1, "test.avi"));
	while (true)
	{
	  Image image;
	  vin >> image;
	  if (! vin.good ()) break;
	  cerr << image.timestamp << " " << image.width << " " << image.height << endl;
	  window.clear ();
	  window.show (image);
	}
	window.waitForClick ();
	*/


	// Test video writing
	/*
	new VideoFileFormatFFMPEG;
	VideoOut vout ("test.mpg");
	//vout.set ("framerate", 5);
	Image image (320, 240, RGBAChar);
	for (unsigned int i = 128; i < 255; i++)
	{
	  if (! vout.good ())
	  {
		cerr << "vout is bad" << endl;
		exit (0);
	  }
	  image.clear ((i << 24) | (i << 16) | (i << 8));
	  window.show (image);
	  vout << image;
	}
	*/


	// Test line segment drawing
	/*
	CanvasImage ci (100, 100, RGBAChar);
	ci.clear ();
	ci.drawSegment (Point (-10, 10), Point (110, 90));
	window.show (ci);
	window.waitForClick ();
	*/


	// Test average and deviation filters
	/*
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	ImageOf<float> image;
	image.read ("test.jpg");
	image *= GrayFloat;

	float average = 0;
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		average += image (x, y);
	  }
	}
	average /= image.width * image.height;
	float variance = 0;
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		float t = image (x, y) - average;
		variance += t * t;
	  }
	}
	variance /= image.width * image.height;
	float deviation = sqrtf (variance);
	cerr << average << endl;
	cerr << deviation << endl;

	IntensityAverage avg;
	image * avg;
	IntensityDeviation std (avg.average);
	image * std;
	cerr << avg.average << endl;
	cerr << std.deviation << endl;
	return 0;
	*/



	// Test Transform
	/*
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	Image image;
	image.read ("test.jpg");
	//image *= GrayFloat;

	Transform rot (parmFloat (1, 0) * PI / 180);
	Transform scale (parmFloat (2, 1), parmFloat (3, 1));
	Matrix3x3<float> A;
	A.identity ();
	A(0,1) = parmFloat (4, 0);
	A(2,0) = parmFloat (5, 0);
	A(2,1) = parmFloat (6, 0);
	Transform sheer (A);
	A.identity ();
	A(0,2) = parmFloat (7, 0);
	A(1,2) = parmFloat (8, 0);
	Transform trans (A);
	Transform T = scale * trans * rot * sheer;

	//T.setWindow (50, 50, 100, 100);
	//T.setPeg (50, 50, 100, 100);
	Stopwatch w;
	image *= T;
	cerr << "time = " << w << endl;
	cerr << typeid (*image.format).name () << endl;
	cerr << "image = " << image.width << " " << image.height << endl;
	//window.show (image);
	//window.waitForClick ();
	*/


	// Test Convolution1D
	/*
	new ImageFileFormatJPEG;
	Image test ("test.jpg");
	test *= GrayFloat;

	Gaussian1D c (1, Crop);
	//c.direction = Vertical;
	useNew = false;
	ImageOf<float> original = test * c;
	useNew = true;
	ImageOf<float> changed = test * c;

	if (original.width != changed.width  ||  original.height != changed.height) throw "different sizes!";
	float maximum = 0;
	for (int y = 0; y < original.height; y++)
	{
	  for (int x = 0; x < original.width; x++)
	  {
		float diff = fabs (original(x,y) - changed(x,y));
		float ratio = original(x,y) / changed(x,y);
		if (ratio < 1) ratio = 1.0f / ratio;
		if (diff > maximum)
		{
		  maximum = diff;
		  cerr << x << " " << y << " " << diff << " " << original(x,y) << " " << changed(x,y) << " " << ratio << endl;
		}
	  }
	}

	window.show (changed);
	window.waitForClick ();
	*/


	// Test convolutions
	/*
	Convolution1D kernel (GrayFloat, Convolution1D::horizontal, ZeroFill);
	kernel.resize (3, 1);
	ImageOf<float> ik (kernel);
	ik(0,0) = 0.5;
	ik(1,0) = 0;
	ik(2,0) = -0.5;

	GaussianDerivative1D der (parmFloat (1, 1));
	Gaussian1D gauss (parmFloat (1, 1));

	gauss *= kernel;
	ImageOf<float> id (der);
	ImageOf<float> ig (gauss);
	for (int i = 0; i < gauss.width; i++)
	{
	  cerr << i << " " << id(i,0) << " - " << ig(i,0) << " = " << id(i,0) - ig(i,0) << endl;
	}
	*/


	// Test filter
	/*
	new ImageFileFormatJPEG;
	Image image (parmChar (1, "test.jpg"));
	FiniteDifferenceX dx;
	window.show (image * dx);
	window.waitForClick ();
	FiniteDifferenceY dy;
	window.show (image * dy);
	window.waitForClick ();
	*/


	// Test Pixel
	/*
	//PixelFormatRGBABits bob (2, 0xF000, 0xF00, 0xF0, 0xF);
	new ImageFileFormatJPEG;
	Image test ("test.jpg");
	//((PixelBufferPacked *) test.buffer)->resize (test.width + 13, test.height, *test.format, true);
	//test *= YUV411;

	Image image (test.width, test.height, YUV422);
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		//float pixel;
		//unsigned char pixel;
		unsigned int pixel;
		pixel = test.getYUV (x, y);
		//test.getGray (x, y, pixel);
		image.setYUV (x, y, pixel);
	  }
	}

	Image image = test * UYYVYY;
	window.show (image); // * Zoom (3, 3));
	window.waitForClick ();
	*/


	// Test matrix nature of Point
	/*
	Point p (3, 2);
	cerr << p[0] << " " << p[1] << endl;
	p[0] += p[1];
	cerr << p[0] << " " << p[1] << endl;
	Matrix<double> A (3, 3);
	A.identity (3);
	p = A * p;
	cerr << p << endl;
	p /= -3;
	cerr << p << endl;
	cerr << A.row (2).dot (Vector<double> (p.homogenous (1))) << endl;
	*/


	// Test image to eps
	/*
	new ImageFileFormatJPEG;
	new ImageFileFormatPGM;
	Image image;
	image.read (parmChar (1, "test.pgm"));
	image *= RGBChar;
	image.write (parmChar (1, "test.jpg"), "jpeg");
	*/


	// Test Point
	/*
	PointAffine p;
	p.x = 100;
	p.y = 100;
	p.scale = 3;
	p.angle = 2;
	p.A.identity ();
	p.A(0,0) = 2;
	cerr << "p = " << p << endl;
	cerr << "p.rectification = " << endl << p.rectification () << endl;
	cerr << "! p.rectification = " << endl << ! p.rectification () << endl;
	cerr << "p.projection = " << endl << p.projection () << endl;
	PointAffine p2 (p.projection ());
	cerr << "p2: " << p2 << " " << p2.angle << " " << p2.scale << endl << p2.A << endl;
	cerr << "p2.rectification = " << endl << p2.rectification () << endl;
	cerr << "p2.projection = " << endl << p2.projection () << endl;
	*/


	// Test interest operator
	/*
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	new ImageFileFormatTIFF;
	Image i;
	i.read (parmChar (1, "test.jpg"));
	Image original = i;
	i *= GrayChar;
	InterestPointSet points;
	//InterestMSER m (parmInt (2, 5), parmFloat (3, 0.9f));
	InterestHarrisLaplacian m;
	//InterestDOG m;
	//InterestHessian m;
	Stopwatch timer;
	m.run (i, points);
	cerr << "run time = " << timer << endl;
	cerr << "total points = " << points.size () << endl;
	InterestPointSet::reverse_iterator it;
	CanvasImage ci;
	ci.copyFrom (original);
	for (it = points.rbegin (); it != points.rend (); it++)
	{
	  if (PointMSER * p = dynamic_cast<PointMSER *> (*it))
	  {
		cerr << "MSER = " << (int) p->threshold << " " << p->sign << " " << p->weight << endl;
		ci.drawMSER (*p, i);
		ci.drawEllipse (*p, p->A * ~p->A, p->scale, GREEN);
	  }
	  else if (PointAffine * p = dynamic_cast<PointAffine *> (*it))
	  {
		ci.drawEllipse (*p, p->A * ~p->A, p->scale, GREEN);
	  }
	  else if (PointInterest * p = dynamic_cast<PointInterest *> (*it))
	  {
		PointAffine pa = *p;
		ci.drawEllipse (pa, pa.A * ~pa.A, pa.scale, GREEN);
	  }
	}
	window.show (ci);
	window.waitForClick ();
	*/


	// Test SIFT
	/*
	ImageOf<float> image (128, 128, GrayFloat);
	for (int x = 0; x < image.width; x++)
	{
	  for (int y = 0; y < image.height; y++)
	  {
		image(x,y) = (float) max (y, 67) / image.width;
		//image(x,y) = (float) x / image.width;
	  }
	}
	window.show (image);
	window.waitForClick ();
	DescriptorSIFT sift;
	PointAffine p;
	p.x = (image.width - 1) / 2.0;
	p.y = (image.height - 1) / 2.0;
	p.scale = image.width / (2 * sift.supportRadial);
	//p.scale = 2;
	p.angle = 0;
	double angle = 0; //PI * 0.13;
	p.A(0,0) = cos (angle);
	p.A(0,1) = -sin (angle);
	p.A(1,0) = -p.A(0,1);
	p.A(1,1) = p.A(0,0);
	Vector<float> value = sift.value (image, p);
	// Covert to integer format used by Lowe's implementation
	//for (int i = 0; i < value.rows (); i++)
	//{
	//  value[i] = min (255, (int) (512 * value[i]));
	//}
	//for (int i = 0; i < 6; i++) cerr << "  " << value.region (i * 20, 0, (i+1) * 20 - 1) << endl;
	//cerr << "  " << value.region (120, 0) << endl;
	CanvasImage ci (image);
	ci.drawParallelogram (p);
	window.show (ci);
	window.waitForClick ();
	window.clear ();
	window.show (sift.patch (value));
	window.waitForClick ();
	*/


	// Test DOG + SIFT
	/*
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	InterestDOG l;
	l.storePyramid = true;
	Image i (parmChar (1, "test.jpg"));
	CanvasImage ci = i;
	i *= GrayFloat;
	multiset<PointInterest> points;
	l.run (i, points);
	cerr << "total points = " << points.size () << endl;
	multimap<float, PointInterest> sorted;
	multiset<PointInterest>::iterator it;
	for (it = points.begin (); it != points.end (); it++)
	{
	  sorted.insert (make_pair (it->scale, *it));
	  //ci.drawPoint (*it, red);
	  //ci.drawCircle (*it, it->scale, white);
	}
	//window.show (ci);
	//window.waitForClick ();
	DescriptorSIFT sift;
	DescriptorOrientationHistogram orient;
	multimap<float, PointInterest>::iterator sit;
	for (sit = sorted.begin (); sit != sorted.end (); sit++)
	{
	  PointAffine p = sit->second;
	  Vector<float> angles = orient.value (i, p);
	  p.angle = angles[0];
	  cout << "p=" << p << " " << p.scale << " " << p.angle << endl;
	  Vector<float> value = sift.value (i, p);
	  // Covert to integer format used by Lowe's implementation
	  for (int i = 0; i < value.rows (); i++)
	  {
		value[i] = min (255, (int) (512 * value[i]));
	  }
	  for (int i = 0; i < 6; i++) cout << "  " << value.region (i * 20, 0, (i+1) * 20 - 1) << endl;
	  cout << "  " << value.region (120, 0) << endl;
	}
	*/


	// Experiment with drawing gradient for SIFT directly from image
	/*
	float supportRadial = 3;
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	Image i (parmChar (1, "test.jpg"));
	CanvasImage ci = i;
	i *= GrayFloat;
	PointAffine p;
	p.x = 400; //i.width / 2;
	p.y = 300; //i.height / 2 - 100;
	p.scale = 10;
	p.angle = parmFloat (5, 0);
	p.A(0,0) = parmFloat (2, 1);  // non-uniform scaling
	p.A(0,1) = parmFloat (3, 0);  // skew
	double s = sqrt (p.A(0,0) * p.A(1,1) - p.A(0,1) * p.A(1,0));
	p.A /= s;
	p.scale *= s;
	double addrot = parmFloat (4, 0);
	Matrix2x2<double> AA;
	AA(0,0) = cos (addrot);
	AA(0,1) = -sin (addrot);
	AA(1,0) = -AA(0,1);
	AA(1,1) = AA(0,0);
	p.A = p.A * AA;
	cerr << "p.A: " << p.A(0,0) * p.A(1,1) - p.A(1,0) * p.A(0,1) << endl;
	cerr << p.A << endl;

	//double currentBlur = 0.5;
	//double targetBlur = pow (2.0, floor (log2 (p.scale)));
	//cerr << "targetBlur = " << targetBlur << " " << p.scale << endl;
	//if (currentBlur < targetBlur)
	//{
	//  Gaussian1D blur (sqrt (targetBlur * targetBlur - currentBlur * currentBlur), Boost, GrayFloat, Horizontal);
	//  i *= blur;
	//  blur.direction = Vertical;
	//  i *= blur;
	//}
	//window.show (i);
	//window.waitForClick ();

	int supportPixel = 32;
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (p.projection (), scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = i * GrayFloat * t;

	// p.A == postrot * scaling * prerot == U * D * VT
	// p.A^-1 w/ scaling uninverted == -prerot * scaling * -postrot
	// accounting for p.angle:  -angle * -prerot * scaling * -postrot
	Matrix<double> U;
	Matrix<double> D;
	Matrix<double> VT;
	gesvd (p.A, U, D, VT);
	if (U(0,0) * U(1,1) < U(1,0) * U(0,1))
	{
	  cerr << "U det " << U(0,0) * U(1,1) - U(0,1) * U(1,0) << endl;
	  cerr << "VT det " << VT(0,0) * VT(1,1) - VT(0,1) * VT(1,0) << endl;
	  //U.column (1) *= -1;
	  //VT.row (1) *= -1;
	  VT = U * VT;
	  U.identity ();
	  cerr << "UDVT:" << endl << U * MatrixDiagonal<double> (D) * VT << endl;
	}
	cerr << "U rot = " << atan2 (U(1,0), U(0,0)) << endl;
	cerr << "VT rot = " << atan2 (VT(1,0), VT(0,0)) << endl;
	Matrix2x2<double> rot;
	rot(0,0) = cos (-p.angle);
	rot(0,1) = -sin (-p.angle);
	rot(1,0) = -rot(0,1);
	rot(1,1) = rot(0,0);
	Matrix2x2<float> Q = rot * ~VT * MatrixDiagonal<double> (D) * ~U;

	//PointAffine pp = p;
	//pp.A = Q;
	//ci.drawParallelogram (pp, supportRadial, green);
	//ci.drawParallelogram (p, supportRadial, red);
	//window.show (ci);
	//window.waitForClick ();
	//return 0;

	Q *= p.scale / scale;

	ImageOf<float> I_x = i * FiniteDifferenceX ();
	ImageOf<float> I_y = i * FiniteDifferenceY ();
	ImageOf<float> p_x = patch * FiniteDifferenceX ();
	ImageOf<float> p_y = patch * FiniteDifferenceY ();
	int sourceT;
	int sourceL;
	int sourceB;
	int sourceR;
	Vector<double> tl (3);
	tl[0] = -supportRadial;
	tl[1] = supportRadial;
	tl[2] = 1;
	Vector<double> tr (3);
	tr[0] = supportRadial;
	tr[1] = supportRadial;
	tr[2] = 1;
	Vector<double> bl (3);
	bl[0] = -supportRadial;
	bl[1] = -supportRadial;
	bl[2] = 1;
	Vector<double> br (3);
	br[0] = supportRadial;
	br[1] = -supportRadial;
	br[2] = 1;
	Matrix<double> S = p.projection ();
	Matrix<double> R = p.rectification ();
	R.region (0, 0, 1, 2) *= scale;
	R(0,2) += supportPixel;
	R(1,2) += supportPixel;
	Point ptl = S * tl;
	Point ptr = S * tr;
	Point pbl = S * bl;
	Point pbr = S * br;
	sourceL = (int) rint (max (min (ptl.x, ptr.x, pbl.x, pbr.x), 0.0f));
	sourceR = (int) rint (min (max (ptl.x, ptr.x, pbl.x, pbr.x), I_x.width - 1.0f));
	sourceT = (int) rint (max (min (ptl.y, ptr.y, pbl.y, pbr.y), 0.0f));
	sourceB = (int) rint (min (max (ptl.y, ptr.y, pbl.y, pbr.y), I_x.height - 1.0f));
	ImageOf<float> q_x (p_x.width, p_x.height, GrayFloat);
	ImageOf<float> q_y (p_x.width, p_x.height, GrayFloat);
	int bins = 36;
	Vector<float> histogram1 (bins);
	histogram1.clear ();
	Vector<float> histogram2 (bins);
	histogram2.clear ();
	ImageOf<float> bob = i;
	for (int y = 0; y < p_x.height; y++)
	{
	  for (int x = 0; x < p_x.width; x++)
	  {
		float dx = p_x(x,y);
		float dy = p_y(x,y);
		float angle = atan2 (dy, dx);
		int bin = (int) ((angle + PI) * bins / (2 * PI));
		bin = min (bin, bins - 1);  // Technically, these two lines should not be necessary.  They compensate for numerical jitter.
		bin = max (bin, 0);
		float weight = sqrtf (dx * dx + dy * dy);
		histogram1[bin] += weight;
	  }
	}
	for (int y = sourceT; y <= sourceB; y++)
	{
	  for (int x = sourceL; x <= sourceR; x++)
	  {
		Point q = R * Point (x, y);
		int qx = (int) rint (q.x);
		int qy = (int) rint (q.y);
		if (qx >= 0  &&  qx < patchSize  &&  qy >= 0  &&  qy < patchSize)
		{
		  Vector<float> v (2);
		  v[0] = I_x(x,y);
		  v[1] = I_y(x,y);

		  // bi-linear method of finding gradient
		  //int fromX = (int) x;
		  //int fromY = (int) y;
		  //float dx = x - fromX;
		  //float dy = y - fromY;
		  //float * p00 = & bob(fromX,fromY);
		  //float * p10 = p00 + 1;
		  //float * p01 = p00 + i.width;
		  //float * p11 = p01 + 1;
		  //v[0] = (1.0 - dy) * (*p10 - *p00) + dy * (*p11 - *p01);
		  //v[1] = (1.0 - dx) * (*p01 - *p00) + dx * (*p11 - *p10);

		  v = Q * v;
		  q_x(qx,qy) = v[0];
		  q_y(qx,qy) = v[1];
		  //cerr << v[0] << " " << p_x(qx,qy) << "     " << v[1] << " " << p_y(qx,qy) << endl;

		  float angle = atan2 (v[1], v[0]);
		  int bin = (int) ((angle + PI) * bins / (2 * PI));
		  bin = min (bin, bins - 1);
		  bin = max (bin, 0);
		  float weight = v.frob (2);
		  histogram2[bin] += weight;
		}
	  }
	}
	histogram1.normalize ();
	histogram2.normalize ();
	ChiSquared c1;
	NormalizedCorrelation c2;
	MetricEuclidean c3;
	HistogramIntersection c4;
	cerr << "chi: " << c1.value (histogram1, histogram2) << endl;
	cerr << "corr: " << c2.value (histogram1, histogram2) << endl;
	cerr << "euc: " << c3.value (histogram1, histogram2) << endl;
	cerr << "int: " << c4.value (histogram1, histogram2) << endl;
	q_x.bitblt (p_x, q_x.width);
	q_y.bitblt (p_y, q_y.width);
	//window.show (q_x);
	//window.waitForClick ();
	//window.show (q_y);
	//window.waitForClick ();
	*/


	// Test DescriptorLBP
	/*
	new ImageFileFormatJPEG;
	new ImageFileFormatPGM;
	new ImageFileFormatTIFF;
	Image image (parmChar (1, "test.ppm"));
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		image.setAlpha (x, y, 0xFF);
	  }
	}
	window.show (image);
	DescriptorLBP lbp (parmInt (2, 8), parmFloat (3, 1));
	PointAffine p;
	p.x = image.width / 2;
	p.y = image.height / 2;
	p.scale = 20;
	Vector<float> value = lbp.value (image);
	cerr << value << endl;
	*/


	// Test ChiSquared comparison
	/*
	new ImageFileFormatPGM;
	Image image (parmChar (1, "test.ppm"));
	DescriptorColorHistogram2D descriptor (parmInt (2, 10), 4);
	Comparison * comparison = descriptor.comparison ();
	PointAffine p;
	p.x = 100;
	p.y = 100;
	p.scale = 5;
	Vector<float> d1 = descriptor.value (image, p);
	p.x = 400;
	Vector<float> d2 = descriptor.value (image, p);
	TransformGauss zoom (10, 10);
	Image disp = descriptor.patch (d1) * zoom;
	disp.bitblt (descriptor.patch (d2) * zoom, disp.width + 10);
	cerr << comparison->value (d1, d2) << endl;
	window.show (disp);
	window.waitForClick ();
	*/


	// Test Convolution2D::response function
	/*
	int supportPixel = 50;
	int patchSize = 2 * supportPixel + 1;
	Image patch (patchSize, patchSize, RGBAChar);
	patch.clear (0xFFFFFF);
	Point middle (supportPixel, supportPixel);
	//GaussianDerivativeFirst Gx (0, supportPixel / Gaussian2D::cutoff, -1, 0, GrayFloat, UseZeros);
	GaussianDerivativeFirst Gx (0, supportPixel / Gaussian2D::cutoff);
	cerr << middle << endl;
	cerr << patch.width << " " << patch.height << endl;
	cerr << Gx.width << " " << Gx.height << endl;
	cerr << Gx.response (patch, middle) << endl;
	*/


	// Test drawText
	/*
	new ImageFileFormatPGM;
	CanvasImage disp (640, 480, RGBAChar);
	disp.setFont (parmChar (1, "courier"), parmFloat (2, 12));
	disp.drawText ("bob", Point (320, 240), WHITE, parmFloat (3, 0));
	disp.write ("test2.pgm", "pgm");
	window.show (disp);
	window.waitForClick ();
	*/


	// Test CanvasPS
	/*
	CanvasPS cps ("bob.ps", 75, 75);
	vector<Point> polygon;
	polygon.push_back (Point (10, 10));
	polygon.push_back (Point (100, 10));
	polygon.push_back (Point (100, 100));
	polygon.push_back (Point (10, 100));
	cps.drawPolygon (polygon, white);
	cps.drawPoint (Point (50, 50), white);
	*/


	// Test scale and translation
	/*
	CanvasImage ci (100, 100, RGBAChar);
	ci.setLineWidth (5);
	vector<Point> polygon;
	polygon.push_back (Point (0, 0));
	polygon.push_back (Point (99, 0));
	polygon.push_back (Point (99, 99));
	polygon.push_back (Point (0, 99));
	ci.drawPolygon (polygon);
	window.show (ci);
	window.waitForClick ();
	*/

	/*
	ci.drawLine (50, 50, -5000);
	ci.setScale (0.5, -0.5);
	ci.setTranslation (0, 99);
	ci.drawLine (50, 50, -5000);
	window.show (ci);
	window.waitForClick ();
	*/


	// Test circle drawing code
	/*
	//ci.setScale (1, -1);
	//ci.setTranslation (0, 99);
	ci.drawCircle (Point (50, 50), 25, white, parmFloat (1, 0), parmFloat (2, 2 * PI));
	//ci.drawCircle (Point (50, 50), 25, white, parmFloat (1, 0) * PI / 180, parmFloat (2, 360) * PI / 180);
	window.show (ci);
	window.waitForClick ();
	*/


	// Test elipse drawing code
	/*
	//CanvasImage ci (100, 100);

	Matrix2x2<float> A;
	A(0,0) = 1.0 / pow (parmFloat (1, 1), 2);
	A(0,1) = 0;
	A(1,0) = 0;
	A(1,1) = 1.0 / pow (parmFloat (2, 1), 2);

	float t = M_PI * 0.1;
	Matrix2x2<float> rot;
	rot(0,0) = cos (t);
	rot(0,1) = - sin (t);
	rot(1,0) = -rot(0,1);
	rot(1,1) = rot(0,0);
	//A = ~rot * A * rot;

	ci.drawEllipse (Point (50, 50), A, 0xFFFFFF, parmFloat (3, 0) * PI / 180, parmFloat (4, 360) * PI / 180);
	ci.drawRay (Point (50, 50), parmFloat (4, 0) * PI / 180, white);
	ci.setScale (1, -1.5);
	//ci.setLineWidth (parmFloat (3, 1));
	ci.setTranslation (0, 99);
	ci.drawEllipse (Point (50, 50), A, 0xFF0000, parmFloat (3, 0) * PI / 180, parmFloat (4, 360) * PI / 180);
	ci.drawRay (Point (50, 50), parmFloat (4, 0) * PI / 180, red);
	window.show (ci);
	window.waitForClick ();

	//cps.drawEllipse (Point (50, 50), A, black, parmFloat (3, 0) * PI / 180, parmFloat (4, 360) * PI / 180);
	*/


	// Test spin image
	/*
	{
	  DescriptorSpin spin (parmInt (1, 6), parmInt (2, 6), parmFloat (3, 3), parmFloat (4, 2));
	  ImageOf<float> synth1 (100, 100, GrayFloat);
	  PointAffine p;
	  p.x = (synth1.width - 1) / 2.0;
	  p.y = (synth1.height - 1) / 2.0;
	  p.scale = synth1.width / (2.0 * spin.supportRadial);

	  // Random image with Gaussian distribution
	  for (int x = 0; x < synth1.width; x++)
	  {
		for (int y = 0; y < synth1.height; y++)
		{
		  synth1(x,y) = randGaussian ();
		}
	  }
	  Vector<float> value = spin.value (synth1, p);
	  Image patch = spin.patch (value);
	  int zoomfactor = (int) ceilf ((float) synth1.height / patch.height);
	  Zoom zoom (zoomfactor, zoomfactor);
	  patch *= zoom;
	  patch.bitblt (synth1, patch.width);
	  window.show (patch);
	  window.waitForClick ();

	  // Random image with uniform distribution
	  for (int x = 0; x < synth1.width; x++)
	  {
		for (int y = 0; y < synth1.height; y++)
		{
		  synth1 (x, y) = randfb ();
		}
	  }
	  value = spin.value (synth1, p);
	  patch = spin.patch (value);
	  patch *= zoom;
	  patch.bitblt (synth1, patch.width);
	  window.show (patch);
	  window.waitForClick ();

	  // Coencentric steps: one intensity level per bin
	  for (int x = 0; x < synth1.width; x++)
	  {
		for (int y = 0; y < synth1.height; y++)
		{
		  float dx = x - p.x;
		  float dy = y - p.y;
		  float radius = sqrt (dx * dx + dy * dy);
		  int r = (int) (radius / (p.x / spin.binsRadial));
		  synth1(x,y) = 1.0 - (r + 0.5) / spin.binsRadial;
		}
	  }
	  value = spin.value (synth1, p);
	  patch = spin.patch (value);
	  patch *= zoom;
	  patch.bitblt (synth1, patch.width);
	  window.show (patch);
	  window.waitForClick ();

	  // Intensity surface which is a hemisphere
	  for (int x = 0; x < synth1.width; x++)
	  {
		for (int y = 0; y < synth1.height; y++)
		{
		  float dx = x - p.x;
		  float dy = y - p.y;
		  float radius = sqrt (dx * dx + dy * dy);
		  float value = sqrt (1.0 - pow (radius / p.x, 2));
		  if (isnan (value))
		  {
			value = 0;
		  }
		  synth1 (x, y) = value;
		}
	  }
	  value = spin.value (synth1, p);
	  patch = spin.patch (value);
	  patch *= zoom;
	  patch.bitblt (synth1, patch.width);
	  window.show (patch);
	  window.waitForClick ();

	  // Intensity surface which is a cone
	  for (int x = 0; x < synth1.width; x++)
	  {
		for (int y = 0; y < synth1.height; y++)
		{
		  float dx = x - p.x;
		  float dy = y - p.y;
		  float radius = sqrt (dx * dx + dy * dy);
		  synth1 (x, y) = 1.0 - radius / p.x;
		}
	  }
	  value = spin.value (synth1, p);
	  patch = spin.patch (value);
	  patch *= zoom;
	  patch.bitblt (synth1, patch.width);
	  window.show (patch);
	  window.waitForClick ();
	}
	*/
  }
  catch (const char * error)
  {
	cerr << "Exception: " << error << endl;
	return 1;
  }

  return 0;
}
