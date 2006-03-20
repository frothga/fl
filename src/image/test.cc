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


int
main (int argc, char * argv[])
{
  try
  {
	new ImageFileFormatJPEG;


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
		int f = (int) rint (i * 219.0 / 255.0) + 16;
		for (int y = 0; y < image.height; y++)
		{
		  for (int x = 0; x < image.width; x++)
		  {
			int g = image.getGray (x, y);
			if (abs (g - f) > 1)
			{
			  cout << x << " " << y << " expected " << f << " but got " << g << endl;
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
