#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class FilterHarris ---------------------------------------------------------

const double FilterHarris::alpha = 0.06;

FilterHarris::FilterHarris (double sigmaD, double sigmaI, const PixelFormat & format)
: G_I (sigmaI, format),
  G1_I (sigmaI, format),
  G1_D (sigmaD, format),
  dG_D (sigmaD, format),
  xx (format),
  xy (format),
  yy (format)
{
  this->sigmaD = sigmaD;
  this->sigmaI = sigmaI;

  dG_D *= sigmaD;  // Boost to make results comparable across scale.

  offsetI = G1_I.width / 2;
  offsetD = (G1_D.width >? dG_D.width) / 2;
  offset = offsetI + offsetD;

  offset1 = 0;
  offset2 = 0;
  int difference = (G1_D.width - dG_D.width) / 2;
  if (difference > 0)
  {
	offset1 = difference;
  }
  else if (difference < 0)
  {
	offset2 = -difference;
  }
}

Image
FilterHarris::filter (const Image & image)
{
  preprocess (image);
  return process ();
}

void
FilterHarris::preprocess (const Image & image)
{
  if (*image.format != *G1_D.format)
  {
	preprocess (image * (*G1_D.format));
  }

  G1_D.direction = Vertical;
  dG_D.direction = Horizontal;
  Image dx = image * G1_D * dG_D;

  G1_D.direction = Horizontal;
  dG_D.direction = Vertical;
  Image dy = image * G1_D * dG_D;

  xx.resize (dx.width <? dy.width, dx.height <? dy.height);
  xy.resize (xx.width, xx.height);
  yy.resize (xx.width, xx.height);

  if (*dx.format == GrayFloat)
  {
	ImageOf<float> dxf (dx);
	ImageOf<float> dyf (dy);
	ImageOf<float> xxf (xx);
	ImageOf<float> xyf (xy);
	ImageOf<float> yyf (yy);
	for (int y = 0; y < xx.height; y++)
	{
	  for (int x = 0; x < xx.width; x++)
	  {
		float tx = dxf (x + offset1, y + offset2);
		float ty = dyf (x + offset2, y + offset1);
		xxf (x, y) = tx * tx;
		xyf (x, y) = tx * ty;
		yyf (x, y) = ty * ty;
	  }
	}
  }
  else if (*dx.format == GrayDouble)
  {
	ImageOf<double> dxd (dx);
	ImageOf<double> dyd (dy);
	ImageOf<double> xxd (xx);
	ImageOf<double> xyd (xy);
	ImageOf<double> yyd (yy);
	for (int y = 0; y < xx.height; y++)
	{
	  for (int x = 0; x < xx.width; x++)
	  {
		double tx = dxd (x + offset1, y + offset2);
		double ty = dyd (x + offset2, y + offset1);
		xxd (x, y) = tx * tx;
		xyd (x, y) = tx * ty;
		yyd (x, y) = ty * ty;
	  }
	}
  }
  else
  {
	throw "FilterHarris::preprocess: unimplemented format";
  }
}

Image
FilterHarris::process ()
{
  int last = G1_I.width - 1;

  G1_I.direction = Vertical;
  Image Sxx = xx * G1_I;
  Image Sxy = xy * G1_I;
  Image Syy = yy * G1_I;
  G1_I.direction = Horizontal;
  Sxx = Sxx * G1_I;
  Sxy = Sxy * G1_I;
  Syy = Syy * G1_I;

  if (*G1_I.format == GrayFloat)
  {
	ImageOf<float> result (xx.width - last, xx.height - last, GrayFloat);
	ImageOf<float> Sxxf (Sxx);
	ImageOf<float> Sxyf (Sxy);
	ImageOf<float> Syyf (Syy);
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		float txx = Sxxf (x, y);
		float txy = Sxyf (x, y);
		float tyy = Syyf (x, y);
		result (x, y) = (txx * tyy - txy * txy) - alpha * (txx + tyy) * (txx + tyy);
	  }
	}
	return result;
  }
  else if (*G1_I.format == GrayDouble)
  {
	ImageOf<double> result (xx.width - last, xx.height - last, GrayDouble);
	ImageOf<double> Sxxd (Sxx);
	ImageOf<double> Sxyd (Sxy);
	ImageOf<double> Syyd (Syy);
	for (int y = 0; y < result.height; y++)
	{
	  for (int x = 0; x < result.width; x++)
	  {
		double txx = Sxxd (x, y);
		double txy = Sxyd (x, y);
		double tyy = Syyd (x, y);
		result (x, y) = (txx * tyy - txy * txy) - alpha * (txx + tyy) * (txx + tyy);
	  }
	}
	return result;
  }
  else
  {
	throw "FilterHarris::process: unimplemented format";
  }
}

double
FilterHarris::response (const int x, const int y) const
{
  Point p (x + offsetI, y + offsetI);
  double txx = G_I.response (xx, p);
  double txy = G_I.response (xy, p);
  double tyy = G_I.response (yy, p);
  return (txx * tyy - txy * txy) - alpha * (txx + tyy) * (txx + tyy);
}
