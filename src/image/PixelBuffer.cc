/*
Author: Fred Rothganger


Revision 1.1 thru 1.6 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/08/02 12:32:05  Fred
Change stride to mean bytes rather than pixels.

Change depth to a float.

Add PixelBufferBits and PixelBufferYUYV.

Revision 1.5  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.4  2006/04/08 14:42:55  Fred
Implement PixelBufferUYYVYY.

Revision 1.3  2006/04/02 02:39:48  Fred
Add PixelBufferPacked constructor that binds to a Pointer object.

Change PixelFormatPlanar class to PixelFormatYUV, and change
PixelBufferPlanar::resize() to assume other planar formats exist besides YUV,
but that any other format has equal sized planes.

Revision 1.2  2006/03/20 05:36:53  Fred
Finish writing PixelBufferPlanar.  Move job of preserving buffer contents into
a separate function, so it can be reused by PixelBuffer classes.

Revision 1.1  2006/03/13 02:40:14  Fred
Classes for managing the memory layout of an Image.
-------------------------------------------------------------------------------
*/


#include "fl/image.h"


using namespace fl;
using namespace std;


// Utility functions ----------------------------------------------------------

/**
   \param oldStride Curent width of one row in bytes (not pixels).
   \param newStride Desired width of one row in bytes.
 **/
void
reshapeBuffer (Pointer & memory, int oldStride, int newStride, int newHeight)
{
  int oldHeight = memory.size ();
  if (oldHeight <= 0)
  {
	oldHeight = 0;
  }
  else if (oldStride > 0)
  {
	oldHeight /= oldStride;
  }
  int copyWidth  = min (newStride, oldStride);
  int copyHeight = min (newHeight, oldHeight);

  if (newStride == oldStride)
  {
	if (newHeight > oldHeight)
	{
	  Pointer temp (memory);
	  memory.detach ();
	  memory.grow (newStride * newHeight);
	  int count = newStride * copyHeight;
	  memcpy (memory.memory, temp.memory, count);
	  assert (count >= 0  &&  count < memory.size ());
	  memset ((char *) memory + count, 0, memory.size () - count);
	}
  }
  else  // different strides
  {
	Pointer temp (memory);
	memory.detach ();
	memory.grow (newStride * newHeight);
	memory.clear ();

	unsigned char * target = (unsigned char *) memory;
	unsigned char * source = (unsigned char *) temp;
	for (int y = 0; y < copyHeight; y++)
	{
	  memcpy (target, source, copyWidth);
	  target += newStride;
	  source += oldStride;
	}
  }
}


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
: memory (stride * height)
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
  this->memory.attach (buffer, stride * height);
}

PixelBufferPacked::PixelBufferPacked (const Pointer & buffer, int stride, int depth)
{
  planes       = 1;
  this->stride = stride;
  this->depth  = depth;
  this->memory = buffer;
}

PixelBufferPacked::~PixelBufferPacked ()
{
}

void *
PixelBufferPacked::pixel (int x, int y)
{
  return & ((char *) memory)[y * stride + x * depth];
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
  // If p exists, then implicitly the number of planes is 1.
  return    p
         && stride == p->stride
         && depth  == p->depth
         && memory == p->memory;
}

void
PixelBufferPacked::copyFrom (void * buffer, int stride, int height, int depth)
{
  this->memory.copyFrom (buffer, stride * height);
  this->stride = stride;
  this->depth  = depth;
}

void
PixelBufferPacked::resize (int width, int height, const PixelFormat & format, bool preserve)
{
  if (width <= 0  ||  height <= 0)
  {
	stride = 0;
	depth  = (int) format.depth;
	memory.detach ();
	return;
  }

  if (! preserve  ||  format.depth != depth)
  {
	depth  = (int) format.depth;
	stride = width * depth;
	memory.grow (stride * height);
	return;
  }

  reshapeBuffer (memory, stride, width * depth, height);
  stride = width * depth;
}


// class PixelBufferPlanar ----------------------------------------------------

PixelBufferPlanar::PixelBufferPlanar ()
{
  planes   = 3;
  stride0  = 0;
  stride12 = 0;
  ratioH   = 1;
  ratioV   = 1;
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

/**
   Attach to an FFMPEG picture.
 **/
PixelBufferPlanar::PixelBufferPlanar (void * buffer0, void * buffer1, void * buffer2, int stride0, int stride12, int height, int ratioH, int ratioV)
{
  planes         = 3;
  this->stride0  = stride0;
  this->stride12 = stride12;
  this->ratioH   = ratioH;
  this->ratioV   = ratioV;

  plane0.attach (buffer0, stride0  * height);
  plane1.attach (buffer1, stride12 * height);
  plane2.attach (buffer2, stride12 * height);
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
  pixelArray[2] = ((char *) plane2) + (y12 * stride12 + x12);

  return pixelArray;
}

void
PixelBufferPlanar::resize (int width, int height, const PixelFormat & format, bool preserve)
{
  if (width <= 0  ||  height <= 0)
  {
	plane0.detach ();
	plane1.detach ();
	plane2.detach ();
	return;
  }

  assert (format.depth == 1);  // may generalize to variable depth, if there exists a case that needs it

  const PixelFormatYUV * f = dynamic_cast<const PixelFormatYUV *> (&format);
  if (f)
  {
	ratioH = f->ratioH;
	ratioV = f->ratioV;
  }
  else
  {
	ratioH = 1;
	ratioV = 1;
  }

  if (preserve)
  {
	reshapeBuffer (plane0, stride0,  width,          height);
	reshapeBuffer (plane1, stride12, width / ratioH, height / ratioV);
	reshapeBuffer (plane2, stride12, width / ratioH, height / ratioV);
	stride0  = width;
	stride12 = width / ratioH;
  }
  else
  {
	stride0  = width;
	stride12 = width / ratioH;
	plane0.grow (stride0  * height);
	plane1.grow (stride12 * height);
	plane2.grow (stride12 * height);
  }
}

PixelBuffer *
PixelBufferPlanar::duplicate () const
{
  PixelBufferPlanar * result = new PixelBufferPlanar ();
  result->ratioH   = ratioH;
  result->ratioV   = ratioV;
  result->stride0  = stride0;
  result->stride12 = stride12;

  result->plane0.copyFrom (plane0);
  result->plane1.copyFrom (plane1);
  result->plane2.copyFrom (plane2);

  return result;
}

void
PixelBufferPlanar::clear ()
{
  plane0.clear ();
  plane1.clear ();
  plane2.clear ();
}

bool
PixelBufferPlanar::operator == (const PixelBuffer & that) const
{
  const PixelBufferPlanar * p = dynamic_cast<const PixelBufferPlanar *> (&that);
  return    p
         && ratioH   == p->ratioH
         && ratioV   == p->ratioV
         && stride0  == p->stride0
         && stride12 == p->stride12
         && plane0   == p->plane0
         && plane1   == p->plane1
         && plane2   == p->plane2;
}


// class PixelBufferBits ------------------------------------------------------

PixelBufferBits::PixelBufferBits (int slices)
{
  planes       = -1;
  this->slices = slices;
  stride       = 0;
}

PixelBufferBits::PixelBufferBits (int stride, int height, int slices)
{
  planes       = -1;
  this->slices = slices;
  this->stride = stride;
  memory.grow (stride * height);
}

PixelBufferBits::PixelBufferBits (void * buffer, int stride, int height, int slices)
{
  planes       = -1;
  this->slices = slices;
  this->stride = stride;
  memory.attach (buffer, stride * height);
}

PixelBufferBits::~PixelBufferBits ()
{
}

void *
PixelBufferBits::pixel (int x, int y)
{
  pixelData.address = (unsigned char *) memory + y * stride + x / slices;
  pixelData.index   = x % slices;
  return & pixelData;
}

void
PixelBufferBits::resize (int width, int height, const PixelFormat & format, bool preserve)
{
  if (width <= 0  ||  height <= 0)
  {
	stride = 0;
	memory.detach ();
	return;
  }

  if (! preserve  ||  format.depth != 1.0f / slices)
  {
	slices = (int) (1.0f / format.depth);
	stride = width / slices;
	if (width % slices) stride++;
	memory.grow (stride * height);
	return;
  }

  int newStride = width / slices;
  if (width % slices) newStride++;
  reshapeBuffer (memory, stride, newStride, height);
  stride = newStride;
}

PixelBuffer *
PixelBufferBits::duplicate () const
{
  PixelBufferBits * result = new PixelBufferBits (slices);
  result->memory.copyFrom (memory);
  result->stride = stride;
  return result;
}

void
PixelBufferBits::clear ()
{
  memory.clear ();
}

bool
PixelBufferBits::operator == (const PixelBuffer & that) const
{
  const PixelBufferBits * p = dynamic_cast<const PixelBufferBits *> (&that);
  return    p
         && stride == p->stride
         && slices == p->slices
         && memory == p->memory;
}


// class PixelBufferUYYVYY ----------------------------------------------------

PixelBufferUYYVYY::PixelBufferUYYVYY ()
{
  planes = 3;
  stride = 0;
}

PixelBufferUYYVYY::PixelBufferUYYVYY (void * buffer, int stride, int height)
{
  planes       = 3;
  this->stride = stride;
  memory.attach (buffer, stride * height);
}

PixelBufferUYYVYY::~PixelBufferUYYVYY ()
{
}

void *
PixelBufferUYYVYY::pixel (int x, int y)
{
  int h = x / 4;
  int p = x % 4;
  unsigned char * base = & ((unsigned char *) memory)[y * stride + h * 6];
  if (p < 2)
  {
	pixelArray[0] = base + (p + 1);
  }
  else
  {
	pixelArray[0] = base + (p + 2);
  }
  pixelArray[1] = base;
  pixelArray[2] = base + 3;

  return pixelArray;
}

void
PixelBufferUYYVYY::resize (int width, int height, const PixelFormat & format, bool preserve)
{
  assert (width % 4 == 0);

  if (width <= 0  ||  height <= 0)
  {
	stride = 0;
	memory.detach ();
	return;
  }

  int newStride = width * 6 / 4;
  if (! preserve)
  {
	stride = newStride;
	memory.grow (stride * height);
	return;
  }

  reshapeBuffer (memory, stride, newStride, height);
  stride = newStride;
}

PixelBuffer *
PixelBufferUYYVYY::duplicate () const
{
  PixelBufferUYYVYY * result = new PixelBufferUYYVYY;
  result->memory.copyFrom (memory);
  result->stride = stride;
  return result;
}

void
PixelBufferUYYVYY::clear ()
{
  memory.clear ();
}

bool
PixelBufferUYYVYY::operator == (const PixelBuffer & that) const
{
  const PixelBufferUYYVYY * p = dynamic_cast<const PixelBufferUYYVYY *> (&that);
  return    p
         && stride == p->stride
         && memory == p->memory;
}


// class PixelBufferYUYV ----------------------------------------------------

PixelBufferYUYV::PixelBufferYUYV (bool swap)
{
  planes = 3;
  stride = 0;
  this->swap = swap;
}

PixelBufferYUYV::PixelBufferYUYV (void * buffer, int stride, int height, bool swap)
{
  planes       = 3;
  this->stride = stride;
  this->swap   = swap;
  memory.attach (buffer, stride * height);
}

PixelBufferYUYV::~PixelBufferYUYV ()
{
}

void *
PixelBufferYUYV::pixel (int x, int y)
{
  int h = x / 2;
  int p = x % 2;
  unsigned char * base = & ((unsigned char *) memory)[y * stride + h * 4];
  if (swap)
  {
	pixelArray[0] = base + (p * 2 + 1);
	pixelArray[1] = base;
	pixelArray[2] = base + 2;
  }
  else
  {
	pixelArray[0] = base + p * 2;
	pixelArray[1] = base + 1;
	pixelArray[2] = base + 3;
  }

  return pixelArray;
}

void
PixelBufferYUYV::resize (int width, int height, const PixelFormat & format, bool preserve)
{
  assert (width % 2 == 0);

  if (width <= 0  ||  height <= 0)
  {
	stride = 0;
	memory.detach ();
	return;
  }

  int newStride = width * 2;
  if (! preserve)
  {
	stride = newStride;
	memory.grow (stride * height);
	return;
  }

  reshapeBuffer (memory, stride, newStride, height);
  stride = newStride;
}

PixelBuffer *
PixelBufferYUYV::duplicate () const
{
  PixelBufferYUYV * result = new PixelBufferYUYV (swap);
  result->memory.copyFrom (memory);
  result->stride = stride;
  return result;
}

void
PixelBufferYUYV::clear ()
{
  memory.clear ();
}

bool
PixelBufferYUYV::operator == (const PixelBuffer & that) const
{
  const PixelBufferYUYV * p = dynamic_cast<const PixelBufferYUYV *> (&that);
  return    p
         && stride == p->stride
         && swap   == p->swap
         && memory == p->memory;
}
