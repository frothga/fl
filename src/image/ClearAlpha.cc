#include "fl/convolve.h"


// For debugging
#include <iostream>
using namespace std;


using namespace fl;


// class ClearAlpha -----------------------------------------------------------

ClearAlpha::ClearAlpha (unsigned int color)
{
  this->color = color;
}

Image
ClearAlpha::filter (const Image & image)
{
  Image result (image.width, image.height, *image.format);

  //float rc = ((color &   0xFF0000) >> 16) / 255.0f;
  //float gc = ((color &     0xFF00) >>  8) / 255.0f;
  //float bc =  (color &       0xFF)        / 255.0f;

  Pixel c;
  c.setRGBA (color);

  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  /*
	  unsigned int rgb = image.getRGBA (x, y);
	  float rf    = ((rgb &   0xFF0000) >> 16) / 255.0f;
	  float gf    = ((rgb &     0xFF00) >>  8) / 255.0f;
	  float bf    =  (rgb &       0xFF)        / 255.0f;
	  float alpha = ((rgb & 0xFF000000) >> 24) / 255.0f;

	  rf = rf * alpha + (1 - alpha) * rc;
	  gf = gf * alpha + (1 - alpha) * gc;
	  bf = bf * alpha + (1 - alpha) * bc;

	  unsigned int r = (unsigned int) (rf * 255);
	  unsigned int g = (unsigned int) (gf * 255);
	  unsigned int b = (unsigned int) (bf * 255);
	  result.setRGB (x, y, (r << 16) | (g << 8) | b);
	  */

	  result (x, y) = c << image (x, y);
	}
  }

  return result;
}
