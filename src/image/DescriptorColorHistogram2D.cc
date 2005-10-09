/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Changed lapackd.h to lapack.h
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/lapack.h"
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
DescriptorColorHistogram2D::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  stream.write ((char *) &width,         sizeof (width));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
}
