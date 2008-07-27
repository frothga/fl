/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"
#include "fl/lapack.h"


using namespace std;
using namespace fl;


// class DescriptorFilters ----------------------------------------------------

DescriptorFilters::DescriptorFilters ()
{
}

DescriptorFilters::DescriptorFilters (istream & stream)
{
  read (stream);
  prepareFilterMatrix ();
}

DescriptorFilters::~DescriptorFilters ()
{
}

void
DescriptorFilters::prepareFilterMatrix ()
{
  // Determine size of patch
  patchWidth = 0;
  patchHeight = 0;
  vector<ConvolutionDiscrete2D>::iterator i;
  for (i = filters.begin (); i < filters.end (); i++)
  {
	patchWidth = max (patchWidth, i->width);
	patchHeight = max (patchHeight, i->height);
  }
  filterMatrix.resize (filters.size (), patchWidth * patchHeight);

  // Populate matrix
  Rotate180 rotation;
  for (int j = 0; j < filters.size (); j++)
  {
	Image temp (patchWidth, patchHeight, GrayFloat);
	temp.clear ();
	int ox = (patchWidth - filters[j].width) / 2;
	int oy = (patchHeight - filters[j].height) / 2;
	temp.bitblt (filters[j], ox, oy);
	temp *= rotation;
	float * buffer = (float *) ((PixelBufferPacked *) temp.buffer)->memory;
	for (int k = 0; k < patchWidth * patchHeight; k++)
	{
	  filterMatrix (j, k) = buffer[k];
	}
  }
}

Vector<float>
DescriptorFilters::value (const Image & image, const PointAffine & point)
{
  Vector<float> result (filters.size ());
  for (int i = 0; i < filters.size (); i++)
  {
	result[i] = filters[i].response (image, point);
  }
  return result;
}

Image
DescriptorFilters::patch (const Vector<float> & value)
{
  if (filterMatrix.rows () != filters.size ())
  {
	prepareFilterMatrix ();
  }
  Vector<float> b = value / value.frob (2);
  Vector<float> x;
  gelss (filterMatrix, x, b, (float *) 0, false, true);
  Image result;
  result.copyFrom ((unsigned char *) x.data, patchWidth, patchHeight, GrayFloat);
  return result;
}

void
DescriptorFilters::read (istream & stream)
{
  Descriptor::read (stream);

  int count = 0;
  stream.read ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	int width = 0;
	int height = 0;
	stream.read ((char *) &width, sizeof (width));
	stream.read ((char *) &height, sizeof (height));
	Image image (width, height, GrayFloat);
	PixelBufferPacked * pbp = (PixelBufferPacked *) image.buffer;
	stream.read ((char *) pbp->memory, height * pbp->stride);
	filters.push_back (ConvolutionDiscrete2D (image));
  }
}

void
DescriptorFilters::write (ostream & stream) const
{
  Descriptor::write (stream);

  int count = filters.size ();
  stream.write ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	int width  = filters[i].width;
	int height = filters[i].height;
	stream.write ((char *) &width,  sizeof (width));
	stream.write ((char *) &height, sizeof (height));
	PixelBufferPacked * pbp = (PixelBufferPacked *) filters[i].buffer;
	stream.write ((char *) pbp->memory, height * pbp->stride);
  }
}

