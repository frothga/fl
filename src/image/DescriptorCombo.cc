/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.7 thru 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/02/25 22:40:25  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.7  2006/02/05 22:25:03  Fred
Break dependency numeric and image libraries:  Created a new class called
Metric that resides in the numeric library and does a job similar to but more
general than Comparison.  Derive Comparison from Metric.  This forces a change
in the semantics of the value() function.  Moved preprocessing flag out of the
function prototype and made it a member of the class.  In the future, it may
prove useful to have preprocessing in Metric itself, in which case relevant
members will move up.

Rearrange construction of ComparisonCombo so it doesn't depend on
DescriptorCombo.

Revision 1.6  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.4  2004/08/30 01:26:06  rothgang
Include timestamp in change detection for cacheing input image.

Revision 1.3  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.2  2004/05/03 18:57:24  rothgang
Add Factory.  Handle more members of Descriptor interface.

Revision 1.1  2004/02/22 00:13:37  rothgang
Add descriptor class that can combine other descriptors into a single feature
vector.
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/serialize.h"


using namespace std;
using namespace fl;


// class DescriptorCombo ------------------------------------------------------

DescriptorCombo::DescriptorCombo ()
{
  lastBuffer = 0;
}

DescriptorCombo::DescriptorCombo (istream & stream)
{
  lastBuffer = 0;

  read (stream);
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
DescriptorCombo::read (istream & stream)
{
  Descriptor::read (stream);

  int count = 0;
  stream.read ((char *) &count, sizeof (count));
  if (! stream.good ())
  {
	throw "Can't finish reading DescriptorCombo: stream bad.";
  }
  for (int i = 0; i < count; i++)
  {
	add (Factory<Descriptor>::read (stream));
  }
}

void
DescriptorCombo::write (ostream & stream) const
{
  Descriptor::write (stream);

  int count = descriptors.size ();
  stream.write ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	Factory<Descriptor>::write (stream, *descriptors[i]);
  }
}


// class ComparisonCombo ------------------------------------------------------

ComparisonCombo::ComparisonCombo ()
{
  totalDimension = 0;
}

ComparisonCombo::ComparisonCombo (istream & stream)
{
  totalDimension = 0;
  read (stream);
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
ComparisonCombo::read (istream & stream)
{
  Comparison::read (stream);

  int count = 0;
  stream.read ((char *) &count, sizeof (count));
  if (! stream.good ())
  {
	throw "Can't finish reading ComparisonCombo: stream bad.";
  }
  for (int i = 0; i < count; i++)
  {
	Comparison * c = dynamic_cast<Comparison *> (Factory<Metric>::read (stream));
	if (! c) throw "Stored object is not a Comparison.";
    comparisons.push_back (c);
	int d;
	stream.read ((char *) &d, sizeof (d));
    dimensions.push_back (d);
    totalDimension += d;
  }
}

void
ComparisonCombo::write (ostream & stream) const
{
  Comparison::write (stream);

  int count = comparisons.size ();
  stream.write ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	comparisons[i]->write (stream);
	int d = dimensions[i];
	stream.write ((char *) &d, sizeof (d));
  }
}
