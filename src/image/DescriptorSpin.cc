/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.6, 1.8 and 1.9 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.11  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.10  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.9  2005/10/09 05:08:08  Fred
Remove lapack.h, as it is no longer necessary to obtain matrix inversion
operator.  Add Sandia copyright notice.

Revision 1.8  2005/09/10 16:57:26  Fred
Add detail to revision history.

Revision 1.7  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.6  2005/01/22 21:12:46  Fred
MSVC compilability fix:  Replace GNU operator with min() and max().

Revision 1.5  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.4  2004/03/22 20:35:39  rothgang
Remove probability transformation from comparison.  Switched to chi^2.

Revision 1.3  2004/02/15 18:33:30  rothgang
Simplify and modernize.  Moved major functions back into value().  Directly
transform image pixels into patch space, immediately compute point in spin
space, and use bilinear binning.  The current version is extremely fast
compared to any previous version, and probably just as discriminating.

Revision 1.2  2004/01/23 23:16:20  rothgang
Simplified.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorSpin -------------------------------------------------------

DescriptorSpin::DescriptorSpin (int binsRadial, int binsIntensity, float supportRadial, float supportIntensity)
{
  this->binsRadial       = binsRadial;
  this->binsIntensity    = binsIntensity;
  this->supportRadial    = supportRadial;
  this->supportIntensity = supportIntensity;
  dimension = binsRadial * binsIntensity;
}

DescriptorSpin::DescriptorSpin (std::istream & stream)
{
  read (stream);
}

Vector<float>
DescriptorSpin::value (const Image & image, const PointAffine & point)
{
  // Determine square region in source image to scan

  Matrix<double> R = point.rectification ();
  Matrix<double> S = !R;

  Vector<double> tl (3);
  tl[0] = -supportRadial;
  tl[1] = supportRadial;
  tl[2] = 1;
  Vector<double> tr (3);
  tr[0] = supportRadial;
  tr[1] = supportRadial;
  tr[2] = 1;
  Vector<double> bl (3);
  bl[0] = -supportRadial;
  bl[1] = -supportRadial;
  bl[2] = 1;
  Vector<double> br (3);
  br[0] = supportRadial;
  br[1] = -supportRadial;
  br[2] = 1;

  Point ptl = S * tl;
  Point ptr = S * tr;
  Point pbl = S * bl;
  Point pbr = S * br;

  int sourceL = (int) rint (max (min (ptl.x, ptr.x, pbl.x, pbr.x), 0.0f));
  int sourceR = (int) rint (min (max (ptl.x, ptr.x, pbl.x, pbr.x), image.width - 1.0f));
  int sourceT = (int) rint (max (min (ptl.y, ptr.y, pbl.y, pbr.y), 0.0f));
  int sourceB = (int) rint (min (max (ptl.y, ptr.y, pbl.y, pbr.y), image.height - 1.0f));

  R.region (0, 0, 1, 2) *= binsRadial / supportRadial;  // Now R maps directly to radial bin values

  // Determine mapping between pixel values and intensity bins
  ImageOf<float> that (image);
  float average = 0;
  float count = 0;
  Point q;
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float radius = (R * q).distance ();
	  if (radius < binsRadial)
	  {
		float weight = 1.0f - radius / binsRadial;
		average += that(x,y) * weight;
		count += weight;
	  }
	}
  }
  average /= count;
  float deviation = 0;
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float radius = (R * q).distance ();
	  if (radius < binsRadial)
	  {
		float d = that(x,y) - average;
		float weight = 1.0f - radius / binsRadial;
		deviation += d * d * weight;
	  }
	}
  }
  deviation = sqrt (deviation / count);
  float range = 2.0f * supportIntensity * deviation;
  if (range == 0.0f)  // In case the image is completely flat
  {
	range = 1.0f;
  }
  float quantum = range / binsIntensity;
  float minIntensity = average - range / 2 + 0.5f * quantum;

  // Bin up all the pixels
  Matrix<float> result (binsIntensity, binsRadial);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float rf = (R * q).distance () - 0.5f;
	  if (rf < binsRadial)
	  {
		int rl = (int) floorf (rf);
		int rh = rl + 1;
		rf -= rl;
		float rf1 = 1.0f - rf;
		if (rh > binsRadial - 1)
		{
		  rh = binsRadial - 1;
		  rf = 0.0f;
		}
		rl = max (rl, 0);

		float df = (that(x,y) - minIntensity) / quantum;
		int dl = (int) floorf (df);
		int dh = dl + 1;
		df -= dl;
		float df1 = 1.0f - df;
		if (dl < 0)
		{
		  dl = 0;
		  dh = 0;
		}
		if (dh > binsIntensity - 1)
		{
		  dl = binsIntensity - 1;
		  dh = binsIntensity - 1;
		}

		result(dl,rl) += df1 * rf1;
		result(dl,rh) += df1 * rf;
		result(dh,rl) += df  * rf1;
		result(dh,rh) += df  * rf;
	  }
	}
  }

  // Convert to probabilities
  for (int r = 0; r < binsRadial; r++)
  {
	float sum = result.column (r).frob (1);
	result.column (r) /= sum;
  }

  return result;
}

Image
DescriptorSpin::patch (const Vector<float> & value)
{
  ImageOf<float> result (binsRadial, binsIntensity, GrayFloat);

  for (int r = 0; r < binsRadial; r++)
  {
	for (int d = 0; d < binsIntensity; d++)
	{
	  result (r, d) = 1.0 - value[(r * binsIntensity) + (binsIntensity - d - 1)];
	}
  }

  return result;
}

Comparison *
DescriptorSpin::comparison ()
{
  //return new HistogramIntersection;
  return new ChiSquared;
}

void
DescriptorSpin::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &binsRadial,       sizeof (binsRadial));
  stream.read ((char *) &binsIntensity,    sizeof (binsIntensity));
  stream.read ((char *) &supportRadial,    sizeof (supportRadial));
  stream.read ((char *) &supportIntensity, sizeof (supportIntensity));

  dimension = binsRadial * binsIntensity;
}

void
DescriptorSpin::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &binsRadial,       sizeof (binsRadial));
  stream.write ((char *) &binsIntensity,    sizeof (binsIntensity));
  stream.write ((char *) &supportRadial,    sizeof (supportRadial));
  stream.write ((char *) &supportIntensity, sizeof (supportIntensity));
}
