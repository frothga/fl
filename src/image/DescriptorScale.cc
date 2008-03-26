/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

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

Revision 1.2  2004/05/03 19:03:21  rothgang
Add Factory.

Revision 1.1  2003/07/30 14:09:42  rothgang
Added a convenience class for finding characteristic scale.
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"


using namespace fl;
using namespace std;


// class DescriptorScale ------------------------------------------------------

DescriptorScale::DescriptorScale (float firstScale, float lastScale, int interQuanta, float quantum)
{
  initialize (firstScale, lastScale, powf (quantum, 1.0f / interQuanta));
}

DescriptorScale::DescriptorScale (istream & stream)
{
  read (stream);
}

void
DescriptorScale::initialize (float firstScale, float lastScale, float stepSize)
{
  dimension = 1;
  firstScale = max (1.0f, firstScale);
  lastScale = max (firstScale, lastScale);

  int s = 0;
  while (true)
  {
	float scale = firstScale * powf (stepSize, s++);
	if (scale > lastScale)
	{
	  break;
	}
	Laplacian l (scale);
	l *= scale * scale;
	laplacians.push_back (l);
  }
}

Vector<float>
DescriptorScale::value (const Image & image, const PointAffine & point)
{
  Vector<float> result (1);
  result[0] = 1;

  float bestResponse = 0;
  vector<Laplacian>::iterator l;
  for (l = laplacians.begin (); l < laplacians.end (); l++)
  {
	float response = fabsf (l->response (image, point));
	if (response > bestResponse)
	{
	  bestResponse = response;
	  result[0] = l->sigma;
	}
  }

  return result;
}

Image
DescriptorScale::patch (const Vector<float> & value)
{
  float scale = value[0];
  int h = (int) ceilf (scale);
  int width = 2 * h + 1;
  CanvasImage result (width, width, RGBAChar);
  result.clear ();
  result.drawCircle (Point (h, h), scale, 0xFFFFFF);
  return result;
}

void
DescriptorScale::read (std::istream & stream)
{
  Descriptor::read (stream);

  float firstScale;
  float lastScale;
  float stepSize;

  stream.read ((char *) &firstScale, sizeof (firstScale));
  stream.read ((char *) &lastScale,  sizeof (lastScale));
  stream.read ((char *) &stepSize,   sizeof (stepSize));

  initialize (firstScale, lastScale, stepSize);
}

void
DescriptorScale::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  float firstScale = laplacians.front ().sigma;
  float lastScale  = laplacians.back ().sigma;
  float stepSize   = laplacians[1].sigma / laplacians[0].sigma;

  stream.write ((char *) &firstScale, sizeof (firstScale));
  stream.write ((char *) &lastScale,  sizeof (lastScale));
  stream.write ((char *) &stepSize,   sizeof (stepSize));
}
