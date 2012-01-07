/*
Author: Fred Rothganger
Created 12/22/2011


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class BlurDecimate ---------------------------------------------------------

BlurDecimate::BlurDecimate (int ratioX, double sigmaXbefore, double sigmaXafter,
							int ratioY, double sigmaYbefore, double sigmaYafter)
: ratioX       (ratioX),
  sigmaXbefore (sigmaXbefore),
  sigmaXafter  (sigmaXafter),
  ratioY       (ratioY),
  sigmaYbefore (sigmaYbefore),
  sigmaYafter  (sigmaYafter)
{
  blurX.width = 0;  // lie, but should not affect memory management, so safe
  blurY.width = 0;
}

Image
BlurDecimate::filter (const Image & image)
{
  const int ratioY = this->ratioY >  0 ? this->ratioY : ratioX;

  // Prepare kernels on first call
  if (blurX.width == 0)
  {
	double a = sigmaXafter * ratioX;
	double b = sigmaXbefore;  // for ease of reading
	double s = sqrt (a * a - b * b);
	blurX = Gaussian1D (s, Boost, GrayFloat, Horizontal);
  }
  if (blurY.width == 0)
  {
	const double sigmaYbefore = this->sigmaYbefore ? this->sigmaYbefore : sigmaXbefore;
	const double sigmaYafter  = this->sigmaYafter  ? this->sigmaYafter  : sigmaXafter;
	double a = sigmaYafter * ratioY;
	double b = sigmaYbefore;
	double s = sqrt (a * a - b * b);
	blurY = Gaussian1D (s, Boost, GrayFloat, Vertical);
  }

  // Blur and downsample

  Image temp = image * GrayFloat * blurX;
  ImageOf<float> result (image.width / ratioX, image.height / ratioY, GrayFloat);

  int startX = (ratioX > 2) ? ratioX / 2 : 0;
  int startY = (ratioY > 2) ? ratioY / 2 : 0;

  Point t;
  t.y = startY;
  for (int y = 0; y < result.height; y++)
  {
	t.x = startX;
	for (int x = 0; x < result.width; x++)
	{
	  result(x,y) = blurY.response (temp, t);
	  t.x += ratioX;
	}
	t.y += ratioY;
  }

  return result;
}
