#include "fl/convolve.h"
#include "fl/pi.h"

#include <float.h>


using namespace std;
using namespace fl;


// class TransformGauss ------------------------------------------------------------

void
TransformGauss::prepareG ()
{
  // Calculate size and shape of Gaussian
  const float sigma2 = 0.25;  // radius of support squared; ie: 0.5 * 0.5, since we want half a pixel in all directions
  Matrix2x2<float> S = IA * ~IA * sigma2;
  sigmaX = sqrt (S (0, 0));
  sigmaY = sqrt (S (1, 1));
  float C = 2.0 * PI * sigmaX * sigmaY;
  Gshw = (int) ceil (sigmaX * 3);
  Gshh = (int) ceil (sigmaY * 3);
  S = !S;

  // Generate discretized Gaussian
  const float stepsPerZ = 6;
  GstepX = (int) ceil (stepsPerZ / sigmaX) >? 1;  // (steps / pixel) = (steps / Z) / (pixels / Z)
  GstepY = (int) ceil (stepsPerZ / sigmaY) >? 1;
  G.resize ((2 * Gshw + 1) * GstepX,
			(2 * Gshh + 1) * GstepY);
  int hw = G.width / 2;
  int hh = G.height / 2;
  for (int y = 0; y < G.height; y++)
  {
	for (int x = 0; x < G.width; x++)
	{
	  float dx = (float) (x - hw) / GstepX;
	  float dy = (float) (y - hh) / GstepY;
	  float tx = S (0, 0) * dx + S (0, 1) * dy;
	  float ty = S (1, 0) * dx + S (1, 1) * dy;
	  float value = (1 / C) * exp (-0.5 * (dx * tx + dy * ty));
	  if (value < FLT_MIN  &&  dx >= -0.5  &&  dx < 0.5  &&  dy >= -0.5  &&  dy < 0.5)
	  {
		value = FLT_MIN;
	  }
	  G (x, y) = value;
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

  Matrix3x3<float> H;  // homography from destination image to source image

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
		if (x > -0.5 - sigmaX  &&  x < image.width - 0.5 + sigmaX  &&  y > -0.5 - sigmaY  &&  y < image.height - 0.5 + sigmaY)
		{
		  int rx = (int) rint (x);
		  int ry = (int) rint (y);
		  int beginX = rx - Gshw;
		  int beginY = ry - Gshh;
		  int endX = beginX + 2 * Gshw;
		  int endY = beginY + 2 * Gshh;
		  endX = endX <? (image.width - 1);
		  endY = endY <? (image.height - 1);
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
		if (x > -0.5 - sigmaX  &&  x < image.width - 0.5 + sigmaX  &&  y > -0.5 - sigmaY  &&  y < image.height - 0.5 + sigmaY)
		{
		  int rx = (int) rint (x);
		  int ry = (int) rint (y);
		  int beginX = rx - Gshw;
		  int beginY = ry - Gshh;
		  int endX = beginX + 2 * Gshw;
		  int endY = beginY + 2 * Gshh;
		  endX = endX <? (image.width - 1);
		  endY = endY <? (image.height - 1);
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
	Image result (*image.format);
	prepareResult (image, result, H);
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
		  int endX = beginX + 2 * Gshw;
		  int endY = beginY + 2 * Gshh;
		  endX = endX <? (image.width - 1);
		  endY = endY <? (image.height - 1);
		  float weight = 0;
		  Pixel sum (0);
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
			  sum += image (fromX, fromY) * w;
			  Gy += GstepY;
			}
			Gx += GstepX;
		  }
		  result (toX, toY) = sum / weight;
		}
	  }
	}
	return result;
  }
}
