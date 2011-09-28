/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/convolve.h"


using namespace std;
using namespace fl;


// class Median ---------------------------------------------------------------

Median::Median (int radius, float order)
: radius (radius),
  order (order),
  cacheSize (0)
{
  if (radius < 1  ) throw "This filter requires a radius of at least 1.";
  if (radius > 127) cerr << "WARNING: Window size exceeds numeric capacity." << endl; 
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
  const int histogramBytes   = sizeof (uint16_t) * (16 + 256);
  const int extraBookkeeping = sizeof (int) * 16 + 1024;
  int maxHistograms = (cacheSize - extraBookkeeping) / histogramBytes;

  if (maxHistograms < 3 * radius) // 2 * radius, plus some amount of progress on each stripe
  {
	filter (width, height, 0, width - 1, inBuffer, inStrideH, inStrideV, outBuffer, outStrideH, outStrideV);
  }
  else
  {
	int left = 0;
	while (left < width)
	{
	  int begin = max (0, left - radius);
	  int end   = begin + maxHistograms;
	  int right;
	  if (end < width)  // need seam between stripes
	  {
		right = end - radius - 1;
	  }
	  else  // end >= width, so no more stipes follow
	  {
		end = width;
		right = width - 1;
	  }
	  filter (end - begin, height, left - begin, right - begin,
			  inBuffer  + begin * inStrideH,  inStrideH,  inStrideV,
			  outBuffer + begin * outStrideH, outStrideH, outStrideV);
	  left = right + 1;
	}
  }
}

void
Median::filter (int width, int height, int left, int right, uint8_t * inBuffer, int inStrideH, int inStrideV, uint8_t * outBuffer, int outStrideH, int outStrideV)
{
  class Histogram
  {
  public:
	Histogram ()
	{
	  uint64_t * p = (uint64_t *) coarse;
	  p[0] = 0;
	  p[1] = 0;
	  p[2] = 0;
	  p[3] = 0;

	  p = (uint64_t *) fine;
	  uint64_t * end = (uint64_t *) (fine + 256);
	  while (p < end) *p++ = 0;
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

	inline void operator += (Histogram & that)
	{
	  // The trick here, and in all other similar functions in this class,
	  // is to add four numbers at the same time.  As long as the arithmetic
	  // does not overflow, adding a pair of unsigned 64-bit integers has
	  // exactly the same effect as adding four pairs of unsigned 16-bit
	  // integers individually.
	  uint64_t * from = (uint64_t *) that.coarse;
	  uint64_t * to   = (uint64_t *) coarse;
	  to[0] += from[0];
	  to[1] += from[1];
	  to[2] += from[2];
	  to[3] += from[3];

	  from            = (uint64_t *) that.fine;
	  to              = (uint64_t *) fine;
	  uint64_t * end  = (uint64_t *) (fine + 256);
	  while (to < end) *to++ += *from++;
	}

	inline void addCoarse (Histogram & that)
	{
	  uint64_t * from = (uint64_t *) that.coarse;
	  uint64_t * to   = (uint64_t *) coarse;
	  to[0] += from[0];
	  to[1] += from[1];
	  to[2] += from[2];
	  to[3] += from[3];
	}

	inline void subtractCoarse (Histogram & that)
	{
	  uint64_t * from = (uint64_t *) that.coarse;
	  uint64_t * to   = (uint64_t *) coarse;
	  to[0] -= from[0];
	  to[1] -= from[1];
	  to[2] -= from[2];
	  to[3] -= from[3];
	}

	inline void addFine (Histogram & that, int c)
	{
	  int        index = c * 16;
	  uint64_t * from  = (uint64_t *) (that.fine + index);
	  uint64_t * to    = (uint64_t *) (fine      + index);
	  to[0] += from[0];
	  to[1] += from[1];
	  to[2] += from[2];
	  to[3] += from[3];
	}

	inline void subtractFine (Histogram & that, int c)
	{
	  int        index = c * 16;
	  uint64_t * from  = (uint64_t *) (that.fine + index);
	  uint64_t * to    = (uint64_t *) (fine      + index);
	  to[0] -= from[0];
	  to[1] -= from[1];
	  to[2] -= from[2];
	  to[3] -= from[3];
	}

	inline void clearFine (int c)
	{
	  uint64_t * p = (uint64_t *) (fine + c * 16);
	  p[0] = 0;
	  p[1] = 0;
	  p[2] = 0;
	  p[3] = 0;
	}

	uint16_t coarse[16];
	uint16_t fine  [256];
  };

  Histogram * histograms = new Histogram[width];
  int heightRadius = min (radius, height);

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
	int r = y - radius - 1;
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

	//   Prepare running total up to just before first column
	int lo = max (left - 1 - radius, 0);
	int hi = min (left - 1 + radius, width - 1);
	for (int x = lo; x <= hi; x++) total += histograms[x];
	for (int i = 0; i < 16; i++) lastColumn[i] = left - 1;

	//   Scan columns
	int count = (hi - lo + 1) * countY;
	for (int x = left; x <= right; x++)
	{
	  r = x - radius - 1;
	  if (r >= 0)
	  {
		total.subtractCoarse (histograms[r]);
		count -= countY;
	  }

	  r = x + radius;
	  if (r < width)
	  {
		total.addCoarse (histograms[r]);
		count += countY;
	  }

	  // Find coarse level
	  int threshold = (int) (order * count);
	  int sum = 0;
	  int c;
	  for (c = 0; c < 16; c++)
	  {
		sum += total.coarse[c];
		if (sum >= threshold) break;
	  }
	  sum -= total.coarse[c];

	  // Update associated 2nd level histogram
	  r = x - radius;
	  if (lastColumn[c] >= r)
	  {
		for (int w = lastColumn[c] + 1; w <= x; w++)
		{
		  r = w - radius - 1;
		  if (r >= 0) total.subtractFine (histograms[r], c);

		  r = w + radius;
		  if (r < width) total.addFine (histograms[r], c);
		}
	  }
	  else
	  {
		total.clearFine (c);
		lo = max (x - radius, 0);
		hi = min (x + radius, width - 1);
		for (int w = lo; w <= hi; w++) total.addFine (histograms[w], c);
	  }
	  lastColumn[c] = x;

	  // Find fine level
	  c *= 16;
	  int last = c + 16;
	  for (; c < last; c++)
	  {
		sum += total.fine[c];
		if (sum >= threshold) break;
	  }
	  outBuffer[y * outStrideV + x * outStrideH] = c;
	}
  }

  delete [] histograms;
}
