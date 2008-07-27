/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
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
