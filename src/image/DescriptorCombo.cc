#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorCombo ------------------------------------------------------

DescriptorCombo::DescriptorCombo ()
{
  dimension_ = 0;
  monochrome = true;
  lastImage = 0;
}

DescriptorCombo::~DescriptorCombo ()
{
  for (int i = 0; i < descriptors.size (); i++)
  {
	delete descriptors[i];
  }
}

void
DescriptorCombo::add (Descriptor * descriptor)
{
  descriptors.push_back (descriptor);
  dimension_ += descriptor->dimension ();
  monochrome &= descriptor->isMonochrome ();
}

Vector<float>
DescriptorCombo::value (const Image & image, const PointAffine & point)
{
  if (&image != lastImage  ||  (void *) image.buffer != lastBuffer)
  {
	grayImage = image * GrayFloat;
	lastImage = &image;
	lastBuffer = (void *) image.buffer;
  }

  Vector<float> result (dimension ());
  int r = 0;
  for (int i = 0; i < descriptors.size (); i++)
  {
	Vector<float> value = descriptors[i]->value (descriptors[i]->isMonochrome () ? grayImage : image, point);
	result.region (r) = value;
	r += value.rows ();
  }
  return result;
}

Image
DescriptorCombo::patch (const Vector<float> & value)
{
  Image result;
  return result;
}

Comparison *
DescriptorCombo::comparison ()
{
  return new ComparisonCombo (descriptors);
}

bool
DescriptorCombo::isMonochrome ()
{
  return monochrome;
}

int
DescriptorCombo::dimension ()
{
  return dimension_;
}


// class ComparisonCombo ------------------------------------------------------

ComparisonCombo::ComparisonCombo (vector<Descriptor *> & descriptors)
{
  totalDimension = 0;
  for (int i = 0; i < descriptors.size (); i++)
  {
	comparisons.push_back (descriptors[i]->comparison ());
	dimensions.push_back (descriptors[i]->dimension ());
	totalDimension += dimensions[i];
  }
}

ComparisonCombo::~ComparisonCombo ()
{
  for (int i = 0; i < comparisons.size (); i++)
  {
	delete comparisons[i];
  }
}

Vector<float>
ComparisonCombo::preprocess (const Vector<float> & value) const
{
  Vector<float> result (totalDimension);
  int r = 0;
  for (int i = 0; i < comparisons.size (); i++)
  {
	int l = dimensions[i] - 1;
	result.region (r) = comparisons[i]->preprocess (value.region (r, 0, r+l));
	r += dimensions[i];
  }
  return result;
}

float
ComparisonCombo::value (const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  float result = 1.0f;
  int r = 0;
  for (int i = 0; i < comparisons.size (); i++)
  {
	int l = dimensions[i] - 1;
	result *= comparisons[i]->value (value1.region (r, 0, r+l), value2.region (r, 0, r+l), preprocessed);
	r += dimensions[i];
  }
  return result;
}

float
ComparisonCombo::value (int index, const Vector<float> & value1, const Vector<float> & value2, bool preprocessed) const
{
  int r = 0;
  for (int i = 0; i < index; i++)
  {
	r += dimensions[i];
  }
  int l = dimensions[index] - 1;
  return comparisons[index]->value (value1.region (r, 0, r+l), value2.region (r, 0, r+l), preprocessed);
}

Vector<float>
ComparisonCombo::extract (int index, const Vector<float> & value) const
{
  int r = 0;
  for (int i = 0; i < index; i++)
  {
	r += dimensions[i];
  }
  int l = dimensions[index] - 1;
  return value.region (r, 0, r+l);
}
