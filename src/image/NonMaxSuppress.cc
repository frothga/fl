/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.8  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.7  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.6  2003/08/06 22:18:44  rothgang
Determine minimum as well as max and average.

Revision 1.5  2003/07/30 16:07:31  rothgang
Finish implementing border modes.  Keep count along with other statistics.

Revision 1.4  2003/07/30 15:34:30  rothgang
Reorder loops for better locality.  Begin adding border modes.

Revision 1.3  2003/07/30 14:50:20  rothgang
Clarify range limiting code.

Revision 1.2  2003/07/17 14:49:22  rothgang
Only update statistic "maximum" when a pixel passes the suppression test.  This
makes it consistent with the method for computing average.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class NonMaxSuppress -------------------------------------------------------

NonMaxSuppress::NonMaxSuppress (int half, BorderMode mode)
{
  this->half = half;

  // Remap border modes to ones that we actually handle.
  switch (mode)
  {
    case Crop:
    case ZeroFill:
	  mode = ZeroFill;
	  break;
    default:
	  mode = UseZeros;
  }
  this->mode = mode;

  maximum = -INFINITY;
  minimum = INFINITY;
  average = 0;
  count   = 0;
}

Image
NonMaxSuppress::filter (const Image & image)
{
  maximum = -INFINITY;
  minimum = INFINITY;
  average = 0;
  count   = 0;

  const int width = 2 * half;

  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (image.width, image.height, GrayFloat);
	ImageOf<float> that (image);
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		int hl = max (0,               x - half);
		int hh = min (image.width - 1, x + half);
		int vl = max (0,                y - half);
		int vh = min (image.height - 1, y + half);
		float me = that (x, y);
		if (mode == ZeroFill  &&  (hh - hl < width  ||  vh - vl < width))
		{
		  me = 0;
		}
		for (int v = vl; me != 0  &&  v <= vh; v++)
		{
		  for (int h = hl; h <= hh; h++)
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
		  minimum = min (minimum, me);
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
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		int hl = max (0,               x - half);
		int hh = min (image.width - 1, x + half);
		int vl = max (0,                y - half);
		int vh = min (image.height - 1, y + half);
		double me = that (x, y);
		if (mode == ZeroFill  &&  (hh - hl < width  ||  vh - vl < width))
		{
		  me = 0;
		}
		for (int v = vl; me != 0  &&  v <= vh; v++)
		{
		  for (int h = hl; h <= hh; h++)
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
		  minimum = min (minimum, (float) me);
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
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		int hl = max (0,               x - half);
		int hh = min (image.width - 1, x + half);
		int vl = max (0,                y - half);
		int vh = min (image.height - 1, y + half);
		unsigned char me = that (x, y);
		if (mode == ZeroFill  &&  (hh - hl < width  ||  vh - vl < width))
		{
		  me = 0;
		}
		int c = 0;
		int cx = 0;
		int cy = 0;
		for (int v = vl; me  &&  v <= vh; v++)
		{
		  for (int h = hl; h <= hh; h++)
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

			// Scan everything above ...
			for (int v = vl; me  &&  v < y; v++)
			{
			  for (int h = hl; h <= hh; h++)
			  {
				if (result (h, v))
				{
				  me = 0;
				  break;
				}
			  }
			}
			// ... and for extra measure, scan the row to the left of us.
			if (me)
			{
			  for (int h = hl; h < x; h++)
			  {
				if (result (h, y))
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
		  minimum = min (minimum, (float) me);
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
