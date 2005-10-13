/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
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
  const int bins = counts.size ();
  for (int i = 0; i < bins; i++)
  {
	counts[i] = 0;
  }

  #define addup(size) \
  { \
	size * pixel = (size *) image.buffer; \
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
