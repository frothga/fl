/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

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


// class Normalize ------------------------------------------------------------

Normalize::Normalize (double length)
{
  this->length = length;
}

Image
Normalize::filter (const Image & image)
{
  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (image.width, image.height, GrayFloat);
	result.timestamp = image.timestamp;
	ImageOf<float> that (image);
	float sum = 0;
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		sum += that (x, y) * that (x, y);
	  }
	}
	sum = sqrt (sum);
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		result (x, y) = that (x, y) * length / sum;
	  }
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> result (image.width, image.height, GrayDouble);
	result.timestamp = image.timestamp;
	ImageOf<double> that (image);
	double sum = 0;
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		sum += that (x, y) * that (x, y);
	  }
	}
	sum = sqrt (sum);
	for (int x = 0; x < image.width; x++)
    {
	  for (int y = 0; y < image.height; y++)
	  {
		result (x, y) = that (x, y) * length / sum;
	  }
	}
	return result;
  }
  else
  {
	throw "Normalize::filter: unimplemented format";
  }
}
