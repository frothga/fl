#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class NonMaxSuppress -------------------------------------------------------

NonMaxSuppress::NonMaxSuppress (int half)
{
  this->half = half;
  maximum = 0;
  average = 0;
}

Image
NonMaxSuppress::filter (const Image & image)
{
  maximum = 0;
  average = 0;
  int count = 0;

  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (image.width, image.height, GrayFloat);
	ImageOf<float> that (image);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		int h;
		int hl = max (0, x - half);
		int hh = min (image.width, x + half + 1);
		int v;
		int vl = max (0, y - half);
		int vh = min (image.height, y + half + 1);
		float me = that (x, y);
		for (h = hl; me != 0  &&  h < hh; h++)
		{
		  for (v = vl; v < vh; v++)
		  {
			if (that (h, v) >= me)  // Forces a group of equals to be supressed, but we assume this is a zero-probability case.  Alternative is center-of-gravity test as in GrayChar chase below.
			{
			  if (h != x  ||  v != y)
			  {
				me = 0;
				break;
			  }
			}
		  }
		}
		result (x, y) = me;
		if (me != 0)
		{
		  maximum = max (maximum, me);
		  average += me;
		  count++;
		}
	  }
	}
	average /= count;
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> result (image.width, image.height, GrayDouble);
	ImageOf<double> that (image);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		int h;
		int hl = max (0, x - half);
		int hh = min (image.width, x + half + 1);
		int v;
		int vl = max (0, y - half);
		int vh = min (image.height, y + half + 1);
		double me = that (x, y);
		for (h = hl; me != 0  &&  h < hh; h++)
		{
		  for (v = vl; v < vh; v++)
		  {
			if (that (h, v) >= me)  // Forces a group of equals to be supressed, but we assume this is a zero-probability case.  Alternative is center-of-gravity test as in GrayChar chase below.
			{
			  if (h != x  ||  v != y)
			  {
				me = 0;
				break;
			  }
			}
		  }
		}
		result (x, y) = me;
		if (me != 0)
		{
		  maximum = max (maximum, (float) me);
		  average += me;
		  count++;
		}
	  }
	}
	average /= count;
	return result;
  }
  else if (*image.format == GrayChar)
  {
	ImageOf<unsigned char> result (image.width, image.height, GrayChar);
	ImageOf<unsigned char> that (image);
	for (int x = 0; x < result.width; x++)
	{
	  for (int y = 0; y < result.height; y++)
	  {
		int h;
		int hl = max (0, x - half);
		int hh = min (image.width, x + half + 1);
		int v;
		int vl = max (0, y - half);
		int vh = min (image.height, y + half + 1);
		unsigned char me = that (x, y);
		int c = 0;
		int cx = 0;
		int cy = 0;
		for (h = hl; me  &&  h < hh; h++)
		{
		  for (v = vl; v < vh; v++)
		  {
			if (that (h, v) > me)
			{
			  me = 0;
			  break;
			}
			else if (that (h, v) == me)
			{
			  c++;
			  cx += x - h;
			  cy += y - v;
			}
		  }
		}
		if (c > 1)
		{
		  if (cx / c > 1  ||  cy / c > 1)
		  {
			// We are not the center of our cluster of equal points.
			me = 0;
		  }
		  else
		  {
			// We are the center.  However, it is possible for more than one point to perceive this,
			// so arbitrate further by suppressing self if any point has already been added to output
			// within our neighborhood.

			// Scan everything to the left ...
			for (h = hl; me  &&  h < x; h++)
			{
			  for (v = vl; v < vh; v++)
			  {
				if (result (h, v))
				{
				  me = 0;
				  break;
				}
			  }
			}
			// ... and for extra measure, scan the column above us.
			if (me)
			{
			  for (v = vl; v < y; v++)
			  {
				if (result (x, v))
				{
				  me = 0;
				  break;
				}
			  }
			}
		  }
		}
		result (x, y) = me;
		if (me)
		{
		  maximum = max (maximum, (float) me);
		  average += me;
		  count++;
		}
	  }
	}
	average /= count;
	return result;
  }
  else
  {
	return filter (image * GrayFloat);
  }
}
