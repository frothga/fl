#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class Transform ------------------------------------------------------------

Transform::Transform (const MatrixAbstract<float> & A, bool inverse)
{
  if (inverse)
  {
	IA = A;
	this->A = !IA;
	needInverse = false;
  }
  else
  {
	this->A = A;
	needInverse = true;
  }

  defaultViewport = true;

  if (A.columns () >= 3)
  {
	peg = false;
	if (inverse)
	{
	  float scale = 1;
	  if (A.rows () >= 3)
	  {
		scale = A(2,2);
	  }
	  translateX = - (this->A(0,0) * A(0,2) + this->A(0,1) * A(1,2)) / scale;
	  translateY = - (this->A(1,0) * A(0,2) + this->A(1,1) * A(1,2)) / scale;
	}
	else
	{
	  translateX = A(0,2);
	  translateY = A(1,2);
	}
  }
  else
  {
	peg = true;
	translateX = 0;
	translateY = 0;
  }
}

Transform::Transform (const MatrixAbstract<double> & A, bool inverse)
{
  if (inverse)
  {
	IA <<= MatrixRegion<double> (A, 0, 0, 1, 1);
	this->A = !IA;
	needInverse = false;
  }
  else
  {
	this->A <<= MatrixRegion<double> (A, 0, 0, 1, 1);
	needInverse = true;
  }

  defaultViewport = true;

  if (A.columns () >= 3)
  {
	peg = false;
	if (inverse)
	{
	  double scale = 1;
	  if (A.rows () >= 3)
	  {
		scale = A(2,2);
	  }
	  translateX = - (this->A(0,0) * A(0,2) + this->A(0,1) * A(1,2)) / scale;
	  translateY = - (this->A(1,0) * A(0,2) + this->A(1,1) * A(1,2)) / scale;
	}
	else
	{
	  translateX = A(0,2);
	  translateY = A(1,2);
	}
  }
  else
  {
	peg = true;
	translateX = 0;
	translateY = 0;
  }
}

Transform::Transform (float angle)
{
  A (0, 0) = cos (angle);
  A (1, 0) = sin (angle);
  A (0, 1) = - A (1, 0);
  A (1, 1) = A (0, 0);

  needInverse = true;
  defaultViewport = true;
  peg = true;
  translateX = 0;
  translateY = 0;
}

Transform::Transform (float scaleX, float scaleY)
{
  A (0, 0) = scaleX;
  A (0, 1) = 0;
  A (1, 0) = 0;
  A (1, 1) = scaleY;

  needInverse = true;
  defaultViewport = true;
  peg = true;
  translateX = 0;
  translateY = 0;
}

Image
Transform::filter (const Image & image)
{
  if (needInverse)
  {
	IA = !A;
	needInverse = false;
  }

  Vector<float> cs (2);  // Center of source image
  Vector<float> cd (2);  // Center of destination image

  const float fImageWidth  = image.width - 0.5;
  const float fImageHeight = image.height - 0.5;
  const int   iImageWidth  = image.width - 1;
  const int   iImageHeight = image.height - 1;

  if (*image.format == GrayFloat)
  {
	ImageOf<float> result (GrayFloat);
	prepareResult (image, result, cs, cd);
	ImageOf<float> that (image);
	for (int toX = 0; toX < result.width; toX++)
	{
	  for (int toY = 0; toY < result.height; toY++)
	  {
		float x = toX - cd[0];
		float y = toY - cd[1];
		float tx =  IA (0, 0) * x + IA (0, 1) * y;
		y        = (IA (1, 0) * x + IA (1, 1) * y) + cs[1];
		x        = tx + cs[0];
		if (x > -0.5  &&  x < fImageWidth  &&  y > -0.5  &&  y < fImageHeight)
		{
		  int fromX = (int) floor (x);
		  int fromY = (int) floor (y);
		  float dx = x - fromX;
		  float dy = y - fromY;
		  float p00 = (fromX < 0             ||  fromY < 0            ) ? 0 : that (fromX,     fromY);
		  float p01 = (fromX < 0             ||  fromY >= iImageHeight) ? 0 : that (fromX,     fromY + 1);
		  float p10 = (fromX >= iImageWidth  ||  fromY < 0            ) ? 0 : that (fromX + 1, fromY);
		  float p11 = (fromX >= iImageWidth  ||  fromY >= iImageHeight) ? 0 : that (fromX + 1, fromY + 1);
		  result (toX, toY) =   (1.0 - dy) * ((1.0 - dx) * p00 + dx * p10)
			                  +  dy        * ((1.0 - dx) * p01 + dx * p11);
		}
	  }
	}
	return result;
  }
  else if (*image.format == GrayDouble)
  {
	ImageOf<double> result (GrayDouble);
	prepareResult (image, result, cs, cd);
	ImageOf<double> that (image);
	for (int toX = 0; toX < result.width; toX++)
	{
	  for (int toY = 0; toY < result.height; toY++)
	  {
		float x = toX - cd[0];
		float y = toY - cd[1];
		float tx =  IA (0, 0) * x + IA (0, 1) * y;
		y        = (IA (1, 0) * x + IA (1, 1) * y) + cs[1];
		x        = tx + cs[0];
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
		  result (toX, toY) =   (1.0 - dy) * ((1.0 - dx) * p00 + dx * p10)
			                  +  dy        * ((1.0 - dx) * p01 + dx * p11);
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
	prepareResult (image, result, cs, cd);
	Pixel zero;
	zero.setRGBA ((unsigned int) 0xFF000000);
	for (int toX = 0; toX < result.width; toX++)
	{
	  for (int toY = 0; toY < result.height; toY++)
	  {
		float x = toX - cd[0];
		float y = toY - cd[1];
		float tx =  IA (0, 0) * x + IA (0, 1) * y;
		y        = (IA (1, 0) * x + IA (1, 1) * y) + cs[1];
		x        = tx + cs[0];
		if (x > -0.5  &&  x < fImageWidth  &&  y > -0.5  &&  y < fImageHeight)
		{
		  int fromX = (int) floor (x);
		  int fromY = (int) floor (y);
		  float dx = x - fromX;
		  float dy = y - fromY;
		  Pixel p00 = (fromX < 0             ||  fromY < 0            ) ? zero : image (fromX,     fromY);
		  Pixel p01 = (fromX < 0             ||  fromY >= iImageHeight) ? zero : image (fromX,     fromY + 1);
		  Pixel p10 = (fromX >= iImageWidth  ||  fromY < 0            ) ? zero : image (fromX + 1, fromY);
		  Pixel p11 = (fromX >= iImageWidth  ||  fromY >= iImageHeight) ? zero : image (fromX + 1, fromY + 1);
		  result (toX, toY) =   (p00 * (1.0 - dx) + p10 * dx) * (1.0 - dy)
			                  + (p01 * (1.0 - dx) + p11 * dx) * dy;
		}
	  }
	}
	return result;
  }
}

void
Transform::setPeg (float originX, float originY, int width, int height)
{
  defaultViewport = false;
  peg = true;

  this->originX = originX;
  this->originY = originY;
  this->width   = width;
  this->height  = height;
}

void
Transform::setWindow (float originX, float originY, int width, int height)
{
  defaultViewport = false;
  peg = false;

  this->originX = originX;
  this->originY = originY;
  this->width   = width;
  this->height  = height;
}

Transform
Transform::operator * (const Transform & that) const
{
  Transform result (A * that.A);
  result.translateX = A (0, 0) * that.translateX + A (0, 1) * that.translateY + translateX;
  result.translateY = A (1, 0) * that.translateX + A (1, 1) * that.translateY + translateY;
  result.peg = result.translateX == 0  &&  result.translateY == 0;
  return result;
}

void
Transform::prepareResult (const Image & image, Image & result, Vector<float> & cs, Vector<float> & cd) const
{
  // Prepare image and various parameters
  if (peg)
  {
	if (defaultViewport)
	{
	  cs[0] = (image.width - 1) / 2.0;
	  cs[1] = (image.height - 1) / 2.0;

	  float l = cs[0];
	  float r = cs[0];
	  float t = cs[1];
	  float b = cs[1];

      #define twistCorner(inx,iny) \
	  { \
	    float outx = inx - cs[0]; \
	    float outy = iny - cs[1]; \
	    float tx =  A (0, 0) * outx + A (0, 1) * outy; \
	    outy     = (A (1, 0) * outx + A (1, 1) * outy) + cs[1]; \
	    outx     = tx + cs[0]; \
	    l = l <? outx; \
	    r = r >? outx; \
	    t = t <? outy; \
	    b = b >? outy; \
      }

	  twistCorner (-0.5,                -0.5);                  // Upper  left  corner
	  twistCorner ((image.width - 0.5), -0.5);                  // Upper  right corner
	  twistCorner (-0.5,                (image.height - 0.5)); // Bottom left  corner
	  twistCorner ((image.width - 0.5), (image.height - 0.5)); // Bottom right corner

	  cd[0] = cs[0] - l - 0.5;
	  cd[1] = cs[1] - t - 0.5;
	  result.resize ((int) ceil (r - l), (int) ceil (b - t));
	}
	else
	{
	  int w = width < 0 ? image.width : width;
	  int h = height < 0 ? image.height : height;
	  result.resize (w, h);
	  cd[0] = (w - 1) / 2.0;
	  cd[1] = (h - 1) / 2.0;
	  cs[0] = originX < 0 ? (image.width - 1)  / 2.0 : originX;
	  cs[1] = originY < 0 ? (image.height - 1) / 2.0 : originY;
	}
  }
  else
  {
	if (defaultViewport)
	{
	  cd[0] = translateX;
	  cd[1] = translateY;
	  cs[0] = 0;
	  cs[1] = 0;
	  result.resize (image.width, image.height);
	}
	else
	{
	  cd[0] = translateX - (originX - width  / 2.0);
	  cd[1] = translateY - (originY - height / 2.0);
	  cs[0] = 0;
	  cs[1] = 0;
	  result.resize (width, height);
	}
  }
}
