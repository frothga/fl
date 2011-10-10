/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorCombo ------------------------------------------------------

DescriptorCombo::DescriptorCombo ()
{
  lastBuffer = 0;
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
  dimension += descriptor->dimension;
  monochrome &= descriptor->monochrome;
  supportRadial = max (supportRadial, descriptor->supportRadial);
}

Vector<float>
DescriptorCombo::value (const Image & image, const PointAffine & point)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "DescriptorCombo only handles packed buffers for now";

  if ((void *) imageBuffer->memory != lastBuffer  ||  image.timestamp != lastTime)
  {
	grayImage = image * GrayFloat;
	lastBuffer = (void *) imageBuffer->memory;
	lastTime = image.timestamp;
  }

  Vector<float> result (dimension);
  int r = 0;
  for (int i = 0; i < descriptors.size (); i++)
  {
	Vector<float> value = descriptors[i]->value (descriptors[i]->monochrome ? grayImage : image, point);
	result.region (r) = value;
	r += value.rows ();
  }
  return result;
}

Vector<float>
DescriptorCombo::value (const Image & image)
{
  Vector<float> result (dimension);
  int r = 0;
  for (int i = 0; i < descriptors.size (); i++)
  {
	Vector<float> value = descriptors[i]->value (image);
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

Image
DescriptorCombo::patch (int index, const Vector<float> & value)
{
  int r2 = 0;
  int r1;
  for (int i = 0; i <= index; i++)
  {
	r1 = r2;
	r2 += descriptors[i]->dimension;
  }
  r2 -= 1;
  return descriptors[index]->patch (value.region (r1, 0, r2));
}

Comparison *
DescriptorCombo::comparison ()
{
  ComparisonCombo * result = new ComparisonCombo;
  for (int i = 0; i < descriptors.size (); i++)
  {
	result->add (descriptors[i]->comparison (), descriptors[i]->dimension);
  }
  return result;
}

void
DescriptorCombo::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & descriptors;
}


// class ComparisonCombo ------------------------------------------------------

ComparisonCombo::ComparisonCombo ()
{
  totalDimension = 0;
}

ComparisonCombo::~ComparisonCombo ()
{
  clear ();
}

void
ComparisonCombo::clear ()
{
  for (int i = 0; i < comparisons.size (); i++)
  {
	delete comparisons[i];
  }
  comparisons.clear ();
  dimensions.clear ();
  totalDimension = 0;
}

void
ComparisonCombo::add (Comparison * comparison, int dimension)
{
  comparisons.push_back (comparison);
  dimensions.push_back (dimension);
  totalDimension += dimension;
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

/**
   The idea here is to treat each of the subvalues as a probability (by
   subtracting them from 1) and then find their product.  Finally, convert
   this back to a distance by subtracting from 1 again.
 **/
float
ComparisonCombo::value (const Vector<float> & value1, const Vector<float> & value2) const
{
  float result = 1.0f;
  int r = 0;
  for (int i = 0; i < comparisons.size (); i++)
  {
	Comparison * c = comparisons[i];
	c->needPreprocess = needPreprocess;
	int l = dimensions[i] - 1;
	result *= 1.0f - c->value (value1.region (r, 0, r+l), value2.region (r, 0, r+l));
	r += dimensions[i];
  }
  return 1.0f - result;
}

float
ComparisonCombo::value (int index, const Vector<float> & value1, const Vector<float> & value2) const
{
  int r = 0;
  for (int i = 0; i < index; i++)
  {
	r += dimensions[i];
  }
  int l = dimensions[index] - 1;

  Comparison * c = comparisons[index];
  c->needPreprocess = needPreprocess;
  return c->value (value1.region (r, 0, r+l), value2.region (r, 0, r+l));
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

void
ComparisonCombo::serialize (Archive & archive, uint32_t version)
{
  archive & *((Comparison *) this);
  archive & comparisons;
  archive & dimensions;

  if (archive.in)
  {
	for (int i = 0; i < dimensions.size (); i++) totalDimension += dimensions[i];
  }
}
