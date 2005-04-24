/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
*/


#include "fl/slideshow.h"
#include "fl/canvas.h"
#include "fl/color.h"
#include "fl/convolve.h"
#include "fl/video.h"
#include "fl/interest.h"
#include "fl/descriptor.h"
#include "fl/lapackd.h"
#include "fl/random.h"
#include "fl/track.h"

#include <math.h>
#include <fstream>


using namespace std;
using namespace fl;


int
main (int argc, char * argv[])
{
  #define parmChar(n,d) (argc > n ? argv[n] : d)
  #define parmInt(n,d) (argc > n ? atoi (argv[n]) : d)
  #define parmFloat(n,d) (argc > n ? atof (argv[n]) : d)

  try
  {
	SlideShow window;


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
	float total = 0;
	float U = 0;
	float V = 0;
	for (int y = 75; y < image.height - 50; y++)
	{
	  for (int x = 100; x < image.width - 50; x++)
	  {
		unsigned int yuv = image.getYUV (x, y);
		total++;
		U += ((yuv & 0xFF00) >> 8);
		V += (yuv & 0xFF);
		yuv = (yuv & 0xFFFF) | 0x800000;
		image.setYUV (x, y, yuv);
	  }
	}
	U /= total;
	V /= total;
	cerr << "(U,V) = " << U << " " << V << endl;
	//image *= GrayFloat;
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
	Image image (parmChar (1, "test.mat"));
	image *= Rescale (image);
	window.show (image);
	window.waitForClick ();
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
	  image.clear ((i << 16) | (i << 8) | i);
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
	image *= GrayFloat;

	Transform rot (parmFloat (1, 0) * PI / 180);
	Transform scale (parmFloat (2, 1), parmFloat (3, 1));
	Matrix2x2<float> A;
	A.identity ();
	A(0,1) = parmFloat (4, 0);
	Transform sheer (A);
	TransformGauss T = scale * rot * sheer;

	//T.setWindow (0, 0, 100, 100);
	//T.setPeg (50, 50, 100, 100);
	image *= T;
	cerr << typeid (*image.format).name () << endl;
	window.show (image);
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
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	new ImageFileFormatTIFF;
	Image image;
	image.read (parmChar (1, "test.tif"));
	image *= ClearAlpha (0x808080);
	image.write ("/home/rothgang/public_html/test.jpg", "jpg");
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
	InterestHarrisLaplacian l;
	Image i;
	i.read (parmChar (1, "test.jpg"));
	CanvasImage ci = i;
	i *= GrayFloat;
	multiset<PointInterest> points;
	l.run (i, points);
	cerr << "total points = " << points.size () << endl;
	multiset<PointInterest>::iterator it;
	for (it = points.begin (); it != points.end (); it++)
	{
	  ci.drawCircle (*it, it->scale, green);
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
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	InterestDOG l;
	l.storePyramid = true;
	//Image i (parmChar (1, "test.jpg"));
	ImageOf<float> i (1280, 960, GrayFloat);
	for (int x = 0; x < i.width; x++)
	{
	  for (int y = 0; y < i.height; y++)
	  {
		i(x,y) = (float) x / i.width;
	  }
	}
	CanvasImage ci = i;
	i *= GrayFloat;
	multiset<PointInterest> points;
	l.run (i, points);
	PointInterest tp;
	tp.x = 167.52;
	tp.y = 470.56;
	tp.scale = 66.68;
	//tp.x = i.width / 2;
	//tp.y = i.height / 2;
	//tp.scale = 5;
	points.clear ();
	points.insert (tp);
	cerr << "total points = " << points.size () << endl;
	multimap<float, PointInterest> sorted;
	multiset<PointInterest>::iterator it;
	for (it = points.begin (); it != points.end (); it++)
	{
	  sorted.insert (make_pair (it->scale, *it));
	}
	DescriptorSIFT sift;
	DescriptorOrientationHistogram orient;
	multimap<float, PointInterest>::iterator sit;
	for (sit = sorted.begin (); sit != sorted.end (); sit++)
	{
	  PointAffine p = sit->second;
	  // find closest pyramid entry
	  float closestRatio = INFINITY;
	  int closestIndex = 0;
	  for (int j = 0; j < l.scales.size (); j++)
	  {
		float ratio = fabsf (1.0f - l.scales[j] / p.scale);
		if (ratio < closestRatio)
		{
		  closestRatio = ratio;
		  closestIndex = j;
		}
	  }
	  cerr << "closestRatio = " << closestRatio << endl;
	  float octave = (float) l.pyramid[0].width / l.pyramid[closestIndex].width;
	  p.x = (p.x + 0.5f) / octave - 0.5f;
	  p.y = (p.y + 0.5f) / octave - 0.5f;
	  p.scale /= octave;
	  Vector<float> angles = orient.value (l.pyramid[closestIndex], p);
	  cerr << sit->second << " " << sit->second.scale << " : " << angles[0] << endl;
	  //p.angle = angles[0];
	  p.angle = -2.892;
	  //p.angle = PI * 0.13;
	  cerr << "p=" << p << " " << p.scale << " " << p.angle << endl;
	  cerr << "pyramid image =" << l.pyramid[closestIndex].width << " " << l.pyramid[closestIndex].height << endl;
	  Vector<float> value = sift.value (l.pyramid[closestIndex], p);
	  // Covert to integer format used by Lowe's implementation
	  for (int i = 0; i < value.rows (); i++)
	  {
		value[i] = min (255, (int) (512 * value[i]));
	  }
	  for (int i = 0; i < 6; i++) cerr << "  " << value.region (i * 20, 0, (i+1) * 20 - 1) << endl;
	  cerr << "  " << value.region (120, 0) << endl;
	}



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
