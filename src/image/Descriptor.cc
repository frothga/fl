#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class Descriptor -----------------------------------------------------------

Descriptor::~Descriptor ()
{
}

Vector<float>
Descriptor::value (const Image & image, const Point & point)
{
  return value (image, PointAffine (point));
}

Vector<float>
Descriptor::value (const Image & image, const PointInterest & point)
{
  return value (image, PointAffine (point));
}

Vector<float>
Descriptor::value (const Image & image)
{
  throw "alpha selected regions not implemented";
}

Comparison *
Descriptor::comparison ()
{
  return new NormalizedCorrelation;
}

bool
Descriptor::isMonochrome ()
{
  return true;
}

int
Descriptor::dimension ()
{
  return 0;
}

void
Descriptor::read (istream & stream)
{
}

void
Descriptor::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
