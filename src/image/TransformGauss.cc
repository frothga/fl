/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4, 1.6 and 1.7 Copyright 2005 Sandia Corporation.
Revisions 1.9 thru 1.10    Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.10  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.9  2006/01/16 05:43:16  Fred
Zero out pixels in output image that fall out of bounds in input image. 
Previously, they were being left undefined!

Revision 1.8  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.7  2005/10/09 05:31:13  Fred
Update revision history and add Sandia copyright notice.

Revision 1.6  2005/04/23 20:59:17  Fred
Keep in sync with full homography version of Transform.cc.

Revision 1.5  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.4  2005/01/22 21:18:26  Fred
MSVC compilability fix:  Replace GNU operator with min() and max().

Revision 1.3  2003/07/18 13:13:53  rothgang
Update the "all formats" part of the filter to handle data more precisely. 
Before, precision was being lost by converting back to integer type when
storing intermediate results in the inner loop.  This made some transformations
look blochy.  Added code to the kernel generation function to change the scale
of the kernel when magnifying the source image.  Before, the kernel could be
much smaller than the radius of one pixel in the source image, so there was no
benefit from Gaussian interpolation.  Now the magnified images come out with
slight smoothing along pixel boundaries, rather than hard edges.

Revision 1.2  2003/07/09 15:42:48  rothgang
Use homography matrix in filter() method.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/convolve.h"
#include "fl/pi.h"
#include "fl/color.h"

#include <float.h>


// For debugging
//#include "fl/slideshow.h"


using namespace std;
using namespace fl;


// class TransformGauss ------------------------------------------------------------

/**
   Generate Gaussian kernel and associated parameters.
   \todo This class does not handle full 8-DOF homographies.  To do so, it
   must rescale the kernel based on position in the image.  This method
   assumes a fixed-size kernel.
 **/
void
TransformGauss::prepareG ()
{
  const float sigma = 0.5;    // radius of support; 0.5 since we want half a pixel in all directions
  const float sigma2 = sigma * sigma;
  const float stepsPerZ = 6;  // Number of substeps per standard deviation
  const float C = 1.0 / (2.0 * PI * sigma2);

  // Calculate size and shape of Gaussian
  MatrixFixed<float,2,2> S = IA.region (0, 0, 1, 1) * ~IA.region (0, 0, 1, 1) * sigma2;
  sigmaX = sqrt (S(0,0));  // Distance of one standard deviation ("Z") from origin along x-axis in source image
  sigmaY = sqrt (S(1,1));  // ditto for y-axis
  Gshw = (int) ceil (sigmaX * 3);  // Desired size of kernel in source pixels
  Gshh = (int) ceil (sigmaY * 3);
  GstepX = max ((int) ceil (stepsPerZ / sigmaX), 1);  // (steps / source-pixel) = (steps / Z) / (source-pixels / Z)
  GstepY = max ((int) ceil (stepsPerZ / sigmaY), 1);
  G.resize ((2 * Gshw + 1) * GstepX,
			(2 * Gshh + 1) * GstepY);
  S = !S;
  float sigmaM = max (sigmaX, sigmaY);
  if (sigmaM < sigma)  // try to match support of kernel in destination image when zooming up
  {
	float adjust = sigmaM / 0.5;
	S *= adjust * adjust;
  }

  // Compute Gaussian kernel
  int hw = G.width / 2;
  int hh = G.height / 2;
  for (int y = 0; y < G.height; y++)
  {
	for (int x = 0; x < G.width; x++)
	{
	  float dx = (float) (x - hw) / GstepX;
	  float dy = (float) (y - hh) / GstepY;
	  float tx = S(0,0) * dx + S(0,1) * dy;
	  float ty = S(1,0) * dx + S(1,1) * dy;
	  G (x, y) = C * expf (-0.5 * (dx * tx + dy * ty));
	}
  }

  needG = false;
}

Image
TransformGauss::filter (const Image & image)
{
  if (needG)
  {
	prepareG ();
  }

  MatrixFixed<double,3,3> H;  // homography from destination image to source image
  int w;
  int h;
  int lo;
  int hi;
  prepareResult (image, w, h, H, lo, hi);

  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (w, h, GrayFloat);
	ImageOf<float> that (image);
	for (int toY = 0; toY < result.height; toY++)
	{
	  for (int toX = 0; toX < result.width; toX++)
	  {
		float x = H(0,0) * toX + H(0,1) * toY + H(0,2);
		float y = H(1,0) * toX + H(1,1) * toY + H(1,2);
		float z = H(2,0) * toX + H(2,1) * toY + H(2,2);
		x /= z;
		y /= z;
		if (x > -0.5 - sigmaX  &&  x < image.width - 0.5 + sigmaX  &&  y > -0.5 - sigmaY  &&  y < image.height - 0.5 + sigmaY)
		{
		  int rx = (int) rint (x);
		  int ry = (int) rint (y);
		  int beginX = rx - Gshw;
		  int beginY = ry - Gshh;
		  int endX   = rx + Gshw;
		  int endY   = ry + Gshh;
		  endX = min (endX, (image.width - 1));
		  endY = min (endY, (image.height - 1));
		  float weight = 0;
		  float sum = 0;
		  int Gx      = (int) ((0.499999 + rx - x) * GstepX);  // 0.499999 rather than 0.5 to ensure we get [0, GstepX) rather than [0, GstepX].
		  int offsetY = (int) ((0.499999 + ry - y) * GstepY);
		  if (beginX < 0)
		  {
			Gx -= GstepX * beginX;
			beginX = 0;
		  }
		  if (beginY < 0)
		  {
			offsetY -= GstepY * beginY;
			beginY = 0;
		  }
		  for (int fromX = beginX; fromX <= endX; fromX++)
		  {
			int Gy = offsetY;
			for (int fromY = beginY; fromY <= endY; fromY++)
			{
			  float w = G (Gx, Gy);
			  weight += w;
			  sum += that (fromX, fromY) * w;
			  Gy += GstepY;
			}
			Gx += GstepX;
		  }
		  result (toX, toY) = sum / weight;
		}
		else
		{
		  result (toX, toY) = 0.0f;
		}
	  }
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> result (w, h, GrayDouble);
	ImageOf<double> that (image);
	for (int toY = 0; toY < result.height; toY++)
	{
	  for (int toX = 0; toX < result.width; toX++)
	  {
		double x = H(0,0) * toX + H(0,1) * toY + H(0,2);
		double y = H(1,0) * toX + H(1,1) * toY + H(1,2);
		double z = H(2,0) * toX + H(2,1) * toY + H(2,2);
		x /= z;
		y /= z;
		if (x > -0.5 - sigmaX  &&  x < image.width - 0.5 + sigmaX  &&  y > -0.5 - sigmaY  &&  y < image.height - 0.5 + sigmaY)
		{
		  int rx = (int) rint (x);
		  int ry = (int) rint (y);
		  int beginX = rx - Gshw;
		  int beginY = ry - Gshh;
		  int endX   = rx + Gshw;
		  int endY   = ry + Gshh;
		  endX = min (endX, (image.width - 1));
		  endY = min (endY, (image.height - 1));
		  double weight = 0;
		  double sum = 0;
		  int Gx      = (int) ((0.499999 + rx - x) * GstepX);  // 0.499999 rather than 0.5 to ensure we get [0, GstepX) rather than [0, GstepX].
		  int offsetY = (int) ((0.499999 + ry - y) * GstepY);
		  if (beginX < 0)
		  {
			Gx -= GstepX * beginX;
			beginX = 0;
		  }
		  if (beginY < 0)
		  {
			offsetY -= GstepY * beginY;
			beginY = 0;
		  }
		  for (int fromX = beginX; fromX <= endX; fromX++)
		  {
			int Gy = offsetY;
			for (int fromY = beginY; fromY <= endY; fromY++)
			{
			  double w = G (Gx, Gy);
			  weight += w;
			  sum += that (fromX, fromY) * w;
			  Gy += GstepY;
			}
			Gx += GstepX;
		  }
		  result (toX, toY) = sum / weight;
		}
		else
		{
		  result (toX, toY) = 0.0;
		}
	  }
	}
	return result;
  }
  else if (*image.format == GrayChar)
  {
	return filter (image * GrayFloat);
  }
  else
  {
	Image result (w, h, *image.format);
	for (int toY = 0; toY < result.height; toY++)
	{
	  for (int toX = 0; toX < result.width; toX++)
	  {
		float x = H(0,0) * toX + H(0,1) * toY + H(0,2);
		float y = H(1,0) * toX + H(1,1) * toY + H(1,2);
		float z = H(2,0) * toX + H(2,1) * toY + H(2,2);
		x /= z;
		y /= z;
		if (x > -0.5 - sigmaX  &&  x < image.width - 0.5 + sigmaX  &&  y > -0.5 - sigmaY  &&  y < image.height - 0.5 + sigmaY)
		{
		  int rx = (int) rint (x);
		  int ry = (int) rint (y);
		  int beginX = rx - Gshw;
		  int beginY = ry - Gshh;
		  int endX   = rx + Gshw;
		  int endY   = ry + Gshh;
		  endX = min (endX, (image.width - 1));
		  endY = min (endY, (image.height - 1));
		  float weight = 0;
		  float sum[] = {0, 0, 0, 0};
		  int Gx      = (int) ((0.499999 + rx - x) * GstepX);  // 0.499999 rather than 0.5 to ensure we get [0, GstepX) rather than [0, GstepX].
		  int offsetY = (int) ((0.499999 + ry - y) * GstepY);
		  if (beginX < 0)
		  {
			Gx -= GstepX * beginX;
			beginX = 0;
		  }
		  if (beginY < 0)
		  {
			offsetY -= GstepY * beginY;
			beginY = 0;
		  }
		  for (int fromX = beginX; fromX <= endX; fromX++)
		  {
			int Gy = offsetY;
			for (int fromY = beginY; fromY <= endY; fromY++)
			{
			  float w = G (Gx, Gy);
			  weight += w;
			  float pixel[4];
			  image.getRGBA (fromX, fromY, pixel);
			  sum[0] += pixel[0] * w;
			  sum[1] += pixel[1] * w;
			  sum[2] += pixel[2] * w;
			  sum[3] += pixel[3] * w;
			  Gy += GstepY;
			}
			Gx += GstepX;
		  }
		  sum[0] /= weight;
		  sum[1] /= weight;
		  sum[2] /= weight;
		  sum[3] /= weight;
		  result.setRGBA (toX, toY, sum);
		}
		else
		{
		  result.setRGBA (toX, toY, BLACK);
		}
	  }
	}
	return result;
  }
}
