#include "fl/convolve.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class DifferenceOfGaussians ------------------------------------------------

DifferenceOfGaussians::DifferenceOfGaussians (double sigmaPlus, double sigmaMinus, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (format, mode)
{
  Gaussian2D plus (sigmaPlus, mode, GrayDouble);
  Gaussian2D minus (sigmaMinus, mode, GrayDouble);
  Image temp = plus - minus;

  if (format == GrayChar)
  {
	ImageOf<unsigned char> tempC = temp * GrayChar;

	int top    = tempC.height - 1;
	int bottom = 0;
	int left   = tempC.width - 1;
	int right  = 0;

	for (int x = 0; x < width; x++)
	{
	  for (int y = 0; y < height; y++)
	  {
		if (tempC (x, y) != 128)  // 128 is the standard bias for char images.
		{
		  top    = min (top,    y);
		  bottom = max (bottom, y);
		  left   = min (left,   x);
		  right  = max (right,  x);
		}
	  }
	}

	int w = right - left + 1;
	int h = bottom - top + 1;
	bitblt (tempC, 0, 0, left, top, w, h);
  }
  else
  {
	*this <<= temp * format;
	normalFloats ();
  }
}
