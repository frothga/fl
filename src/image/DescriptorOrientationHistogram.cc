/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.8, 1.10 thru 1.12 Copyright 2005 Sandia Corporation.
Revisions 1.14 thru 1.15      Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.15  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.14  2006/02/25 22:40:55  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.13  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.12  2005/10/09 05:07:22  Fred
Remove lapack.h, as it is no longer necessary to obtain matrix inversion
operator.

Revision 1.11  2005/10/09 04:41:15  Fred
Add Sandia distribution terms.

Rename lapack?.h to lapack.h

Revision 1.10  2005/09/10 16:42:29  Fred
Clarify revision history.  Add Sandia copyright notice.  This will need to be
updated with license info before release.

Commit to using new cache mechanism.

Use PointAffine::projection() rather than invertion
PoinAffine::rectification().

Revision 1.9  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.8  2005/01/22 21:09:39  Fred
MSVC compilability fix:  Allocate array for histogram using malloc rather than
using variable sized array extension.

Guarantee that orientations are returned in descending order of strength.

Revision 1.7  2004/08/30 01:26:06  rothgang
Include timestamp in change detection for cacheing input image.

Revision 1.6  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.5  2004/05/03 20:16:15  rothgang
Rearrange parameters for Gaussians so border mode comes before format.

Revision 1.4  2004/05/03 19:00:21  rothgang
Add Factory.

Revision 1.3  2004/02/15 18:39:35  rothgang
Add kernelSize to constructor and rearrange parameters to be more like other
Descriptor constructors.  Improve patch change detection.  Add blur to patch to
adjust scale at which orientation is found.

Revision 1.2  2004/01/14 18:11:16  rothgang
Use FiniteDifferenceX/Y for computing derivative images.

Revision 1.1  2003/12/30 21:06:55  rothgang
Create orientation descriptor.
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"


using namespace fl;
using namespace std;


// class DescriptorOrientationHistogram ---------------------------------------

DescriptorOrientationHistogram::DescriptorOrientationHistogram (float supportRadial, int supportPixel, float kernelSize, int bins)
{
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;
  this->kernelSize    = kernelSize;
  this->bins          = bins;

  cutoff = 0.8f;

  lastBuffer = 0;
}

DescriptorOrientationHistogram::DescriptorOrientationHistogram (istream & stream)
{
  lastBuffer = 0;
  read (stream);
}

void
DescriptorOrientationHistogram::computeGradient (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "DescriptorOrientationHistogram only handles packed buffers for now";

  if (lastBuffer == (void *) imageBuffer->memory  &&  lastTime == image.timestamp)
  {
	return;
  }
  lastBuffer = (void *) imageBuffer->memory;
  lastTime = image.timestamp;

  Image work = image * GrayFloat;
  I_x = work * FiniteDifferenceX ();
  I_y = work * FiniteDifferenceY ();
}

Vector<float>
DescriptorOrientationHistogram::value (const Image & image, const PointAffine & point)
{
  // First, prepeare the derivative images I_x and I_y.
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  Point center;
  float sigma;
  float radius;
  if (point.A(0,0) == 1.0f  &&  point.A(0,1) == 0.0f  &&  point.A(1,0) == 0.0f  &&  point.A(1,1) == 1.0f)  // No shape change, so we can work in context of original image.
  {
	ImageCache::shared.add (image);
	PyramidImage * entry = ImageCache::shared.get (ImageCache::monochrome, point.scale);
	if (! entry) throw "Could not find cached image";
	float octave = (float) image.width / entry->width;
	PointAffine p = point;
	p.x = (p.x + 0.5f) / octave - 0.5f;
	p.y = (p.y + 0.5f) / octave - 0.5f;
	p.scale /= octave;

	computeGradient (*entry);

	radius = p.scale * supportRadial;
	sourceL = (int) floorf (p.x - radius);
	sourceR = (int) ceilf  (p.x + radius);
	sourceT = (int) floorf (p.y - radius);
	sourceB = (int) ceilf  (p.y + radius);

	sourceL = max (sourceL, 0);
	sourceR = min (sourceR, I_x.width - 1);
	sourceT = max (sourceT, 0);
	sourceB = min (sourceB, I_x.height - 1);

	center = p;
	sigma = p.scale;
  }
  else  // Shape change, so we must compute a transformed patch
  {
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (point.projection (), scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = image * t;
	patch *= GrayFloat;

	// Add necessary blur.  Assume blur level in source image is 0.5.
	// The current blur level in the transformed patch depends on
	// the ratio of scale to point.scale.
	double currentBlur = scale * 0.5 / point.scale;
	currentBlur = max (currentBlur, 0.5);
	double targetBlur = supportPixel / kernelSize;
	if (currentBlur < targetBlur)
	{
	  Gaussian1D blur (sqrt (targetBlur * targetBlur - currentBlur * currentBlur), Boost, GrayFloat, Horizontal);
	  patch *= blur;
	  blur.direction = Vertical;
	  patch *= blur;
	}

	lastBuffer = 0;
	computeGradient (patch);

	sourceT = 0;
	sourceL = 0;
	sourceB = patchSize - 1;
	sourceR = sourceB;

	center.x = supportPixel - 0.5f;
	center.y = center.x;
	sigma = supportPixel / supportRadial;
	radius = supportPixel;
  }

  // Second, gather up the gradient histogram.
  float * histogram = new float[bins];
  memset (histogram, 0, bins * sizeof (float));
  float radius2 = radius * radius;
  float sigma2 = 2.0 * sigma * sigma;
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  float cx = x - center.x;
	  float cy = y - center.y;
	  float d2 = cx * cx + cy * cy;
	  if (d2 < radius2)
	  {
		float dx = I_x(x,y);
		float dy = I_y(x,y);
		float angle = atan2 (dy, dx);
		int bin = (int) ((angle + PI) * bins / (2 * PI));
		bin = min (bin, bins - 1);  // Technically, these two lines should not be necessary.  They compensate for numerical jitter.
		bin = max (bin, 0);
		float weight = sqrtf (dx * dx + dy * dy) * expf (- (cx * cx + cy * cy) / sigma2);
		histogram[bin] += weight;
	  }
	}
  }

  // Smooth histogram
  for (int i = 0; i < 6; i++)
  {
	float previous = histogram[bins - 1];
	for (int j = 0; j < bins; j++)
	{
      float current = histogram[j];
      histogram[j] = (previous + current + histogram[(j + 1) % bins]) / 3.0;
      previous = current;
	}
  }

  // Find max value
  float maximum = 0;
  for (int i = 0; i < bins; i++)
  {
	maximum = max (maximum, histogram[i]);
  }
  float threshold = cutoff * maximum;

  // Collect peaks
  map<float, float> angles;
  for (int i = 0; i < bins; i++)
  {
	float h0 = histogram[(i == 0 ? bins : i) - 1];
	float h1 = histogram[i];
	float h2 = histogram[(i + 1) % bins];
	if (h1 > h0  &&  h1 > h2  &&  h1 >= threshold)
	{
	  float peak = 0.5f * (h0 - h2) / (h0 - 2.0f * h1 + h2);
	  angles.insert (make_pair (h1, (i + 0.5f + peak) * 2 * PI / bins - PI));
	}
  }
  delete [] histogram;

  // Store peak angles in result
  Vector<float> result (angles.size ());
  float * r = & result[0];
  map<float, float>::reverse_iterator ri;
  for (ri = angles.rbegin (); ri != angles.rend (); ri++)
  {
	*r++ = ri->second;
  }
  return result;
}

Image
DescriptorOrientationHistogram::patch (const Vector<float> & value)
{
  CanvasImage result;

  // TODO: actually implement this method!

  result.clear ();
  return result;
}

void
DescriptorOrientationHistogram::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
  stream.read ((char *) &kernelSize,    sizeof (kernelSize));
  stream.read ((char *) &bins,          sizeof (bins));
  stream.read ((char *) &cutoff,        sizeof (cutoff));
}

void
DescriptorOrientationHistogram::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &kernelSize,    sizeof (kernelSize));
  stream.write ((char *) &bins,          sizeof (bins));
  stream.write ((char *) &cutoff,        sizeof (cutoff));
}
