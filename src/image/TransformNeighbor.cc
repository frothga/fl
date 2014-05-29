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
#include "fl/lapack.h"


using namespace std;
using namespace fl;


struct RGBA
{
  float r;
  float g;
  float b;
  float a;
};


// class TransformNeighbor ------------------------------------------------------------

Image
TransformNeighbor::filter (const Image & image)
{
  MatrixFixed<double,3,3> H;  // homography from destination image to source image
  int w;
  int h;
  int lo;
  int hi;
  prepareResult (image, w, h, H, lo, hi);

  const int    iLastX1 = image.width  - 1;
  const int    iLastY1 = image.height - 1;
  const double lastX5  = image.width  - 0.5;
  const double lastY5  = image.height - 0.5;

  const double H00 = H(0,0);
  const double H10 = H(1,0);
  const double H20 = H(2,0);
  const double H01 = H(0,1);
  const double H11 = H(1,1);
  const double H21 = H(2,1);
  const double H02 = H(0,2);
  const double H12 = H(1,2);
  // no need for H22 because it is guaranteed to be 1 by prepareResult()

  // One row + one pixel before beginning of destination image
  double tx = -H00 - H01 + H02;
  double ty = -H10 - H11 + H12;
  double tz = -H20 - H21 + 1.0;

  // There are two versions of the transformation code.  One handles all
  // 8 degrees of freedom of a homography, and the other is optimized for
  // the case when the homography is a pure affine transformation (6-DOF).

  if (tz != 1.0)  // full 8-DOF homography
  {
	if (*image.format == GrayFloat)
	{
	  ImageOf<float> result (w, h, GrayFloat);
	  ImageOf<float> that (image);
	  float * r = (float *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;
		double z = tz += H21;

		// There are two versions of the inner loop, depending on whether we will
		// encounter the edge of the image during this row or not.
		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			*r++ = that ((int) (cx + 0.5), (int) (cy + 0.5));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) (cx + 0.5);
			int fromY = (int) (cy + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  *r++ = that(fromX,fromY);
			}
			else
			{
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
	else if (*image.format == GrayDouble)
	{
	  ImageOf<double> result (w, h, GrayDouble);
	  ImageOf<double> that (image);
	  double * r = (double *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;
		double z = tz += H21;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			*r++ = that ((int) (cx + 0.5), (int) (cy + 0.5));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) (cx + 0.5);
			int fromY = (int) (cy + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  *r++ = that(fromX,fromY);
			}
			else
			{
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
	else if (*image.format == RGBAFloat)
	{
	  ImageOf<RGBA> result (w, h, RGBAFloat);
	  ImageOf<RGBA> that (image);
	  RGBA * r = &result(0,0);

	  RGBA zeroes;
	  zeroes.r = 0.0f;
	  zeroes.g = 0.0f;
	  zeroes.b = 0.0f;
	  zeroes.a = 0.0f;

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;
		double z = tz += H21;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			*r++ = that ((int) (cx + 0.5), (int) (cy + 0.5));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) (cx + 0.5);
			int fromY = (int) (cy + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  *r++ = that(fromX,fromY);
			}
			else
			{
			  *r++ = zeroes;
			}
		  }
		}
	  }
	  return result;
	}
	else  // Any
	{
	  Image result (w, h, *image.format);

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;
		double z = tz += H21;

		// There are two versions of the inner loop, depending on whether we will
		// encounter the edge of the image during this row or not.
		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) (cx + 0.5);
			int fromY = (int) (cy + 0.5);
			result.setRGBA (toX, toY, image.getRGBA (fromX, fromY));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			z += H20;
			double cx = x / z;
			double cy = y / z;
			int fromX = (int) (cx + 0.5);
			int fromY = (int) (cy + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  result.setRGBA (toX, toY, image.getRGBA (fromX, fromY));
			}
			else
			{
			  result.setRGBA (toX, toY, (uint32_t) 0);
			}
		  }
		}
	  }
	  return result;
	}
  }
  else  // tz == 1.0, meaning only 6-DOF homography, allowing slightly fewer computations for coordinates
  {
	if (*image.format == GrayFloat)
	{
	  ImageOf<float> result (w, h, GrayFloat);
	  ImageOf<float> that (image);
	  float * r = (float *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			*r++ = that ((int) (x + 0.5), (int) (y + 0.5));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			int fromX = (int) (x + 0.5);
			int fromY = (int) (y + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  *r++ = that(fromX,fromY);
			}
			else
			{
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
	else if (*image.format == GrayDouble)
	{
	  ImageOf<double> result (w, h, GrayDouble);
	  ImageOf<double> that (image);
	  double * r = (double *) ((PixelBufferPacked *) result.buffer)->base ();

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			*r++ = that ((int) (x + 0.5), (int) (y + 0.5));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			int fromX = (int) (x + 0.5);
			int fromY = (int) (y + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  *r++ = that(fromX,fromY);
			}
			else
			{
			  *r++ = 0.0f;
			}
		  }
		}
	  }
	  return result;
	}
	else if (*image.format == RGBAFloat)
	{
	  ImageOf<RGBA> result (w, h, RGBAFloat);
	  ImageOf<RGBA> that (image);
	  RGBA * r = &result(0,0);

	  RGBA zeroes;
	  zeroes.r = 0.0f;
	  zeroes.g = 0.0f;
	  zeroes.b = 0.0f;
	  zeroes.a = 0.0f;

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			*r++ = that ((int) (x + 0.5), (int) (y + 0.5));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			int fromX = (int) (x + 0.5);
			int fromY = (int) (y + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  *r++ = that(fromX,fromY);
			}
			else
			{
			  *r++ = zeroes;
			}
		  }
		}
	  }
	  return result;
	}
	else  // Any
	{
	  Image result (w, h, *image.format);

	  for (int toY = 0; toY < result.height; toY++)
	  {
		double x = tx += H01;
		double y = ty += H11;

		if (toY >= lo  &&  toY <= hi)
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			result.setRGBA (toX, toY, image.getRGBA ((int) (x + 0.5), (int) (y + 0.5)));
		  }
		}
		else
		{
		  for (int toX = 0; toX < result.width; toX++)
		  {
			x += H00;
			y += H10;
			int fromX = (int) (x + 0.5);
			int fromY = (int) (y + 0.5);
			if (fromX >= 0  &&  fromX <= iLastX1  &&  fromY >= 0  &&  fromY <= iLastY1)
			{
			  result.setRGBA (toX, toY, image.getRGBA (fromX, fromY));
			}
			else
			{
			  result.setRGBA (toX, toY, (uint32_t) 0);
			}
		  }
		}
	  }
	  return result;
	}
  }
}
