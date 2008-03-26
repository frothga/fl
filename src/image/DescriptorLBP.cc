/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.7, 1.9 thru 1.11 Copyright 2005 Sandia Corporation.
Revisions 1.13 thru 1.14     Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.14  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.13  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.12  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.11  2005/10/09 05:06:54  Fred
Remove lapack.h, as it is no longer necessary to obtain matrix inversion
operator.

Revision 1.10  2005/10/09 04:45:52  Fred
Rename lapack?.h to lapack.h.  Add Sandia copyright notice.

Revision 1.9  2005/09/10 16:39:52  Fred
Clarify revision history.

Revision 1.8  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.7  2005/01/22 21:07:49  Fred
MSVC compilability fix:  Replace GNU operator with min() and max().

Revision 1.6  2004/08/30 01:26:06  rothgang
Include timestamp in change detection for cacheing input image.

Revision 1.5  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.4  2004/05/03 18:59:47  rothgang
More discriminating image change detector.  Cache LBP category for all pixels
in image.  Makes histogramming much faster if there are overlapping calls.

Revision 1.3  2004/04/18 04:37:41  rothgang
Add constructor from stream and fix bug in allocating feature vector.

Revision 1.2  2004/03/22 20:32:28  rothgang
Create preprocessing step and stage more constants rather than continually
computing them.

Revision 1.1  2004/02/22 00:12:50  rothgang
Add Local Binary Pattern descriptor.
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"

// for debugging
#include "fl/time.h"


using namespace fl;
using namespace std;


// class DescriptorLBP --------------------------------------------------------

DescriptorLBP::DescriptorLBP (int P, float R, float supportRadial, int supportPixel)
{
  this->P             = P;
  this->R             = R;
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;

  initialize ();
}

DescriptorLBP::DescriptorLBP (istream & stream)
{
  read (stream);
}

void
DescriptorLBP::initialize ()
{
  dimension = P + 2;
  lastBuffer = 0;

  interpolates.resize (P);
  for (int i = 0; i < P; i++)
  {
	Interpolate & t = interpolates[i];
	float angle = i * 2 * PI / P;
	float xf = R * cosf (angle);
	float yf = R * sinf (angle);
	t.xl = (int) floorf (xf);
	t.yl = (int) floorf (yf);
	xf -= t.xl;
	yf -= t.yl;
	if ((xf < 0.01f  ||  xf > 0.99f)  &&  (yf < 0.01f  ||  yf > 0.99f))
	{
	  t.exact = true;
	  if (xf > 0.5f) t.xl++;
	  if (yf > 0.5f) t.yl++;
	}
	else
	{
	  t.exact = false;
	  t.xh = t.xl + 1;
	  t.yh = t.yl + 1;
	  float xf1 = 1.0f - xf;
	  float yf1 = 1.0f - yf;
	  t.wll = xf1 * yf1;
	  t.wlh = xf1 * yf;
	  t.whl = xf  * yf1;
	  t.whh = xf  * yf;
	}
	//cerr << i << " " << t.exact << " " << t.xl << " " << t.yl << endl;
  }
}

inline void
DescriptorLBP::preprocess (const Image & image)
{
  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "DescriptorLBP only handles packed buffers for now";

  if (lastBuffer == (void *) imageBuffer->memory  &&  lastTime == image.timestamp)
  {
	return;
  }
  lastBuffer = (void *) imageBuffer->memory;
  lastTime = image.timestamp;

  ImageOf<float> grayImage = image * GrayFloat;
  categoryImage.format = &GrayChar;
  categoryImage.resize (image.width, image.height);

  vector<bool> bits (P);

  int sourceL = (int) ceilf (R);
  int sourceR = (int) floorf (image.width - 1 - R);
  int sourceT = sourceL;
  int sourceB = (int) floorf (image.height - 1 - R);
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  int ones = 0;

	  float center = grayImage(x,y);
	  for (int i = 0; i < P; i++)
	  {
		float p;
		Interpolate & t = interpolates[i];
		if (t.exact)
		{
		  p = grayImage(x + t.xl, y + t.yl);
		}
		else
		{
		  int xl = x + t.xl;
		  int yl = y + t.yl;
		  int xh = x + t.xh;
		  int yh = y + t.yh;
		  p = grayImage(xl,yl) * t.wll + grayImage(xh,yl) * t.whl + grayImage(xl,yh) * t.wlh + grayImage(xh,yh) * t.whh;
		}
		bool sign = p >= center;
		bits[i] = sign;
		if (sign) ones++;
	  }

	  int transitions = bits.back () == bits.front () ? 0 : 1;
	  for (int i = 1; i < P; i++)
	  {
		if (bits[i-1] != bits[i])
		{
		  transitions++;
		}
	  }

	  if (transitions > 2)
	  {
		ones = P + 1;
	  }

	  categoryImage(x,y) = ones;
	}
  }
}

Vector<float>
DescriptorLBP::value (const Image & image, const PointAffine & point)
{
  Matrix<double> S = ! point.rectification ();
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;
  
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  if (S(0,1) == 0  &&  S(1,0) == 0)  // Special case: indicates that point describes a rectangular region in the image, rather than a patch.
  {
	double h = fabs (S(0,0) * supportRadial);
	double v = fabs (S(1,1) * supportRadial);
	sourceL = (int) rint (max (S(0,2) - h, (double) R));
	sourceR = (int) rint (min (S(0,2) + h, (double) image.width - 1 - R));
	sourceT = (int) rint (max (S(1,2) - v, (double) R));
	sourceB = (int) rint (min (S(1,2) + v, (double) image.height - 1 - R));
	preprocess (image);
  }
  else  // Shape change, so we must compute a transformed patch
  {
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (S, scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = image * t;
	preprocess (patch);

	sourceT = (int) ceilf (R);
	sourceL = sourceT;
	sourceB = (int) floorf (patchSize - 1 - R);
	sourceR = sourceB;
  }

  // Gather LBP values into histogram
  Vector<float> result (P + 2);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  result[categoryImage(x,y)]++;
	}
  }
  result /= result.frob (1);
  return result;
}

Vector<float>
DescriptorLBP::value (const Image & image)
{
  double start = getTimestamp ();
  preprocess (image);
  int sourceL = (int) ceilf (R);
  int sourceR = (int) floorf (image.width - 1 - R);
  int sourceT = sourceL;
  int sourceB = (int) floorf (image.height - 1 - R);
  Vector<float> result (P + 2);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  if (image.getAlpha (x, y))
	  {
		result[categoryImage(x,y)]++;
	  }
	}
  }
  cerr << "before normalization = " << result << " = " << result.frob (1) << endl;
  result /= result.frob (1);
  cerr << "time = " << getTimestamp () - start << endl;
  return result;
}

Image
DescriptorLBP::patch (const Vector<float> & value)
{
  Image result;
  return result;
}

Comparison *
DescriptorLBP::comparison ()
{
  return new ChiSquared;
}

void
DescriptorLBP::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &P,             sizeof (P));
  stream.read ((char *) &R,             sizeof (R));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));

  initialize ();
}

void
DescriptorLBP::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &P,             sizeof (P));
  stream.write ((char *) &R,             sizeof (R));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
}
