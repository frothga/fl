/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Laplacian ------------------------------------------------------------

Laplacian::Laplacian (double sigma, const BorderMode mode, const PixelFormat & format)
: ConvolutionDiscrete2D (mode, format)
{
  // This constructs a Laplacian that is strictly circular.  It would not be
  // hard to make a version that separates sigmaX and sigmaY

  this->sigma = sigma;

  const int half = (int) roundp (Gaussian2D::cutoff * sigma);
  const int kernelSize = 2 * half + 1;
  const int sampleRatio = 2;
  const int samplesSize = (half + 1) * sampleRatio + 1;

  // Oversample each pixel in one quadrant
  Matrix<double> samples (samplesSize, samplesSize);
  double sigma2 = sigma * sigma;
  double sigma4 = sigma2 * sigma2;
  double C = 1.0 / (TWOPI * sigma2);
  for (int c = 0; c < samplesSize; c++)
  {
	double x = (double) c / sampleRatio - 0.5;
	double x2 = x * x;
	for (int r = 0; r < samplesSize; r++)
	{
	  double y = (double) r / sampleRatio - 0.5;
	  double y2 = y * y;
	  samples(r,c) = C * exp (- (x2 + y2) / (2 * sigma2)) * ((x2 + y2) / sigma4 - 2 / sigma2);
	}
  }

  // Average the samples into pixels for one quadrant
  ImageOf<double> temp (kernelSize, kernelSize, GrayDouble);
  for (int x = 0; x <= half; x++)
  {
	int c = sampleRatio * (half - x) + 1;
	for (int y = 0; y <= half; y++)
	{
	  int r = sampleRatio * (half - y) + 1;
	  temp(x,y) = (    samples(r-1,c-1) + 2 * samples(r-1,c) +     samples(r-1,c+1) +
				   2 * samples(r  ,c-1) + 4 * samples(r  ,c) + 2 * samples(r  ,c+1) +
					   samples(r+1,c-1) + 2 * samples(r+1,c) +     samples(r+1,c+1)) / 16;
	}
  }

  // Copy the quadrant to the other three
  for (int x = half + 1; x < kernelSize; x++)
  {
	int fromX = kernelSize - 1 - x;
	for (int y = 0; y <= half; y++)
	{
	  temp(x,y) = temp(fromX,y);
	}
  }
  for (int y = half + 1; y < kernelSize; y++)
  {
	int fromY = kernelSize - 1 - y;
	for (int x = 0; x < kernelSize; x++)
	{
	  temp(x,y) = temp(x,fromY);
	}
  }

  *this <<= temp * format;
  normalFloats ();
}

void
Laplacian::serialize (Archive & archive, uint32_t version)
{
  archive & *((ConvolutionDiscrete2D *) this);
  archive & sigma;
}
