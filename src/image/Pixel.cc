/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/image.h"


using namespace std;
using namespace fl;


// class Pixel ----------------------------------------------------------------

Pixel::Pixel ()
{
  format = & RGBAFloat;
  pixel = & data;
}

Pixel::Pixel (unsigned int rgba)
{
  format = & RGBAChar;
  pixel = & data;
  format->setRGBA (pixel, rgba);
}

Pixel::Pixel (const Pixel & that)
{
  format = that.format;
  if (that.pixel == & that.data)
  {
	pixel = & data;
	data = that.data;
  }
  else
  {
	pixel = that.pixel;
	// Ignore the data
  }
}

Pixel::Pixel (const PixelFormat & format, void * pixel)
{
  this->format = & format;
  this->pixel = pixel;
}

unsigned int
Pixel::getRGBA () const
{
  return format->getRGBA (pixel);
}

void
Pixel::getRGBA (float values[]) const
{
  format->getRGBA (pixel, values);
}

void
Pixel::getXYZ (float values[]) const
{
  format->getXYZ (pixel, values);
}

unsigned char
Pixel::getAlpha () const
{
  return format->getAlpha (pixel);
}

void
Pixel::setRGBA (unsigned int rgba)
{
  format->setRGBA (pixel, rgba);
}

void
Pixel::setRGBA (float values[])
{
  format->setRGBA (pixel, values);
}

void
Pixel::setXYZ (float values[])
{
  format->setXYZ (pixel, values);
}

void
Pixel::setAlpha (unsigned char alpha)
{
  format->setAlpha (pixel, alpha);
}

Pixel &
Pixel::operator = (const Pixel & that)
{
  float values[4];
  that.format->getRGBA (that.pixel, values);
  format->setRGBA (pixel, values);
  return *this;
}

Pixel &
Pixel::operator += (const Pixel & that)
{
  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  thisValue[0] += thatValue[0];
  thisValue[1] += thatValue[1];
  thisValue[2] += thatValue[2];
  thisValue[3] += thatValue[3];

  format->setRGBA (pixel, thisValue);

  return *this;
}

Pixel
Pixel::operator + (const Pixel & that) const
{
  Pixel result;

  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  result.data.rgbafloat[0] = thisValue[0] + thatValue[0];
  result.data.rgbafloat[1] = thisValue[1] + thatValue[1];
  result.data.rgbafloat[2] = thisValue[2] + thatValue[2];
  result.data.rgbafloat[3] = thisValue[3] + thatValue[3];

  return result;
}

Pixel
Pixel::operator * (const Pixel & that) const
{
  Pixel result;

  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  result.data.rgbafloat[0] = thisValue[0] * thatValue[0];
  result.data.rgbafloat[1] = thisValue[1] * thatValue[1];
  result.data.rgbafloat[2] = thisValue[2] * thatValue[2];
  result.data.rgbafloat[3] = thisValue[3] * thatValue[3];

  return result;
}

Pixel
Pixel::operator * (float scalar) const
{
  Pixel result;

  float values[4];
  format->getRGBA (pixel, values);

  result.data.rgbafloat[0] = values[0] * scalar;
  result.data.rgbafloat[1] = values[1] * scalar;
  result.data.rgbafloat[2] = values[2] * scalar;
  result.data.rgbafloat[4] = values[3] * scalar;

  return result;
}

Pixel
Pixel::operator / (float scalar) const
{
  Pixel result;

  float values[4];
  format->getRGBA (pixel, values);

  result.data.rgbafloat[0] = values[0] / scalar;
  result.data.rgbafloat[1] = values[1] / scalar;
  result.data.rgbafloat[2] = values[2] / scalar;
  result.data.rgbafloat[4] = values[3] / scalar;

  return result;
}

Pixel
Pixel::operator << (const Pixel & that)
{
  Pixel result;

  float thisValue[4];
  format->getRGBA (pixel, thisValue);
  float thatValue[4];
  that.format->getRGBA (that.pixel, thatValue);

  float a = thatValue[3];
  float a1 = 1.0f - a;
  result.data.rgbafloat[0] = a1 * thisValue[0] + a * thatValue[0];
  result.data.rgbafloat[1] = a1 * thisValue[1] + a * thatValue[1];
  result.data.rgbafloat[2] = a1 * thisValue[2] + a * thatValue[2];
  result.data.rgbafloat[3] = thisValue[3];  // Don't know what to do for alpha values themselves

  return result;
}
