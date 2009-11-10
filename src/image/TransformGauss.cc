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
  const double sigma2 = sigma * sigma;
  const double stepsPerZ = 6;  // Number of substeps per standard deviation
  const double C = 1.0 / (2.0 * PI * sigma2);

  // Calculate size and shape of Gaussian
  MatrixFixed<double,2,2> S = IA.region (0, 0, 1, 1) * ~IA.region (0, 0, 1, 1) * sigma2;
  sigmaX = sqrt (S(0,0));  // Distance of one standard deviation ("Z") from origin along x-axis in source image
  sigmaY = sqrt (S(1,1));  // ditto for y-axis
  Gshw = (int) ceil (sigmaX * 3);  // Desired size of kernel in source pixels
  Gshh = (int) ceil (sigmaY * 3);
  GstepX = max ((int) ceil (stepsPerZ / sigmaX), 1);  // (steps / source-pixel) = (steps / Z) / (source-pixels / Z)
  GstepY = max ((int) ceil (stepsPerZ / sigmaY), 1);
  G.resize ((2 * Gshw + 1) * GstepX,
			(2 * Gshh + 1) * GstepY);

  // For continuity in the destination image, the kernel must cover at
  // least 1 full pixel in the source image.
  double sigmaM = max (sigmaX, sigmaY);
  if (sigmaM < 0.5)
  {
	double adjust = 0.5 / sigmaM;
	S *= adjust * adjust;
  }

  // Compute Gaussian kernel
  S = !S;  // change from covariance matrix in source image to covariance matrix in destination image
  int hw = G.width / 2;
  int hh = G.height / 2;
  for (int y = 0; y < G.height; y++)
  {
	for (int x = 0; x < G.width; x++)
	{
	  double dx = (double) (x - hw) / GstepX;
	  double dy = (double) (y - hh) / GstepY;
	  double tx = S(0,0) * dx + S(0,1) * dy;
	  double ty = S(1,0) * dx + S(1,1) * dy;
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
  float * g = (float *) ((PixelBufferPacked *) G.buffer)->memory;

  MatrixFixed<double,3,3> H;  // homography from destination image to source image
  int w;
  int h;
  int lo;
  int hi;
  prepareResult (image, w, h, H, lo, hi);

  const int    iLastX1 = image.width  - 1;
  const int    iLastY1 = image.height - 1;
  const double firstX5 = -0.5 - sigmaX;
  const double firstY5 = -0.5 - sigmaY;
  const double lastX5  = image.width  - 0.5 + sigmaX;
  const double lastY5  = image.height - 0.5 + sigmaY;

  const double H00 = H(0,0);
  const double H10 = H(1,0);
  const double H20 = H(2,0);
  const double H01 = H(0,1);
  const double H11 = H(1,1);
  const double H21 = H(2,1);
  const double H02 = H(0,2);
  const double H12 = H(1,2);

  // One row + one pixel before beginning of destination image
  double tx = -H00 - H01 + H02;
  double ty = -H10 - H11 + H12;
  double tz = -H20 - H21 + 1.0;

  if (tz != 1.0)  // full 8-DOF homography
  {
	throw "TransformGauss does not yet handle 8-DOF homographies";
  }
  else  // tz == 1.0, meaning only 6-DOF homography, allowing slightly fewer computations for coordinates
  {
	if (*image.format == GrayFloat)
	{
	  PixelBufferPacked * fromBuffer = (PixelBufferPacked *) image.buffer;
	  float * fromMemory = (float *) fromBuffer->memory;
	  int fromStride = fromBuffer->stride;

	  ImageOf<float> result (w, h, GrayFloat);  // result is guaranteed to be dense because we construct it ourselves, therefore don't need to worry about stride
	  float * r = (float *) ((PixelBufferPacked *) result.buffer)->memory;

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		for (int toX = 0; toX < result.width; toX++)
		{
		  x += H00;
		  y += H10;
		  if (x > firstX5  &&  x < lastX5  &&  y > firstY5  &&  y < lastY5)
		  {
			int rx = (int) roundp (x);
			int ry = (int) roundp (y);
			int beginX = rx - Gshw;
			int beginY = ry - Gshh;
			int endX   = rx + Gshw;
			int endY   = ry + Gshh;
			endX = min (endX, iLastX1);
			endY = min (endY, iLastY1);
			int Gx = (int) ((0.499999 + rx - x) * GstepX);  // 0.499999 rather than 0.5 to ensure we get [0, GstepX) rather than [0, GstepX].
			int Gy = (int) ((0.499999 + ry - y) * GstepY);
			if (beginX < 0)
			{
			  Gx -= GstepX * beginX;
			  beginX = 0;
			}
			if (beginY < 0)
			{
			  Gy -= GstepY * beginY;
			  beginY = 0;
			}
			int fromBlockWidth = endX - beginX + 1;
			int fromStep       = fromStride - fromBlockWidth * sizeof (float);
			int gStep          = G.width * GstepY - fromBlockWidth * GstepX;
			float * f   = (float *) ((char *) fromMemory + (beginY    * fromStride + beginX * sizeof (float)));
			float * end = (float *) ((char *) fromMemory + (endY + 1) * fromStride);
			float * w   = g + (Gy * G.width + Gx);  // get beginning pixel in Gaussian kernel
			float weight = 0;
			float sum    = 0;
			while (f < end)
			{
			  float * rowEnd = f + fromBlockWidth;
			  while (f < rowEnd)
			  {
				weight += *w;
				sum += *f++ * *w;
				w += GstepX;
			  }
			  f = (float *) ((char *) f + fromStep);
			  w += gStep;
			}
			*r++ = sum / weight;
		  }
		  else
		  {
			*r++ = 0.0f;
		  }
		}
	  }
	  return result;
	}
	else if (*image.format == GrayDouble)
	{
	  PixelBufferPacked * fromBuffer = (PixelBufferPacked *) image.buffer;
	  double * fromMemory = (double *) fromBuffer->memory;
	  int fromStride = fromBuffer->stride;

	  ImageOf<double> result (w, h, GrayDouble);
	  double * r = (double *) ((PixelBufferPacked *) result.buffer)->memory;

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		for (int toX = 0; toX < result.width; toX++)
		{
		  x += H00;
		  y += H10;
		  if (x > firstX5  &&  x < lastX5  &&  y > firstY5  &&  y < lastY5)
		  {
			int rx = (int) roundp (x);
			int ry = (int) roundp (y);
			int beginX = rx - Gshw;
			int beginY = ry - Gshh;
			int endX   = rx + Gshw;
			int endY   = ry + Gshh;
			endX = min (endX, iLastX1);
			endY = min (endY, iLastY1);
			int Gx = (int) ((0.499999 + rx - x) * GstepX);  // 0.499999 rather than 0.5 to ensure we get [0, GstepX) rather than [0, GstepX].
			int Gy = (int) ((0.499999 + ry - y) * GstepY);
			if (beginX < 0)
			{
			  Gx -= GstepX * beginX;
			  beginX = 0;
			}
			if (beginY < 0)
			{
			  Gy -= GstepY * beginY;
			  beginY = 0;
			}
			int fromBlockWidth = endX - beginX + 1;
			int fromStep       = fromStride - fromBlockWidth * sizeof (double);
			int gStep          = G.width * GstepY - fromBlockWidth * GstepX;
			double * f   = (double *) ((char *) fromMemory + (beginY    * fromStride + beginX * sizeof (double)));
			double * end = (double *) ((char *) fromMemory + (endY + 1) * fromStride);
			float *  w   = g + (Gy * G.width + Gx);
			double weight = 0;
			double sum    = 0;
			while (f < end)
			{
			  double * rowEnd = f + fromBlockWidth;
			  while (f < rowEnd)
			  {
				weight += *w;
				sum += *f++ * *w;
				w += GstepX;
			  }
			  f = (double *) ((char *) f + fromStep);
			  w += gStep;
			}

			*r++ = sum / weight;
		  }
		  else
		  {
			*r++ = 0.0;
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
		double x = tx += H01;
		double y = ty += H11;

		for (int toX = 0; toX < result.width; toX++)
		{
		  x += H00;
		  y += H10;
		  if (x > firstX5  &&  x < lastX5  &&  y > firstY5  &&  y < lastY5)
		  {
			int rx = (int) roundp (x);
			int ry = (int) roundp (y);
			int beginX = rx - Gshw;
			int beginY = ry - Gshh;
			int endX   = rx + Gshw;
			int endY   = ry + Gshh;
			endX = min (endX, (image.width - 1));
			endY = min (endY, (image.height - 1));
			float weight = 0;
			float sum[] = {0, 0, 0, 0};
			int Gx = (int) ((0.499999 + rx - x) * GstepX);  // 0.499999 rather than 0.5 to ensure we get [0, GstepX) rather than [0, GstepX].
			int Gy = (int) ((0.499999 + ry - y) * GstepY);
			if (beginX < 0)
			{
			  Gx -= GstepX * beginX;
			  beginX = 0;
			}
			if (beginY < 0)
			{
			  Gy -= GstepY * beginY;
			  beginY = 0;
			}
			int fromBlockWidth = endX - beginX + 1;
			int gStep = G.width * GstepY - fromBlockWidth * GstepX;
			float * w = g + (Gy * G.width + Gx);
			for (int fromY = beginY; fromY <= endY; fromY++)
			{
			  for (int fromX = beginX; fromX <= endX; fromX++)
			  {
				weight += *w;
				float pixel[4];
				image.getRGBA (fromX, fromY, pixel);
				sum[0] += pixel[0] * *w;
				sum[1] += pixel[1] * *w;
				sum[2] += pixel[2] * *w;
				sum[3] += pixel[3] * *w;
				w += GstepX;
			  }
			  w += gStep;
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
}
