#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class ConvolutionRecursive1D -----------------------------------------------

Image
ConvolutionRecursive1D::filter (const Image & image)
{
  if (*image.format != GrayDouble)
  {
	return filter (image * GrayDouble);
  }

  int w = image.width;
  int h = image.height;

  ImageOf<double> i (image);
  ImageOf<double> p (w, h, GrayDouble);  // "plus"
  ImageOf<double> m (w, h, GrayDouble);  // "minus"

  double np = n00p + n11p + n22p + n33p;
  double dp = d11p + d22p + d33p + d44p;
  double nm = n11m + n22m + n33m + n44m;
  double dm = d11m + d22m + d33m + d44m;
  double zp = np / (1 + dp);
  double zm = nm / (1 + dm);

  if (direction == Horizontal)
  {
	for (int y = 0; y < h; y++)
	{
	  p(0,y) = zp * i(0,y);
	  p(1,y) =   n00p * i(1,y) + n11p * i(0,y)
		       + n22p * i(0,y) + n33p * i(0,y)
		       - dp * p(0,y);
	  p(2,y) =   n00p * i(2,y) + n11p * i(1,y)
		       + n22p * i(0,y) + n33p * i(0,y)
		       - d11p * p(1,y) - d22p * p(0,y)
		       - d33p * p(0,y) - d44p * p(0,y);
	  p(3,y) =   n00p * i(3,y) + n11p * i(2,y)
		       + n22p * i(1,y) + n33p * i(0,y)
		       - d11p * p(2,y) - d22p * p(1,y)
		       - d33p * p(0,y) - d44p * p(0,y);

	  for (int x = 4; x < w; x++)
	  {
		p(x,y) =   n00p * i(x,  y) + n11p * i(x-1,y)
		         + n22p * i(x-2,y) + n33p * i(x-3,y)
		         - d11p * p(x-1,y) - d22p * p(x-2,y)
		         - d33p * p(x-3,y) - d44p * p(x-4,y);
	  }

	  m(w-1,y) = zm * i(w-1,y);
	  m(w-2,y) = nm * i(w-1,y) - dm * m(w-1,y);
	  m(w-3,y) =   n11m * i(w-2,y) + n22m * i(w-1,y)
		         + n33m * i(w-1,y) + n44m * i(w-1,y)
		         - d11m * m(w-2,y) - d22m * m(w-1,y)
		         - d33m * m(w-1,y) - d44m * m(w-1,y);
	  m(w-4,y) =   n11m * i(w-3,y) + n22m * i(w-2,y)
		         + n33m * i(w-1,y) + n44m * i(w-1,y)
		         - d11m * m(w-3,y) - d22m * m(w-2,y)
		         - d33m * m(w-1,y) - d44m * m(w-1,y);

	  for (int x = w-5; x >= 0; x--)
	  {
		m(x,y) =   n11m * i(x+1,y) + n22m * i(x+2,y)
		         + n33m * i(x+3,y) + n44m * i(x+4,y)
		         - d11m * m(x+1,y) - d22m * m(x+2,y)
		         - d33m * m(x+3,y) - d44m * m(x+4,y);
	  }
	}
  }
  else  // direction == Vertical
  {
	for (int x = 0; x < w; x++)
	{
	  p(x,0) = zp * i(x,0);
	  p(x,1) =   n00p * i(x,1) + n11p * i(x,0)
		       + n22p * i(x,0) + n33p * i(x,0)
		       - dp * p(x,0);
	  p(x,2) =   n00p * i(x,2) + n11p * i(x,1)
		       + n22p * i(x,0) + n33p * i(x,0)
		       - d11p * p(x,1) - d22p * p(x,0)
		       - d33p * p(x,0) - d44p * p(x,0);
	  p(x,3) =   n00p * i(x,3) + n11p * i(x,2)
		       + n22p * i(x,1) + n33p * i(x,0)
		       - d11p * p(x,2) - d22p * p(x,1)
		       - d33p * p(x,0) - d44p * p(x,0);

	  for (int y = 4; y < h; y++)
	  {
		p(x,y) =   n00p * i(x,y)   + n11p * i(x,y-1)
		         + n22p * i(x,y-2) + n33p * i(x,y-3)
		         - d11p * p(x,y-1) - d22p * p(x,y-2)
		         - d33p * p(x,y-3) - d44p * p(x,y-4);
	  }


	  m(x,h-1) = zm * i(x,h-1);
	  m(x,h-2) = nm * i(x,h-1) - dm * m(x,h-1);
	  m(x,h-3) =   n11m * i(x,h-2) + n22m * i(x,h-1)
		         + n33m * i(x,h-1) + n44m * i(x,h-1)
		         - d11m * m(x,h-2) - d22m * m(x,h-1)
		         - d33m * m(x,h-1) - d44m * m(x,h-1);
	  m(x,h-4) =   n11m * i(x,h-3) + n22m * i(x,h-2)
		         + n33m * i(x,h-1) + n44m * i(x,h-1)
		         - d11m * m(x,h-3) - d22m * m(x,h-2)
		         - d33m * m(x,h-1) - d44m * m(x,h-1);

	  for (int y = h-5; y >= 0; y--)
	  {
		m(x,y) =   n11m * i(x,y+1) + n22m * i(x,y+2)
		         + n33m * i(x,y+3) + n44m * i(x,y+4)
		         - d11m * m(x,y+1) - d22m * m(x,y+2)
		         - d33m * m(x,y+3) - d44m * m(x,y+4);
	  }
	}
  }

  p = p + m;
  p *= scale;

  return p;
}

double
ConvolutionRecursive1D::response (const Image & image, const Point & p) const
{
  if (*image.format != GrayDouble)
  {
	return response (image * GrayDouble, p);
  }

  // The value at a given point can't be calculated independently of all the
  // pixels above or to the left (depending on direction of filter).
  // Therefore, we must run the recursive filter along the row/column and
  // break at the requested pixel.

  // TODO: copy the convolution code from above and adapt to break at pixel.

  throw "ConvolutionRecursive1D::response not implemented yet";
}

void
ConvolutionRecursive1D::set_nii_and_dii (double sigma, double a0, double a1, double b0, double b1, double c0, double c1, double o0, double o1)
{
  double co0s = cos (o0 / sigma);
  double co1s = cos (o1 / sigma);
  double so0s = sin (o0 / sigma);
  double so1s = sin (o1 / sigma);

  n00p = a0 + c0;
  n11p =   exp (-b1 / sigma) * (c1 * so1s - (c0 + 2 * a0) * co1s)
	     + exp (-b0 / sigma) * (a1 * so0s - (2 * c0 + a0) * co0s);
  n22p =   2  * exp (-b0 /sigma - b1 / sigma) * ((a0 + c0) * co1s * co0s
												 - co1s * a1 * so0s
												 - co0s * c1 * so1s)
	     + c0 * exp (-2 * b0 / sigma) + a0 * exp (-2 * b1 / sigma);
  n33p =   exp (-b1 / sigma - 2 * b0 / sigma) * (c1 * so1s - c0 * co1s)
	     + exp (-b0 / sigma - 2 * b1 / sigma) * (a1 * so0s - a0 * co0s);
  d11p = -2 * exp (-b1 / sigma) * co1s
	     -2 * exp (-b0 / sigma) * co0s;
  d22p =   4 * co1s * co0s * exp (-b0 / sigma - b1 / sigma)
	     + exp (-2 * b1 / sigma) + exp (-2 * b0 / sigma);
  d33p = -2 * co0s * exp (-b0 / sigma - 2 * b1 / sigma)
	     -2 * co1s * exp (-b1 / sigma - 2 * b0 / sigma);
  d44p = exp(-2 * b0 / sigma - 2 * b1 / sigma);
}


// class GaussianRecursive1D --------------------------------------------------

GaussianRecursive1D::GaussianRecursive1D (double sigma, const Direction direction)
{
  double a0 =  1.68;
  double a1 =  3.735;
  double b0 =  1.783;
  double b1 =  1.723;
  double c0 = -0.6803;
  double c1 = -0.2598;
  double o0 =  0.6318;
  double o1 =  1.997;

  set_nii_and_dii (sigma, a0, a1, b0, b1, c0, c1, o0, o1);

  d11m = d11p;
  d22m = d22p;
  d33m = d33p;
  d44m = d44p;

  n11m = n11p - d11p * n00p;
  n22m = n22p - d22p * n00p;
  n33m = n33p - d33p * n00p;
  n44m =      - d44p * n00p;

  scale = 1 / (sqrt (2 * PI) * sigma);
}


// class GaussianDerivativeRecursive1D ----------------------------------------

GaussianDerivativeRecursive1D::GaussianDerivativeRecursive1D (double sigma, const Direction direction)
{
  double a0 = -0.6472;
  double a1 = -4.531;
  double b0 =  1.527;
  double b1 =  1.516;
  double c0 =  0.6494;
  double c1 =  0.9557;
  double o0 =  0.6719;
  double o1 =  2.072;

  set_nii_and_dii (sigma, a0, a1, b0, b1, c0, c1, o0, o1);

  d11m = d11p;
  d22m = d22p;
  d33m = d33p;
  d44m = d44p;
	
  n11m = -(n11p - d11p * n00p);
  n22m = -(n22p - d22p * n00p);
  n33m = -(n33p - d33p * n00p);
  n44m =          d44p * n00p;

  scale = -1 / (sqrt (2 * PI) * sigma * sigma);
}


// class GaussianDerivativeSecondRecursive1D ----------------------------------

GaussianDerivativeSecondRecursive1D::GaussianDerivativeSecondRecursive1D (double sigma, const Direction direction)
{
  double a0 = -1.331;
  double a1 =  3.661;
  double b0 =  1.24;
  double b1 =  1.314;
  double c0 =  0.3225;
  double c1 = -1.738;
  double o0 =  0.748;
  double o1 =  2.166;

  set_nii_and_dii (sigma, a0, a1, b0, b1, c0, c1, o0, o1);

  d11m = d11p;
  d22m = d22p;
  d33m = d33p;
  d44m = d44p;

  n11m = n11p - d11p * n00p;
  n22m = n22p - d22p * n00p;
  n33m = n33p - d33p * n00p;
  n44m =      - d44p * n00p;

  scale = 1 / (sqrt (2 * PI) * sigma * sigma * sigma);
}
