#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class Descriptor -----------------------------------------------------------

Descriptor *
Descriptor::factory (istream & stream)
{
  throw "Descriptor::factory needs to be revised";

  /*
  string name;
  getline (stream, name);

  if (name == typeid (DescriptorFilters).name ())
  {
	return new DescriptorFilters (stream);
  }
  else if (name == typeid (DescriptorPatch).name ())
  {
	return new DescriptorPatch (stream);
  }
  else if (name == typeid (DescriptorSchmidScale).name ())
  {
	return new DescriptorSchmidScale (stream);
  }
  else if (name == typeid (DescriptorSchmid).name ())
  {
	return new DescriptorSchmid (stream);
  }
  else if (name == typeid (DescriptorSpin).name ())
  {
	return new DescriptorSpin (stream);
  }
  else
  {
	cerr << "Unknown Descriptor: " << name << endl;
	return NULL;
  }
  */
}

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
