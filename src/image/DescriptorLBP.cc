/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"
#include "fl/canvas.h"

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

void
DescriptorLBP::initialize ()
{
  dimension = P + 2;
  lastBuffer = 0;

  interpolates.resize (P);
  for (int i = 0; i < P; i++)
  {
	Interpolate & t = interpolates[i];
	float angle = i * TWOPIf / P;
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
	sourceL = (int) roundp (max (S(0,2) - h, (double) R));
	sourceR = (int) roundp (min (S(0,2) + h, (double) image.width - 1 - R));
	sourceT = (int) roundp (max (S(1,2) - v, (double) R));
	sourceB = (int) roundp (min (S(1,2) + v, (double) image.height - 1 - R));
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
  result /= result.norm (1);
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
  cerr << "before normalization = " << result << " = " << result.norm (1) << endl;
  result /= result.norm (1);
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
DescriptorLBP::serialize (Archive & archive, uint32_t version)
{
  archive & *((Descriptor *) this);
  archive & P;
  archive & R;
  archive & supportRadial;
  archive & supportPixel;

  if (archive.in) initialize ();
}
