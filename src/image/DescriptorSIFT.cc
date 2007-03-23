/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.10, 1.12 thru 1.16 Copyright 2005 Sandia Corporation.
Revisions 1.18 thru 1.20       Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.20  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.19  2006/11/12 14:52:14  Fred
Add ability to ignore sign of gradient.

Revision 1.18  2006/02/01 03:33:56  Fred
Fixed parenthesization on formula for angleStep.  Previously, it was adding an
epsilon in the wrong way, allowing the buffer overrun bug to remain.

Revision 1.17  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.16  2005/10/09 05:07:39  Fred
Remove lapack.h, as it is no longer necessary to obtain matrix inversion
operator.

Revision 1.15  2005/10/09 04:41:56  Fred
Add Sandia distribution terms.

Rename lapack?.h to lapack.h

Revision 1.14  2005/09/10 16:56:53  Fred
Add detail to revision history.  Add Sandia copyright notice.  This will need
to be updated with license info before release.

Commit to using new cache mechanism.

Remove "square" case, and optimize remaining code since it doesn't need so many
edge condistion any more.  Store pre-computed set of weights for pixels in
patch.  Rearrange contents of descriptor to match best scan order in patch.

Use PointAffine::projection() rather than inverting
PointAffine::rectification().

Revision 1.13  2005/08/03 03:57:12  Fred
Change color constants to all caps.

Revision 1.12  2005/04/23 20:16:59  Fred
Use experimental ImageCache to select image from cached scale pyramid.  Note
that testing shows this method is both slower and less repeatable than drawing
off a rectified patch every time.

Revision 1.11  2005/04/23 19:36:45  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.10  2005/01/22 21:12:12  Fred
MSVC compilability fix:  Allocate memory for histogram as an fl::Vector rather
than using variable size vector extension.  Replace GNU operator with min() and
max().

Revision 1.9  2004/09/08 17:12:06  rothgang
Use linear mapping for Euclidean distance.

Revision 1.8  2004/08/30 01:26:06  rothgang
Include timestamp in change detection for cacheing input image.

Revision 1.7  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.6  2004/05/03 19:03:09  rothgang
Add Factory.  Add patch representation functions.

Revision 1.5  2004/03/22 20:36:01  rothgang
Remove probability transformation from comparison.

Revision 1.4  2004/02/15 18:36:27  rothgang
Better detection of patch change.  Add dimension().  Adjust parameters in
MetricEuclidean.

Revision 1.3  2004/01/14 18:10:54  rothgang
Restore previous method of computing patch in image.  Add comparison.  Fix
angle calculation durint binning.  Use FiniteDifferenceX/Y for computing
derivative images.

Revision 1.2  2004/01/08 21:27:48  rothgang
Simplify computation of non-deformed region.

Revision 1.1  2003/12/30 21:07:41  rothgang
Create SIFT descriptor.
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/color.h"


using namespace fl;
using namespace std;


// class DescriptorSIFT ------------------------------------------------------

DescriptorSIFT::DescriptorSIFT (int width, int angles)
{
  this->width = width;
  this->angles = angles;
  angleRange = TWOPIf;
  dimension = width * width * angles;

  supportRadial = 3 * width / 2.0f;  // Causes each bin to cover 3 sigmas.
  supportPixel = (int) ceilf (2 * supportRadial);  // Causes drawn off patch to hold 2 pixels per sigma.
  sigmaWeight = width / 2.0;
  maxValue = 0.2f;

  init ();
}

DescriptorSIFT::DescriptorSIFT (istream & stream)
{
  read (stream);
}

DescriptorSIFT::~DescriptorSIFT ()
{
  map<int, ImageOf<float> *>::iterator it;
  for (it = kernels.begin (); it != kernels.end (); it++)
  {
	delete it->second;
  }
}

void
DescriptorSIFT::init ()
{
  dimension = width * width * angles;
  angleStep = (angleRange / angles) + 1e-6;

  map<int, ImageOf<float> *>::iterator it;
  for (it = kernels.begin (); it != kernels.end (); it++)
  {
	delete it->second;
  }
  kernels.clear ();
}

float *
DescriptorSIFT::getKernel (int size)
{
  map<int, ImageOf<float> *>::iterator it = kernels.find (size);
  if (it == kernels.end ())
  {
	const float center = (width - 1) / 2.0f;
	const float sigma2 = 2.0f * sigmaWeight * sigmaWeight;
	const float keyScale = (float) width / size;
	const float keyOffset = 0.5f * keyScale - 0.5f;

	ImageOf<float> * G = new ImageOf<float> (size, size, GrayFloat);
	it = kernels.insert (make_pair (size, G)).first;
	float * g = & (*G)(0,0);

	float qy = keyOffset;
	for (int y = 0; y < size; y++)
	{
	  float qx = keyOffset;
	  float yc = qy - center;
	  for (int x = 0; x < size; x++)
	  {
		float xc = qx - center;
		*g++ = expf (- (xc * xc + yc * yc) / sigma2);
		qx += keyScale;
	  }
	  qy += keyScale;
	}
  }

  return & (* it->second)(0,0);
}

Vector<float>
DescriptorSIFT::value (const Image & image, const PointAffine & point)
{
  // First, grab the patch and prepeare the derivative images I_x and I_y.

  // Find or generate gray image at appropriate blur level
  const float scaleTolerance = pow (2.0f, -1.0f / 6);  // TODO: parameterize "6", should be 2 * octaveSteps
  ImageCache::shared.add (image);
  PyramidImage * entry = ImageCache::shared.get (ImageCache::monochrome, point.scale);
  if (! entry  ||  scaleTolerance > (entry->scale > point.scale ? point.scale / entry->scale : entry->scale / point.scale))
  {
	entry = ImageCache::shared.getLE (ImageCache::monochrome, point.scale);
	if (! entry)  // No smaller image exists, which means base level image (scale == 0.5) does not exist.
	{
	  entry = ImageCache::shared.add (image * GrayFloat, ImageCache::monochrome);
	}
  }
  const float octave = (float) image.width / entry->width;
  PointAffine p = point;
  p.x = (p.x + 0.5f) / octave - 0.5f;
  p.y = (p.y + 0.5f) / octave - 0.5f;
  p.scale /= octave;

  Image patch;
  if (entry->width == entry->height  &&  p.angle == 0  &&  fabs (2.0f * p.scale * supportRadial - entry->width) < 0.5)
  {
	// patch == entire image, so no need to transform
	// Note that the test above should also verify that p is at the center
	// of the image.  However, if the other tests pass, then this is almost
	// certainly the case.
	patch = (*entry);
  }
  else
  {
	int patchSize = 2 * supportPixel;
	const double patchScale = supportPixel / supportRadial;
	Transform t (p.projection (), patchScale);
	t.setWindow (0, 0, patchSize, patchSize);
	patch = (*entry) * t;
  }
  float * g = getKernel (patch.width);

  // ensure standard blur level
  /*
	double currentBlur = scale * 0.5 / point.scale;
	currentBlur = max (currentBlur, 0.5);
	double targetBlur = scale;
	if (currentBlur < targetBlur)
	{
	  Gaussian1D blur (sqrt (targetBlur * targetBlur - currentBlur * currentBlur), Boost, GrayFloat, Horizontal);
	  patch *= blur;
	  blur.direction = Vertical;
	  patch *= blur;
	}
  */

  I_x = patch * fdX;
  I_y = patch * fdY;

  const float keyScale = (float) width / patch.width;
  const float keyOffset = 0.5f * keyScale - 0.5f;

  const int rowStep = width * angles;

  // Second, gather up the gradient histogram that constitutes the SIFT key.
  Vector<float> result (width * width * angles);
  result.clear ();
  float * r = & result[0];
  float * dx = & I_x(0,0);
  float * dy = & I_y(0,0);
  float qy = keyOffset;
  for (int y = 0; y < I_x.height; y++)
  {
	float qx = keyOffset;

	const int yl = (int) floorf (qy);
	const int yh = yl + 1;
	const float yf = qy - yl;
	const float yf1 = 1.0f - yf;

	// Variable naming scheme for pointers: r{row}{column}
	// Below are base pointers for the rows...
	float * const rl = r + yl * rowStep;
	float * const rh = rl + rowStep;

	for (int x = 0; x < I_x.width; x++)
	{
	  float angle = atan2 (*dy, *dx);
	  if (angle < 0.0f) angle += angleRange;
	  angle /= angleStep;
	  const float weight = sqrtf (*dx * *dx + *dy * *dy) * *g++;
	  dx++;
	  dy++;

	  const int xl = (int) floorf (qx);
	  const int xh = xl + 1;
	  const float xf = qx - xl;

	  const int al = (int) floorf (angle);
	  int ah = al + 1;
	  if (ah >= angles)
	  {
		ah = 0;
	  }
	  const float af = angle - al;
	  const float af1 = 1.0f - af;

	  // Use trilinear method to distribute weight to 8 adjacent bins in histogram.
	  int step = xl * angles;
	  if (xl >= 0)
	  {
		const float xweight = (1.0f - xf) * weight;
		if (yl >= 0)
		{
		  float yweight = yf1 * xweight;
		  float * rll = rl + step;
		  rll[al] += af1 * yweight;
		  rll[ah] += af  * yweight;
		}
		if (yh < width)
		{
		  float yweight = yf * xweight;
		  float * rhl = rh + step;
		  rhl[al] += af1 * yweight;
		  rhl[ah] += af  * yweight;
		}
	  }
	  step += angles;
	  if (xh < width)
	  {
		const float xweight = xf * weight;
		if (yl >= 0)
		{
		  float yweight = yf1 * xweight;
		  float * rlh = rl + step;
		  rlh[al] += af1 * yweight;
		  rlh[ah] += af  * yweight;
		}
		if (yh < width)
		{
		  float yweight = yf * xweight;
		  float * rhh = rh + step;
		  rhh[al] += af1 * yweight;
		  rhh[ah] += af  * yweight;
		}
	  }

	  qx += keyScale;
	}
	qy += keyScale;
  }

  result.normalize ();
  bool changed = false;
  for (int i = 0; i < result.rows (); i++)
  {
	if (result[i] > maxValue)
	{
	  result[i] = maxValue;
	  changed = true;
	}
  }
  if (changed)
  {
	result.normalize ();
  }

  return result;
}

inline void
DescriptorSIFT::patch (Canvas * canvas, const Vector<float> & value, int size)
{
  float * length = & value[0];
  Point center;
  Point tip;
  bool nosign = fabs (angleRange - PI) < 1e-6;
  for (int y = 0; y < width; y++)
  {
	center.y = (y + 0.5f) * size;
	for (int x = 0; x < width; x++)
	{
	  center.x = (x + 0.5f) * size;
	  for (int a = 0; a < angles; a++)
	  {
		double angle = a * angleRange / angles;
		double radius = (size / 2.0) * (*length++ / maxValue);
		tip.x = center.x + cos (angle) * radius;
		tip.y = center.y + sin (angle) * radius;
		canvas->drawSegment (center, tip, BLACK);
		if (nosign)
		{
		  tip.x = center.x - cos (angle) * radius;
		  tip.y = center.y - sin (angle) * radius;
		  canvas->drawSegment (center, tip, BLACK);
		}
	  }
	}
  }
}

Image
DescriptorSIFT::patch (const Vector<float> & value)
{
  const int size = 16;  // size of one cell
  CanvasImage result (width * size, width * size, GrayChar);
  result.clear (WHITE);
  patch (&result, value, size);
  return result;
}

void
DescriptorSIFT::patch (const string & fileName, const Vector<float> & value)
{
  const int size = 32;  // size of one cell
  int edge = width * size;
  CanvasPS result (fileName, edge, edge);
  patch (&result, value, size);

  result.setLineWidth (0);  // hairline
  for (int i = 0; i <= width; i++)
  {
	result.drawSegment (Point (i * size, 0), Point (i * size, edge), BLACK);
	result.drawSegment (Point (0, i * size), Point (edge, i * size), BLACK);
  }
}

Comparison *
DescriptorSIFT::comparison ()
{
  return new MetricEuclidean (2.0f);
}

void
DescriptorSIFT::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &width,         sizeof (width));
  stream.read ((char *) &angles,        sizeof (angles));
  stream.read ((char *) &angleRange,    sizeof (angleRange));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
  stream.read ((char *) &sigmaWeight,   sizeof (sigmaWeight));
  stream.read ((char *) &maxValue,      sizeof (maxValue));

  init ();
}

void
DescriptorSIFT::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  stream.write ((char *) &width,         sizeof (width));
  stream.write ((char *) &angles,        sizeof (angles));
  stream.write ((char *) &angleRange,    sizeof (angleRange));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &sigmaWeight,   sizeof (sigmaWeight));
  stream.write ((char *) &maxValue,      sizeof (maxValue));
}
