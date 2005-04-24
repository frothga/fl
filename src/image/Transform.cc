/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/convolve.h"
#include "fl/lapackd.h"


using namespace std;
using namespace fl;


// class Transform ------------------------------------------------------------

Transform::Transform (const Matrix<double> & A, bool inverse)
{
  initialize (A, inverse);
}

Transform::Transform (const Matrix<double> & IA, const double scale)
{
  Matrix<double> temp (3, 3);
  temp.identity ();
  int r = min (2, IA.rows () - 1);
  int c = min (2, IA.columns () - 1);
  temp.region (0, 0) = IA.region (0, 0, r, c);
  temp.region (0, 0, 2, 1) /= scale;

  initialize (temp, true);
}

Transform::Transform (double angle)
{
  Matrix<double> temp (2, 2);
  temp(0,0) = cos (angle);
  temp(1,0) = sin (angle);
  temp(0,1) = -temp(1,0);
  temp(1,1) = temp(0,0);

  initialize (temp, false);
}

Transform::Transform (double scaleX, double scaleY)
{
  Matrix<double> temp (2, 2);
  temp(0,0) = scaleX;
  temp(0,1) = 0;
  temp(1,0) = 0;
  temp(1,1) = scaleY;

  initialize (temp, false);
}

void
Transform::initialize (const Matrix<double> & A, bool inverse)
{
  Matrix3x3<double> temp;
  temp.identity ();
  int r = min (2, A.rows () - 1);
  int c = min (2, A.columns () - 1);
  temp.region (0, 0) = A.region (0, 0, r, c);
  temp /= temp(2,2);

  this->inverse = inverse;
  if (inverse)
  {
	IA      =  temp;
	this->A = !temp;
	this->A /= this->A(2,2);
  }
  else
  {
	this->A =  temp;
	IA      = !temp;
	IA /= IA(2,2);
  }

  defaultViewport = true;
}

Image
Transform::filter (const Image & image)
{
  Matrix3x3<float> H;  // homography from destination image to source image

  const float fImageWidth  = image.width - 0.5;
  const float fImageHeight = image.height - 0.5;
  const int   iImageWidth  = image.width - 1;
  const int   iImageHeight = image.height - 1;

  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (GrayFloat);
	prepareResult (image, result, H);
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
		if (x > -0.5  &&  x < fImageWidth  &&  y > -0.5  &&  y < fImageHeight)
		{
		  int fromX = (int) floorf (x);
		  int fromY = (int) floorf (y);
		  float dx = x - fromX;
		  float dy = y - fromY;
		  float p00 = (fromX < 0             ||  fromY < 0            ) ? 0 : that (fromX,     fromY);
		  float p01 = (fromX < 0             ||  fromY >= iImageHeight) ? 0 : that (fromX,     fromY + 1);
		  float p10 = (fromX >= iImageWidth  ||  fromY < 0            ) ? 0 : that (fromX + 1, fromY);
		  float p11 = (fromX >= iImageWidth  ||  fromY >= iImageHeight) ? 0 : that (fromX + 1, fromY + 1);
		  float dx1 = 1.0f - dx;
		  result (toX, toY) =   (1.0f - dy) * (dx1 * p00 + dx * p10)
			                  +  dy         * (dx1 * p01 + dx * p11);
		}
	  }
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> result (GrayDouble);
	prepareResult (image, result, H);
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
		if (x > -0.5  &&  x < fImageWidth  &&  y > -0.5  &&  y < fImageHeight)
		{
		  int fromX = (int) floor (x);
		  int fromY = (int) floor (y);
		  double dx = x - fromX;
		  double dy = y - fromY;
		  double p00 = (fromX < 0             ||  fromY < 0            ) ? 0 : that (fromX,     fromY);
		  double p01 = (fromX < 0             ||  fromY >= iImageHeight) ? 0 : that (fromX,     fromY + 1);
		  double p10 = (fromX >= iImageWidth  ||  fromY < 0            ) ? 0 : that (fromX + 1, fromY);
		  double p11 = (fromX >= iImageWidth  ||  fromY >= iImageHeight) ? 0 : that (fromX + 1, fromY + 1);
		  double dx1 = 1.0 - dx;
		  result (toX, toY) =   (1.0 - dy) * (dx1 * p00 + dx * p10)
			                  +  dy        * (dx1 * p01 + dx * p11);
		}
	  }
	}
	return result;
  }
  else if (*image.format == GrayChar)
  {
	return filter (image * GrayFloat);
  }
  else  // Any other format
  {
	Image result (*image.format);
	prepareResult (image, result, H);
	Pixel zero;
	zero.setRGBA ((unsigned int) 0xFF000000);
	for (int toY = 0; toY < result.height; toY++)
	{
	  for (int toX = 0; toX < result.width; toX++)
	  {
		float x = H(0,0) * toX + H(0,1) * toY + H(0,2);
		float y = H(1,0) * toX + H(1,1) * toY + H(1,2);
		float z = H(2,0) * toX + H(2,1) * toY + H(2,2);
		x /= z;
		y /= z;
		if (x > -0.5  &&  x < fImageWidth  &&  y > -0.5  &&  y < fImageHeight)
		{
		  int fromX = (int) floorf (x);
		  int fromY = (int) floorf (y);
		  float dx = x - fromX;
		  float dy = y - fromY;
		  Pixel p00 = (fromX < 0             ||  fromY < 0            ) ? zero : image (fromX,     fromY);
		  Pixel p01 = (fromX < 0             ||  fromY >= iImageHeight) ? zero : image (fromX,     fromY + 1);
		  Pixel p10 = (fromX >= iImageWidth  ||  fromY < 0            ) ? zero : image (fromX + 1, fromY);
		  Pixel p11 = (fromX >= iImageWidth  ||  fromY >= iImageHeight) ? zero : image (fromX + 1, fromY + 1);
		  float dx1 = 1.0f - dx;
		  result (toX, toY) =   (p00 * dx1 + p10 * dx) * (1.0f - dy)
			                  + (p01 * dx1 + p11 * dx) * dy;
		}
	  }
	}
	return result;
  }

  /* TODO: Replace the last two cases above with the following code, and test thoroughly.  Best I recollect, it screwed up at large upsampling ratios, producing gaps in the output between pixels.
  else if (*image.format == RGBAFloat)
  {
	// This method is meant to be fast, so it avoids a lot of the library
	// machinery that would otherwise make this simple to express.

	ImageOf<float[4]> result (RGBAFloat);
	prepareResult (image, result, H);
	ImageOf<float[4]> that (image);

	float zero[] = {0, 0, 0, 0};
	float * p00;
	float * p01;
	float * p10;
	float * p11;
	float * r = result (0, 0);

	for (int toY = 0; toY < result.height; toY++)
	{
	  for (int toX = 0; toX < result.width; toX++)
	  {
		float x = H(0,0) * toX + H(0,1) * toY + H(0,2);
		float y = H(1,0) * toX + H(1,1) * toY + H(1,2);
		float z = H(2,0) * toX + H(2,1) * toY + H(2,2);
		x /= z;
		y /= z;
		if (x > -0.5  &&  x < fImageWidth  &&  y > -0.5  &&  y < fImageHeight)
		{
		  int fromX = (int) floorf (x);
		  int fromY = (int) floorf (y);
		  float dx = x - fromX;
		  float dy = y - fromY;
		  p00 = (fromX < 0             ||  fromY < 0            ) ? zero : that (fromX,     fromY);
		  p01 = (fromX < 0             ||  fromY >= iImageHeight) ? zero : that (fromX,     fromY + 1);
		  p10 = (fromX >= iImageWidth  ||  fromY < 0            ) ? zero : that (fromX + 1, fromY);
		  p11 = (fromX >= iImageWidth  ||  fromY >= iImageHeight) ? zero : that (fromX + 1, fromY + 1);
		  float dx1 = 1.0f - dx;
		  float dy1 = 1.0f - dy;
		  *r++ =   (*p00++ * dx1 + *p10++ * dx) * dy1
			     + (*p01++ * dx1 + *p11++ * dx) * dy;
		  *r++ =   (*p00++ * dx1 + *p10++ * dx) * dy1
			     + (*p01++ * dx1 + *p11++ * dx) * dy;
		  *r++ =   (*p00++ * dx1 + *p10++ * dx) * dy1
			     + (*p01++ * dx1 + *p11++ * dx) * dy;
		  *r++ =   (*p00   * dx1 + *p10   * dx) * dy1   // Don't advance p## pointers on last step, because uneeded.  However, r must advance.
			     + (*p01   * dx1 + *p11   * dx) * dy;
		}
	  }
	}
	return result;
  }
  else  // all other formats
  {
	return filter (image * RGBAFloat) * (*image.format);
  }
  */
}

/**
   Set up viewport (of resulting image) so its center hits at a specified
   point in source image.
   \param centerX If NAN, then use center of original image.
   \param centerY If NAN, then use center of original image.
   \param width   If <= 0, then use width of original image.
   \param height  If <= 0, then use width of original image.
**/
void
Transform::setPeg (float centerX, float centerY, int width, int height)
{
  peg = true;
  defaultViewport = false;

  this->centerX = centerX;
  this->centerY = centerY;
  this->width   = width;
  this->height  = height;
}

void
Transform::setWindow (float centerX, float centerY, int width, int height)
{
  peg = false;
  defaultViewport = false;

  this->centerX = centerX;
  this->centerY = centerY;
  this->width   = width;
  this->height  = height;
}

Transform
Transform::operator * (const Transform & that) const
{
  if (! inverse  &&  ! that.inverse)
  {
	return Transform (A * that.A, false);
  }
  else
  {
	return Transform (that.IA * IA, true);
  }
}

void
Transform::prepareResult (const Image & image, Image & result, Matrix3x3<float> & C)
{
  // Prepare image and various parameters

  if (defaultViewport)
  {
	float l = INFINITY;
	float r = -INFINITY;
	float t = INFINITY;
	float b = -INFINITY;

    #define twistCorner(inx,iny) \
	{ \
	  float outz =  A(2,0) * inx + A(2,1) * iny + A(2,2); \
	  if (outz <= 0.0f) throw "Negative scale factor.  Image too large or homography too distorting."; \
	  float outx = (A(0,0) * inx + A(0,1) * iny + A(0,2)) / outz; \
	  float outy = (A(1,0) * inx + A(1,1) * iny + A(1,2)) / outz; \
	  l = min (l, outx); \
	  r = max (r, outx); \
	  t = min (t, outy); \
	  b = max (b, outy); \
    }

	twistCorner (-0.5,                -0.5);                  // Upper  left  corner
	twistCorner ((image.width - 0.5), -0.5);                  // Upper  right corner
	twistCorner (-0.5,                (image.height - 0.5)); // Bottom left  corner
	twistCorner ((image.width - 0.5), (image.height - 0.5)); // Bottom right corner

	peg = false;
	centerX = (l + r) / 2.0f;
	centerY = (t + b) / 2.0f;
	width = (int) ceil (r - l);
	height = (int) ceil (b - t);
  }

  int w = width  <= 0 ? image.width  : width;
  int h = height <= 0 ? image.height : height;
  result.resize (w, h);

  Vector<double> cd (3);
  cd[2] = 1.0f;
  if (peg)
  {
	// Use cd as temporary storage for source image center.
	cd[0] = isnan (centerX) ? (image.width - 1)  / 2.0 : centerX;
	cd[1] = isnan (centerY) ? (image.height - 1) / 2.0 : centerY;

	// Transform center of source image into a point in virtual destination image.
	cd = A * cd;
	cd /= cd[2];
  }
  else
  {
	cd[0] = centerX;
	cd[1] = centerY;
  }

  // Combine center of real destination image with virtual destination point.
  cd[0] -= (result.width  - 1) / 2.0;
  cd[1] -= (result.height - 1) / 2.0;

  // Use cd to construct C
  C = IA;  // currently different types, so deep copy.  Should use copyFrom() for final version
  C.region (0, 2, 2, 2) = IA * cd;
}
