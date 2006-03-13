#include "fl/image.h"


using namespace fl;
using namespace std;


// class PixelBuffer ----------------------------------------------------------

PixelBuffer::~PixelBuffer ()
{
}

bool
PixelBuffer::operator == (const PixelBuffer & that) const
{
  return    typeid (*this) == typeid (that)
         && planes == that.planes;
}


// class PixelBufferPacked ----------------------------------------------------

PixelBufferPacked::PixelBufferPacked (int depth)
{
  planes      = 1;
  stride      = 0;
  this->depth = depth;
}

PixelBufferPacked::PixelBufferPacked (int stride, int height, int depth)
: memory (stride * height * depth)
{
  planes       = 1;
  this->stride = stride;
  this->depth  = depth;
}

PixelBufferPacked::PixelBufferPacked (void * buffer, int stride, int height, int depth)
{
  planes       = 1;
  this->stride = stride;
  this->depth  = depth;
  this->memory.attach (buffer, stride * height * depth);
}

PixelBufferPacked::~PixelBufferPacked ()
{
}

void *
PixelBufferPacked::pixel (int x, int y)
{
  return & ((char *) memory)[(y * stride + x) * depth];
}

PixelBuffer *
PixelBufferPacked::duplicate () const
{
  PixelBufferPacked * result = new PixelBufferPacked (depth);
  result->memory.copyFrom (memory);
  result->stride = stride;
  return result;
}

void
PixelBufferPacked::clear ()
{
  memory.clear ();
}

bool
PixelBufferPacked::operator == (const PixelBuffer & that) const
{
  const PixelBufferPacked * p = dynamic_cast<const PixelBufferPacked *> (&that);
  // If p is non-zero, then implicitly the member "packed" is true.
  // The exception to this might be a PixelBuffer that inherits from
  // PixelBufferPacked but doesn't otherwise claim to be a packed format.
  return    p
         && stride == p->stride
         && depth  == p->depth
         && memory == p->memory;
}

void
PixelBufferPacked::copyFrom (void * buffer, int stride, int height, int depth)
{
  this->memory.copyFrom (buffer, stride * height * depth);
  this->stride = stride;
  this->depth  = depth;
}

void
PixelBufferPacked::resize (int width, int height, const PixelFormat & format, bool preserve)
{
  if (width <= 0  ||  height <= 0)
  {
	stride = 0;
	depth  = format.depth;
	memory.detach ();
	return;
  }

  if (! preserve  ||  format.depth != depth)
  {
	stride = width;
	depth  = format.depth;
	memory.grow (width * height * format.depth);
	return;
  }

  int myHeight = memory.size ();
  int byteWidth = stride * depth;  // at this point, depth is guaranteed to be same as format.depth
  if (myHeight <= 0)
  {
	myHeight = 0;
  }
  else if (byteWidth > 0)
  {
	myHeight /= byteWidth;
  }
  int copyWidth  = min (width,  stride);
  int copyHeight = min (height, myHeight);

  if (width == stride)
  {
	if (height > myHeight)
	{
	  Pointer temp (memory);
	  memory.detach ();
	  memory.grow (width * height * depth);
	  int count = width * copyHeight * depth;
	  memcpy (memory.memory, temp.memory, count);
	  assert (count >= 0  &&  count < memory.size ());
	  memset ((char *) memory + count, 0, memory.size () - count);
	}
  }
  else
  {
	Pointer temp (memory);
	memory.detach ();
	memory.grow (width * height * depth);
	memory.clear ();

	unsigned char * target = (unsigned char *) memory;
	int targetStride = width * depth;
	unsigned char * source = (unsigned char *) temp;
	int sourceStride = stride * depth;
	int count = copyWidth * depth;

	for (int y = 0; y < copyHeight; y++)
	{
	  memcpy (target, source, count);
	  target += targetStride;
	  source += sourceStride;
	}

	stride = width;
  }
}


// class PixelBufferPlanar ----------------------------------------------------

PixelBufferPlanar::PixelBufferPlanar ()
{
  planes   = 3;
  stride0  = 0;
  stride12 = 0;
  ratioH   = 0;
  ratioV   = 0;
}

PixelBufferPlanar::PixelBufferPlanar (int stride, int height, int ratioH, int ratioV)
{
  planes       = 3;
  stride0      = stride;
  stride12     = stride / ratioH;
  this->ratioH = ratioH;
  this->ratioV = ratioV;

  plane0.grow (stride0  * height);
  plane1.grow (stride12 * height);
  plane2.grow (stride12 * height);
}

PixelBufferPlanar::PixelBufferPlanar (void * buffer0, void * buffer1, void * buffer2, int stride, int height, int ratioH, int ratioV)
{
  planes       = 3;
  stride0      = stride;
  stride12     = stride / ratioH;
  this->ratioH = ratioH;
  this->ratioV = ratioV;

  plane0.attach (buffer0, stride0  * height);
  plane1.attach (buffer0, stride12 * height);
  plane2.attach (buffer0, stride12 * height);
}

PixelBufferPlanar::~PixelBufferPlanar ()
{
}

void *
PixelBufferPlanar::pixel (int x, int y)
{
  int x12 = x / ratioH;
  int y12 = y / ratioV;

  pixelArray[0] = ((char *) plane0) + (y   * stride0  + x);
  pixelArray[1] = ((char *) plane1) + (y12 * stride12 + x12);
  pixelArray[2] = ((char *) plane1) + (y12 * stride12 + x12);

  return pixelArray;
}

void
PixelBufferPlanar::resize (int width, int height, const PixelFormat & format, bool preserve)
{
  assert (format.depth == 1);  // may generalize to variable depth, if there exists a case that needs it
  // ratioH = 
}

PixelBuffer *
PixelBufferPlanar::duplicate () const
{
  return 0;
}

void
PixelBufferPlanar::clear ()
{
}

bool
PixelBufferPlanar::operator == (const PixelBuffer & that) const
{
  return false;
}
