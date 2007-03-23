/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision  1.5          Copyright 2005 Sandia Corporation.
Revisions 1.7 thru 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.7  2006/02/17 16:52:55  Fred
Use the new destroy option in gelss() for more efficiency.

Revision 1.6  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/09 04:45:41  Fred
Rename lapack?.h to lapack.h.  Add Sandia copyright notice.

Revision 1.4  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.2  2003/09/07 22:00:10  rothgang
Rename convolution base classes to allow for other methods of computation
besides discrete kernel.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
	stream.read ((char *) ((PixelBufferPacked *) image.buffer)->memory, width * height * GrayFloat.depth);
	filters.push_back (ConvolutionDiscrete2D (image));
  }
}

void
DescriptorFilters::write (ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  int count = filters.size ();
  stream.write ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	int width  = filters[i].width;
	int height = filters[i].height;
	stream.write ((char *) &width,  sizeof (width));
	stream.write ((char *) &height, sizeof (height));
	stream.write ((char *) ((PixelBufferPacked *) filters[i].buffer)->memory, width * height * GrayFloat.depth);
  }
}

