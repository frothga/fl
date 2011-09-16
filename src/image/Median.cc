/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"
#include "fl/MatrixFixed.tcc"


using namespace std;
using namespace fl;


template class MatrixFixed<uint16_t,16,1>;
template class MatrixFixed<uint16_t,16,16>;


// class Median ---------------------------------------------------------------

Median::Median (int radius, float order)
: radius (radius),
  order (order),
  cacheSize (0)
{
  if (radius < 1) throw "This filter requires a radius of at least 1.";
}

static inline int
shift (uint32_t mask)
{
  int result = 0;
  while (mask >>= 1) {result++;}
  return (result + 1) / 8 - 1;
}

Image
Median::filter (const Image & image)
{
  if (*image.format == GrayChar)
  {
	PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
	if (! imageBuffer) throw "Median can only handle packed buffers";

	Image result (image.width, image.height, *image.format);
	PixelBufferPacked * resultBuffer = (PixelBufferPacked *) result.buffer;

	split (image.width, image.height,
		   (uint8_t *) imageBuffer->memory,  1, imageBuffer->stride,
		   (uint8_t *) resultBuffer->memory, 1, resultBuffer->stride);
	return result;
  }

  const PixelFormatRGBABits * format = (const PixelFormatRGBABits *) image.format;
  if (    format
      &&  format->redBits == 8  &&  format->greenBits == 8  &&  format->blueBits == 8
      && (format->alphaBits == 8  ||  format->alphaBits == 0))
  {
	PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
	if (! imageBuffer) throw "Median can only handle packed buffers";

	Image result (image.width, image.height, *image.format);
	PixelBufferPacked * resultBuffer = (PixelBufferPacked *) result.buffer;

	uint8_t * in   = (uint8_t *) imageBuffer ->memory;
	uint8_t * out  = (uint8_t *) resultBuffer->memory;
	int inStrideV  =             imageBuffer ->stride;
	int outStrideV =             resultBuffer->stride;
	int strideH    = (int) roundp (format->depth);
	int redShift   = shift (format->redMask);
	int greenShift = shift (format->greenMask);
	int blueShift  = shift (format->blueMask);

	split (image.width, image.height,
		   in  + redShift, strideH, inStrideV,
		   out + redShift, strideH, outStrideV);

	split (image.width, image.height,
		   in  + greenShift, strideH, inStrideV,
		   out + greenShift, strideH, outStrideV);

	split (image.width, image.height,
		   in  + blueShift, strideH, inStrideV,
		   out + blueShift, strideH, outStrideV);

	// Should we do anything with alpha?

	return result;
  }

  if (image.format->monochrome) return filter (image * GrayChar);
  else                          return filter (image * RGBChar);
}

void
Median::split (int width, int height, uint8_t * inBuffer, int inStrideH, int inStrideV, uint8_t * outBuffer, int outStrideH, int outStrideV)
{
  if (cacheSize == 0)
  {
	filter (width, height, inBuffer, inStrideH, inStrideV, outBuffer, outStrideH, outStrideV);
  }
  else
  {
	// For now, just process whole thing.
	filter (width, height, inBuffer, inStrideH, inStrideV, outBuffer, outStrideH, outStrideV);
  }
}

/**
   @todo This version assumes both borders are processed, and in Boost mode.
   If split() is fully implemented, then need flags to change border handling
   for interior columns.
 **/
void
Median::filter (int width, int height, uint8_t * inBuffer, int inStrideH, int inStrideV, uint8_t * outBuffer, int outStrideH, int outStrideV)
{
  class Histogram
  {
  public:
	Histogram ()
	{
	  coarse.clear ();
	  fine.clear ();
	}

	inline void increment (const uint8_t & pixel)
	{
	  coarse[pixel >> 4]++;
	  fine  [pixel     ]++;
	}

	inline void decrement (const uint8_t & pixel)
	{
	  coarse[pixel >> 4]--;
	  fine  [pixel     ]--;
	}

	inline void operator += (const Histogram & that)
	{
	  coarse += that.coarse;
	  fine   += that.fine;
	}

	MatrixFixed<uint16_t,16,1>  coarse;
	MatrixFixed<uint16_t,16,16> fine;
  };

  Histogram * histograms = new Histogram[width];
  int heightRadius = min (radius, height);
  int widthRadius  = min (radius, width );

  // Prepare column histograms as if they are at row -1
  uint8_t * p    = inBuffer;
  uint8_t * end  = p + heightRadius * inStrideV;  // scan heightRadius number of rows
  int       step = inStrideV - width * inStrideH;
  while (p < end)
  {
	Histogram * h      = histograms;
	Histogram * rowEnd = h + width;  // scan width number of columns
	while (h < rowEnd)
	{
	  (h++)->increment (*p);
	  p += inStrideH;
	}
	p += step;
  }

  // Scan each row of image
  int countY = heightRadius;
  for (int y = 0; y < height; y++)
  {
	// Advance all column histograms one row
	int r = y - radius;
	if (r >= 0)
	{
	  uint8_t * p = inBuffer + r * inStrideV;
	  for (int x = 0; x < width; x++)
	  {
		histograms[x].decrement (*p);
		p += inStrideH;
	  }
	  countY--;
	}

	r = y + radius;
	if (r < height)
	{
	  uint8_t * p = inBuffer + r * inStrideV;
	  for (int x = 0; x < width; x++)
	  {
		histograms[x].increment (*p);
		p += inStrideH;
	  }
	  countY++;
	}

	// Scan across row, keeping running total in histogram
	Histogram total;
	int lastColumn[16];

	//   Prepare running total up to column -1
	for (int x = 0; x < widthRadius; x++) total += histograms[x];
	for (int i = 0; i < 16; i++) lastColumn[i] = -1;

	//   Scan columns
	int count = widthRadius * countY;
	for (int x = 0; x < width; x++)
	{
	  r = x - radius;
	  if (r >= 0)
	  {
		total.coarse -= histograms[r].coarse;
		count--;
	  }

	  r = x + radius;
	  if (r < width)
	  {
		total.coarse += histograms[r].coarse;
		count++;
	  }

	  // Find coarse level
	  int threshold = (int) (order * count);
	  int sum = 0;
	  int c;
	  for (c = 0; c < 16; c++)
	  {
		sum += total.coarse[c];
		if (sum > threshold) break;
	  }

	  // Update associated 2nd level histogram
	  MatrixResult<uint16_t> fineColumn = total.fine.column (c);
	  r = x - radius;
	  if (lastColumn[c] >= r)
	  {
		for (int w = lastColumn[c] + 1; w <= x; w++)
		{
		  r = w - radius;
		  if (r >= 0) fineColumn -= histograms[r].fine.column (c);

		  r = w + radius;
		  if (r < width) fineColumn += histograms[r].fine.column (c);
		}
	  }
	  else
	  {
		fineColumn.clear ();
		int lo = max (x - radius, 0);
		int hi = min (x + radius, width - 1);
		for (int w = lo; w <= hi; w++) fineColumn += histograms[w].fine.column (c);
	  }
	  lastColumn[c] = x;

	  // Find fine level
	  sum = 0;
	  int f = 0;
	  for (f = 0; f < 16; f++)
	  {
		sum += fineColumn[f];
		if (sum > threshold) break;
	  }
	  outBuffer[y * outStrideV + x * outStrideH] = c * 16 + f;
	}
  }

  delete [] histograms;
}
