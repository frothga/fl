#include "fl/slideshow.h"
#include "fl/canvas.h"
#include "fl/color.h"
#include "fl/convolve.h"
#include "fl/video.h"
#include "fl/interest.h"


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


	// Test Video class
	/*
	new ImageFileFormatJPEG;
	new VideoFileFormatFFMPEG;
	VideoIn vin (argv[1]);
	if (*(parmChar (3, "f")) == 's')
	{
	  vin.seekTime (parmFloat (2, 0));
	}
	else
	{
	  vin.seekFrame (parmInt (2, 0));
	}
	Image image;
	while (vin.good ())
	{
	  vin >> image;
	  cerr << image.timestamp << " " << image.width << " " << image.height << endl;

	  if (parmInt (4, 0))
	  {
		image.write ("frame.jpg", "jpeg");
		exit (0);
	  }

	  window.show (image);
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


	// Test Transform
	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	Image image;
	image.read ("test.jpg");
	//image *= GrayDouble;

	Transform rot (parmFloat (1, 0) * PI / 180);
	Transform scale (parmFloat (2, 1), parmFloat (3, 1));
	TransformGauss T = scale * rot;

	//T.setWindow (0, 0, 100, 100);
	T.setPeg (50, 50, 100, 100);
	image *= T;
	window.show (image);
	window.waitForClick ();



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


	// Test assignment of Point from Matrix
	/*
	Matrix<float> M (3, 3);
	M.identity ();
	Point c;
	c = M.column (0);
	cerr << M.column (0) << endl;
	cerr << c.x << " " << c.y << endl;
	c = M.column (1);
	cerr << M.column (1) << endl;
	cerr << c.x << " " << c.y << endl;
	c = M.column (2);
	cerr << M.column (2) << endl;	
	cerr << c.x << " " << c.y << endl;
	cerr << M.column (2)[0] << endl;
	cerr << M.column (2)[1] << endl;
	cerr << M.column (2)[2] << endl;
	*/


	// Test interest operator
	/*
	new ImageFileFormatPGM;
	InterestLaplacian l (5000, 0.02, 2, 0, 3);
	Image i;
	i.read ("test.ppm");
	CanvasImage ci = i;
	i *= GrayFloat;
	multiset<PointInterest> points;
	l.run (i, points);
	multiset<PointInterest>::iterator it;
	for (it = points.begin (); it != points.end (); it++)
	{
	  ci.drawCircle (*it, it->scale, green);
	}
	window.show (ci);
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
	DescriptorSpin spin1 (parmInt (1, 6), parmInt (2, 6), parmFloat (3, 3), parmFloat (4, 2));
	DescriptorSpinExact spin2 (parmInt (1, 6), parmInt (2, 6), parmFloat (3, 3), parmFloat (4, 2));

	Image synth1 (100, 100, Image::GrayFloat);
	synth1.bias = 128;
	synth1.scale = 127;
	for (int x = 0; x < synth1.width; x++)
	{
	  for (int y = 0; y < synth1.height; y++)
	  {
		synth1.pixelGrayFloat (x, y) = randGaussian ();
	  }
	}

	PointInterest p;
	p.x = (synth1.width - 1) / 2.0;
	p.y = (synth1.height - 1) / 2.0;
	p.scale = synth1.width / (2.0 * spin1.supportRadial);
	Vector<float> value1 = spin1.value (synth1, p);
	Image patch1 = spin1.patch (value1);
	float zoomfactor = (float) synth1.height / patch1.height;
	TransformGauss zoom (zoomfactor, zoomfactor);
	patch1 *= zoom;
	Vector<float> value2 = spin2.value (synth1, p);
	Image patch2 = spin2.patch (value2) * zoom;

	Image disp;
	disp.bitblt (synth1);
	disp.bitblt (patch1, synth1.width + 10);
	disp.bitblt (patch2, synth1.width + patch1.width + 20);

	window.show (disp);
	window.waitForClick ();
	*/
  }
  catch (const char * error)
  {
	cerr << "Exception: " << error << endl;
	return 1;
  }

  return 0;
}
