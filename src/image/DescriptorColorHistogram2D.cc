/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.9, 1.11 thru 1.13 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.15  2007/03/23 02:32:06  Fred
Use CVS Log to generate revision history.

Revision 1.14  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.13  2005/10/09 05:06:11  Fred
Remove lapack.h, as it is no longer necessary to obtain matrix inversion
operator.

Revision 1.12  2005/10/09 04:45:05  Fred
Rename lapack?.h to lapack.h.  Add Sandia copyright notice.

Revision 1.11  2005/09/10 16:39:04  Fred
Clarify revision history.

Revision 1.10  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.9  2005/01/22 21:07:16  Fred
MSVC compilability fix:  Use fl/math.h.  Replace GNU operator with min() and
max().

Revision 1.8  2004/09/15 18:51:33  rothgang
Add comment about prefering odd-sized histograms.

Revision 1.7  2004/08/29 16:21:12  rothgang
Change certain attributes of Descriptor from functions to member variables:
supportRadial, monochrome, dimension.  Elevated supportRadial to a member of
the base classs.  It is very common, but not 100% common, so there is a little
wasted storage in a couple of cases.  On the other hand, this allows for client
code to determine what support was used for a descriptor on an affine patch.

Modified read() and write() functions to call base class first, and moved task
of writing name into the base class.  May move task of writing supportRadial
into base class as well, but leaving it as is for now.

Revision 1.6  2004/05/03 18:55:13  rothgang
Add Factory.

Revision 1.5  2004/03/22 20:30:55  rothgang
Switch to simple chi^2 for comparison.

Revision 1.4  2004/02/15 18:51:31  rothgang
Update for new items in Descriptor interface: isMonochrome(), comparison(),
dimension().  Use 1.0 rather than intensity of pixel for binning.  Normalize
resulting histogram to sum to 1 (probability distribution) rather than have
unit norm.  This makes it more ammenable to ChiSquared and
HistorgramIntersection comparison methods.

Revision 1.3  2004/01/14 18:12:27  rothgang
Set monochrome to false.

Revision 1.2  2004/01/08 21:26:47  rothgang
Add support for regions selected by alpha channel.

Revision 1.1  2003/12/30 21:06:22  rothgang
Create color histogram descriptor.
-------------------------------------------------------------------------------
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/math.h"


using namespace fl;
using namespace std;



// class DescriptorColorHistogram2D -------------------------------------------

/**
   \param width An odd value provides a bin centered exactly on white, which
   may improve color matching.
 **/
DescriptorColorHistogram2D::DescriptorColorHistogram2D (int width, float supportRadial)
{
  this->width         = width;
  this->supportRadial = supportRadial;

  initialize ();
}

DescriptorColorHistogram2D::DescriptorColorHistogram2D (istream & stream)
{
  read (stream);
}

void
DescriptorColorHistogram2D::initialize ()
{
  monochrome = false;
  dimension = 0;
  valid.resize (width, width);
  valid.clear ();
  for (int u = 0; u < width; u++)
  {
	float uf = (u + 0.5f) / width - 0.5f;
	for (int v = 0; v < width; v++)
	{
	  float vf = (v + 0.5f) / width - 0.5f;

	  // The following is based on the YUV-to-RGB conversion matrix found
	  // in PixelFormatYVYUChar::getRGBA().
	  // The idea is to find the range of Y values for which the YUV value
	  // converts into an RGB value that is in range.  As long as some
	  // part of the Y value range is valid, the YUV color is valid.
	  float tr =                1.4022f * vf;
	  float tg = -0.3456f * uf -0.7145f * vf;
	  float tb =  1.7710f * uf;
	  float yl = 0;
	  float yh = 1;
	  yl = max (yl, 0.0f - tr);
	  yh = min (yh, 1.0f - tr);
	  yl = max (yl, 0.0f - tg);
	  yh = min (yh, 1.0f - tg);
	  yl = max (yl, 0.0f - tb);
	  yh = min (yh, 1.0f - tb);
	  if (yh > yl)
	  {
		valid(u,v) = true;
		dimension++;
	  }
	}
  }
}

void
DescriptorColorHistogram2D::clear ()
{
  histogram.resize (width, width);
  histogram.clear ();
}

union YUV
{
  unsigned int all;
  struct
  {
	unsigned char v;
	unsigned char u;
	unsigned char y;
  };
};

inline void
DescriptorColorHistogram2D::addToHistogram (const Image & image, const int x, const int y)
{
  YUV yuv;
  yuv.all = image.getYUV (x, y);
  //float weight = yuv.y / 255.0f;
  float weight = 1.0f;  // All pixels count the same, regardless of intensity.  This is the right way to collapse the intensity dimension of a 3D histogram.
  float uf = yuv.u * width / 256.0f - 0.5f;
  float vf = yuv.v * width / 256.0f - 0.5f;
  int ul = (int) floorf (uf);
  int uh = ul + 1;
  int vl = (int) floorf (vf);
  int vh = vl + 1;
  uf -= ul;
  vf -= vl;

  // If one of these values is clipped, then all the weight will go to
  // a single row or column of 2D histogram.
  ul = max (ul, 0);
  uh = min (uh, width - 1);
  vl = max (vl, 0);
  vh = min (vh, width - 1);

  // Use bilinear method to distribute weight to 4 adjacent bins in
  // histogram.
  float uweight = (1.0f - uf) * weight;
  histogram(ul,vl) += (1.0f - vf) * uweight;
  histogram(ul,vh) +=         vf  * uweight;
  uweight       =         uf * weight;
  histogram(uh,vl) += (1.0f - vf) * uweight;
  histogram(uh,vh) +=         vf  * uweight;
}

void
DescriptorColorHistogram2D::add (const Image & image, const int x, const int y)
{
  addToHistogram (image, x, y);
}

Vector<float>
DescriptorColorHistogram2D::finish ()
{
  Vector<float> result (dimension);
  int i = 0;
  for (int u = 0; u < width; u++)
  {
	for (int v = 0; v < width; v++)
	{
	  if (valid(u,v))
	  {
		result[i++] = histogram(u,v);
	  }
	}
  }
  result /= result.frob (1);  // normalize to a probability distribution

  return result;
}

Vector<float>
DescriptorColorHistogram2D::value (const Image & image, const PointAffine & point)
{
  // Prepare to project patch into image.

  Matrix<double> R = point.rectification ();
  Matrix<double> S = ! R;
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;
  
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;

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

  sourceL = (int) floorf (max (min (ptl.x, ptr.x, pbl.x, pbr.x), 0.0f));
  sourceR = (int) ceilf  (min (max (ptl.x, ptr.x, pbl.x, pbr.x), image.width - 1.0f));
  sourceT = (int) floorf (max (min (ptl.y, ptr.y, pbl.y, pbr.y), 0.0f));
  sourceB = (int) ceilf  (min (max (ptl.y, ptr.y, pbl.y, pbr.y), image.height - 1.0f));

  // Gather color values into histogram
  clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  Point q = R * Point (x, y);
	  if (fabsf (q.x) <= supportRadial  &&  fabsf (q.y) <= supportRadial)
	  {
		addToHistogram (image, x, y);
	  }
	}
  }
  return finish ();
}

Vector<float>
DescriptorColorHistogram2D::value (const Image & image)
{
  // Gather color values into histogram
  clear ();
  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  if (image.getAlpha (x, y))
	  {
		addToHistogram (image, x, y);
	  }
	}
  }
  return finish ();
}

Image
DescriptorColorHistogram2D::patch (const Vector<float> & value)
{
  Image result (width, width, RGBAChar);
  result.clear ();

  float maximum = value.frob (INFINITY);

  YUV yuv;
  yuv.all = 0;
  int i = 0;
  for (int u = 0; u < width; u++)
  {
	for (int v = 0; v < width; v++)
	{
	  if (valid(u,v))
	  {
		yuv.y = (unsigned char) (255    * value[i++] / maximum);
		if (yuv.y > 0)
		{
		  yuv.u = (unsigned char) (255.0f * (u + 0.5f) / width);
		  yuv.v = (unsigned char) (255.0f * (v + 0.5f) / width);
		  result.setYUV (u, v, yuv.all);
		}
	  }
	}
  }

  return result;
}

Comparison *
DescriptorColorHistogram2D::comparison ()
{
  return new ChiSquared;
}

void
DescriptorColorHistogram2D::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &width,         sizeof (width));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));

  initialize ();
}

void
DescriptorColorHistogram2D::write (std::ostream & stream) const
{
  Descriptor::write (stream);

  stream.write ((char *) &width,         sizeof (width));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
}
