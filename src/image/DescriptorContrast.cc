#include "fl/descriptor.h"
#include "fl/lapackd.h"


using namespace fl;
using namespace std;


// class DescriptorContrast ---------------------------------------------------

DescriptorContrast::DescriptorContrast (float supportRadial, int supportPixel)
{
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;
}

Vector<float>
DescriptorContrast::value (const Image & image, const PointAffine & point)
{
  int patchSize = 2 * supportPixel;
  double scale = supportPixel / supportRadial;
  Point center (supportPixel, supportPixel);  // should be supportPixel - 0.5, but will be rounded up anyway, so don't bother.

  Matrix<double> S = ! point.rectification ();
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;

  Image patch;
  if (scale >= point.scale)
  {
	Transform rectify (S, scale);
	rectify.setWindow (0, 0, patchSize, patchSize);
	patch = image * rectify;
  }
  else
  {
	TransformGauss rectify (S, scale);
	rectify.setWindow (0, 0, patchSize, patchSize);
	patch = image * rectify;
  }
  patch *= GrayFloat;
  ImageOf<float> I_x = patch * FiniteDifferenceX ();
  ImageOf<float> I_y = patch * FiniteDifferenceY ();

  float average = 0;
  for (int y = 0; y < patch.height; y++)
  {
	for (int x = 0; x < patch.width; x++)
	{
	  float dx = I_x(x,y);
	  float dy = I_y(x,y);
	  //average += sqrtf (dx * dx + dy * dy);
	  average += dx * dx + dy * dy;
	}
  }
  average /= (patch.width * patch.height);

  Vector<float> result (1);
  result[0] = average;
  return result;
}

/**
   \todo Actually implement this method.
 **/
Image
DescriptorContrast::patch (const Vector<float> & value)
{
  Image result;
  return result;
}

Comparison *
DescriptorContrast::comparison ()
{
  return new MetricEuclidean;
}

int
DescriptorContrast::dimension ()
{
  return 1;
}

void
DescriptorContrast::read (std::istream & stream)
{
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
}

void
DescriptorContrast::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }

  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
}
