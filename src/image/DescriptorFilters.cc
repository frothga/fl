/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
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
DescriptorFilters::value (ImageCache & cache, const PointAffine & point)
{
  Vector<float> result (filters.size ());
  for (int i = 0; i < filters.size (); i++)
  {
	result[i] = filters[i].response (cache.original->image, point);
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
  Vector<float> b = value / value.norm (2);
  Vector<float> x;
  gelss (filterMatrix, x, b, (float *) 0, false, true);
  Image result;
  result.copyFrom ((unsigned char *) x.data, patchWidth, patchHeight, GrayFloat);
  return result;
}

void
DescriptorFilters::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & filters;
}
