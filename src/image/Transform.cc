#include "fl/convolve.h"
#include "fl/pi.h"
#include "fl/lapackd.h"


using namespace std;
using namespace fl;


// class Transform ------------------------------------------------------------

Transform::Transform (const Matrix<double> & A, bool inverse)
{
  initialize (A, inverse);
}

Transform::Transform (const Matrix<double> & A, const double scale)
{
  Matrix<double> temp (3, 3);
  temp.identity ();
  int r = min (2, A.rows () - 1);
  int c = min (2, A.columns () - 1);
  temp.region (0, 0) = A.region (0, 0, r, c);
  temp.region (0, 0, 1, 1) /= scale;

  initialize (temp, true);
}

Transform::Transform (double angle)
{
  Matrix2x2<double> temp;
  temp(0,0) = cos (angle);
  temp(1,0) = sin (angle);
  temp(0,1) = -temp(1,0);
  temp(1,1) = temp(0,0);

  initialize (temp, false);
}

Transform::Transform (double scaleX, double scaleY)
{
  Matrix2x2<double> temp;
  temp(0,0) = scaleX;
  temp(0,1) = 0;
  temp(1,0) = 0;
  temp(1,1) = scaleY;

  initialize (temp, false);
}

/**
   \todo Store full homography rather than separating into translation
   and deformation components.  The only thing preventing this before was
   some programs depending on a hack for scaling.  No program does this
   any more.
 **/
void
Transform::initialize (const Matrix<double> & A, bool inverse)
{
  if (inverse)
  {
	IA = MatrixRegion<double> (A, 0, 0, 1, 1);
  }
  else
  {
	// Use Matrix2x2's inversion method.
	IA = ! Matrix2x2<double> (MatrixRegion<double> (A, 0, 0, 1, 1));
  }

  defaultViewport = true;

  if (A.columns () >= 3)
  {
	if (inverse)
	{
	  // "scale" is a special hack to accomodate "S" matrices that have been
	  // scaled as a whole.  These matrices have the form
	  //   [ R  T ]
	  //   [ 0  1 ]
	  // before scaling.  Afterward, they have the form
	  //   [ R/s T/s ]
	  //   [ 0   1/s ]
	  // However, the intention is that only R is scaled, so T must be
	  // unscaled.
	  double scale = 1.0;
	  if (A.rows () >= 3)
	  {
		scale = A(2,2);
	  }

	  translateX = A(0,2) / scale;
	  translateY = A(1,2) / scale;
	}
	else
	{
	  translateX = - (IA(0,0) * A(0,2) + IA(0,1) * A(1,2));
	  translateY = - (IA(1,0) * A(0,2) + IA(1,1) * A(1,2));
	}
  }
  else
  {
	translateX = 0;
	translateY = 0;
  }
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
  Transform result (that.IA * IA, true);  // same as !(A * that.A)
  result.translateX = that.IA(0,0) * translateX + that.IA(0,1) * translateY + that.translateX;
  result.translateY = that.IA(1,0) * translateX + that.IA(1,1) * translateY + that.translateY;
  return result;
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

	// Remove this code and just use A
	Matrix2x2<float> A = !IA;
	float itX = -(A(0,0) * translateX + A(0,1) * translateY);
	float itY = -(A(1,0) * translateX + A(1,1) * translateY);

    #define twistCorner(inx,iny) \
	{ \
	  float outx = A(0,0) * inx + A(0,1) * iny + itX; \
	  float outy = A(1,0) * inx + A(1,1) * iny + itY; \
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
	centerX = (l + r) / 2.0;
	centerY = (t + b) / 2.0;
	width = (int) ceil (r - l);
	height = (int) ceil (b - t);
  }

  int w = width < 0 ? image.width : width;
  int h = height < 0 ? image.height : height;
  result.resize (w, h);

  Vector<float> cd (2);
  if (peg)
  {
	// Just use A in this section

	// Use cd as temporary storage for source image center.
	cd[0] = centerX < 0 ? (image.width - 1)  / 2.0 : centerX;
	cd[1] = centerY < 0 ? (image.height - 1) / 2.0 : centerY;

	// Transform center of source image into a point in virtual destination image.
	Matrix2x2<float> A = !IA;
	float itX = -(A(0,0) * translateX + A(0,1) * translateY);
	float itY = -(A(1,0) * translateX + A(1,1) * translateY);

	float tx = A(0,0) * cd[0] + A(0,1) * cd[1] + itX;
	cd[1]    = A(1,0) * cd[0] + A(1,1) * cd[1] + itY;
	cd[0] = tx;
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
  C.region (0, 0, 1, 1) = IA;
  C.region (0, 2, 1, 2) = IA * cd;
  C(0,2) += translateX;
  C(1,2) += translateY;
  C(2,0) = 0;
  C(2,1) = 0;
  C(2,2) = 1;
}
