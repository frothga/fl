/*
Author: Fred Rothganger
Created 12/22/2011


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class DoubleSize -----------------------------------------------------------

DoubleSize::DoubleSize (bool oddWidth, bool oddHeight)
: oddWidth (oddWidth),
  oddHeight (oddHeight)
{
}

Image
DoubleSize::filter (const Image & image)
{
  const int width  = image.width  * 2 + (oddWidth  ? 1 : 0);
  const int height = image.height * 2 + (oddHeight ? 1 : 0);

  Image source = image * GrayFloat;  // This filter only does GrayFloat

  Image result (width, height, GrayFloat);
  float * resultBuffer = (float *) ((PixelBufferPacked *) result.buffer)->memory;

  // Double the main body of the source
  float * p00 = (float *) ((PixelBufferPacked *) source.buffer)->memory;
  float * p10 = p00 + 1;
  float * p01 = p00 + source.width;
  float * p11 = p01 + 1;

  float * q00 = (float *) resultBuffer;
  float * q10 = q00 + 1;
  float * q01 = q00 + result.width;
  float * q11 = q01 + 1;
  int lastStep = width + 2 + width % 2;  // to step over odd pixel at end, if it is there

  float * end = p00 + source.width * source.height;
  while (p11 < end)
  {
	float * rowEnd = p00 + source.width;
	while (p10 < rowEnd)
	{
	  *q00 =  *p00;
	  *q10 = (*p00 + *p10) / 2.0f;
	  *q01 = (*p00 + *p01) / 2.0f;
	  *q11 = (*p00 + *p10 + *p01 + *p11) / 4.0f;

	  p00 = p10++;
	  p01 = p11++;

	  q00 += 2;
	  q10 += 2;
	  q01 += 2;
	  q11 += 2;
	}
	*q00 = *q10 =  *p00;
	*q01 = *q11 = (*p00 + *p01) / 2.0f;

	q00 += lastStep;
	q10 = q00 + 1;
	q01 += lastStep;
	q11 = q01 + 1;

	p00 = p10++;
	p01 = p11++;
  }

  // Fill in bottom
  if (oddHeight)  // odd: copy two rows of pixels
  {
	float * q02 = q01 + result.width;
	float * q12 = q02 + 1;
	float * rowEnd = p00 + source.width;
	while (p10 < rowEnd)
	{
	  *q00 = *q01 = *q02 =  *p00;
	  *q10 = *q11 = *q12 = (*p00 + *p10) / 2.0f;

	  p00 = p10++;

	  q00 += 2;
	  q10 += 2;
	  q01 += 2;
	  q11 += 2;
	  q02 += 2;
	  q12 += 2;
	}
	*q00 = *q10 = *q01 = *q11 = *q02 = *q12 = *p00;
  }
  else  // even: copy one row of pixels
  {
	float * rowEnd = p00 + source.width;
	while (p10 < rowEnd)
	{
	  *q00 = *q01 =  *p00;
	  *q10 = *q11 = (*p00 + *p10) / 2.0f;

	  p00 = p10++;

	  q00 += 2;
	  q10 += 2;
	  q01 += 2;
	  q11 += 2;
	}
	*q00 = *q10 = *q01 = *q11 = *p00;
  }

  // Fill in right side
  if (oddWidth)  // odd: copy an extra column of pixels
  {
	float * q1 = (float *) resultBuffer;
	float * end = q1 + result.width * result.height;
	q1 += (result.width - 1);
	float * q0 = q1 - 1;
	while (q1 < end)
	{
	  *q1 = *q0;
	  q0 += width;
	  q1 += width;
	}
  }

  return result;
}
