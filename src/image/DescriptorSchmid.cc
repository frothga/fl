#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorSchmid -----------------------------------------------------

DescriptorSchmid::DescriptorSchmid (int scaleCount, float scaleStep)
{
  if (scaleStep < 1)
  {
	scaleStep = sqrt (2.0);
  }

  this->scaleStep = scaleStep;

  initialize (scaleCount);
}

DescriptorSchmid::DescriptorSchmid (std::istream & stream)
{
  read (stream);
}

void
DescriptorSchmid::initialize (int scaleCount)
{
  for (int i = 0; i < descriptors.size (); i++)
  {
	delete descriptors[i];
  }
  descriptors.clear ();

  for (int s = 0; s < scaleCount; s++)
  {
    float scale = pow (scaleStep, s);
    DescriptorSchmidScale * d = new DescriptorSchmidScale (scale);
    descriptors.push_back (d);
  }
}

DescriptorSchmid::~DescriptorSchmid ()
{
  for (int i = 0; i < descriptors.size (); i++)
  {
	delete descriptors[i];
  }
}

Vector<float>
DescriptorSchmid::value (const Image & image, const PointAffine & point)
{
  return findScale (point.scale)->value (image, point);
}

Image
DescriptorSchmid::patch (const Vector<float> & value)
{
  // Pick an entry with nice scale and use it to reconstruct the patch
  return findScale (2.0)->patch (value);
}

DescriptorSchmidScale *
DescriptorSchmid::findScale (float sigma)
{
  // Just a linear search.  Could be a binary search, but for a list this small
  // it doesn't make much difference
  DescriptorSchmidScale * result = NULL;
  float smallestDistance = 1e30;
  for (int i = 0; i < descriptors.size (); i++)
  {
	float distance = fabs (descriptors[i]->sigma - sigma);
	if (distance < smallestDistance)
	{
	  result = descriptors[i];
	  smallestDistance = distance;
	}
  }

  return result;
}

void
DescriptorSchmid::read (std::istream & stream)
{
  int scaleCount;
  stream.read ((char *) &scaleCount, sizeof (scaleCount));
  stream.read ((char *) &scaleStep, sizeof (scaleStep));

  initialize (scaleCount);
}

void
DescriptorSchmid::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }

  int scaleCount = descriptors.size ();
  stream.write ((char *) &scaleCount, sizeof (scaleCount));
  stream.write ((char *) &scaleStep, sizeof (scaleStep));
}

