#include "fl/descriptor.h"
#include "fl/factory.h"


using namespace std;
using namespace fl;


// class Descriptor -----------------------------------------------------------

Factory<Descriptor>::productMap Factory<Descriptor>::products;

Descriptor::Descriptor ()
{
  monochrome = true;
  dimension = 0;
  supportRadial = 0;
}

Descriptor::~Descriptor ()
{
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
