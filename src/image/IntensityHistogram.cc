/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 thru 1.5 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.4  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.3  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.1  2003/12/30 21:10:10  rothgang
Create filter for measuring the spread of intensities in an image.
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class IntensityHistogram ---------------------------------------------------

IntensityHistogram::IntensityHistogram (const vector<float> & ranges)
{
  this->ranges = ranges;

  int bins = ranges.size () - 1;
  counts.resize (bins);
  for (int i = 0; i < bins; i++)
  {
	counts[i] = 0;
  }
}

IntensityHistogram::IntensityHistogram (float minimum, float maximum, int bins)
{
  float step = (maximum - minimum) / bins;

  ranges.resize (bins + 1);
  counts.resize (bins);

  for (int i = 0; i < bins; i++)
  {
	ranges[i] = minimum + i * step;
	counts[i] = 0;
  }
  ranges[bins] = maximum;
}

Image
IntensityHistogram::filter (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "IntensityHistogram can only handle packed buffers for now";
  Pointer imageMemory = imageBuffer->memory;

  const int bins = counts.size ();
  for (int i = 0; i < bins; i++)
  {
	counts[i] = 0;
  }

  #define addup(size) \
  { \
	size * pixel = (size *) imageMemory; \
	size * end   = pixel + image.width * image.height; \
	while (pixel < end) \
	{ \
	  if (*pixel <= ranges.back ()) \
	  { \
		for (int i = bins - 1; i >= 0; i--) \
		{ \
		  if (*pixel >= ranges[i]) \
		  { \
			counts[i]++; \
			break; \
		  } \
		} \
	  } \
	  pixel++; \
	} \
  }

  if (*image.format == GrayFloat)
  {
	addup (float);
  }
  else if (*image.format == GrayDouble)
  {
	addup (double);
  }
  else if (*image.format == GrayChar)
  {
	addup (unsigned char);
  }
  else
  {
	filter (image * GrayFloat);
  }

  return image;
}

int
IntensityHistogram::total () const
{
  int result = 0;
  for (int i = 0; i < counts.size (); i++)
  {
	result += counts[i];
  }
  return result;
}

void
IntensityHistogram::dump (ostream & stream, bool centers, bool percent) const
{
  float total;
  if (percent)
  {
	total = this->total ();
  }

  for (int i = 0; i < counts.size (); i++)
  {
	if (centers)
	{
	  stream << ((ranges[i] + ranges[i+1]) / 2);
	}
	else
	{
	  stream << ranges[i];
	}
	stream << " ";
	if (percent)
	{
	  stream << (counts[i] / total);
	}
	else
	{
	  stream << counts[i];
	}
	stream << endl;
  }
}
