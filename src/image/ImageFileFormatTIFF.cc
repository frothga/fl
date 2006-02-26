/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


05/2005 Fred Rothganger -- Fix bugs with reading.  Complete support for
        writing.  Use new PixelFormat names.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


02/2006 Fred Rothganger -- Change Image structure.  Separate ImageFile.
*/


#include "fl/image.h"
#include "fl/string.h"
#include "fl/math.h"

#include <tiffio.h>


using namespace std;
using namespace fl;


// class ImageFileTIFF --------------------------------------------------------

class ImageFileTIFF : public ImageFile
{
public:
  ImageFileTIFF (TIFF * tif)
  {
	this->tif = tif;
  }
  virtual ~ImageFileTIFF ();

  virtual void read (Image & image);
  virtual void write (const Image & image);

  TIFF * tif;
};

ImageFileTIFF::~ImageFileTIFF ()
{
  TIFFClose (tif);
}

void
ImageFileTIFF::read (Image & image)
{
  if (! tif) throw "ImageFileTIFF not open";

  bool ok = true;

  uint32 w, h;
  ok &= TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &w);
  ok &= TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &h);

  uint16 samplesPerPixel;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
  uint16 bitsPerSample;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
  uint16 sampleFormat;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
  uint16 photometric;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PHOTOMETRIC, &photometric);
  uint16 orientation;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_ORIENTATION, &orientation);
  uint16 planarConfig;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PLANARCONFIG, &planarConfig);
  uint16 extra;
  uint16 * extraFormat;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_EXTRASAMPLES, &extra, &extraFormat);

  if (! ok)
  {
	throw "Unable to get needed tag values.";
  }
  if (photometric > 2)
  {
	throw "Can't handle color palettes, transparency masks, etc.";
	// But we should handle the YCbCr and L*a*b* color spaces.
  }
  if (planarConfig != PLANARCONFIG_CONTIG)
  {
	throw "Can't handle planar formats";
  }
  if (extra > 1)
  {
	throw "No PixelFormats currently support more than one channel beyond three colors.";
  }

  PixelFormat * format = 0;
  switch (bitsPerSample)
  {
	case 8:
	  switch (samplesPerPixel)
	  {
		case 1:
		  format = &GrayChar;
		  break;
		case 3:
		  format = &RGBChar;
		  break;
		case 4:
		  format = &RGBAChar;
	  }
	  break;
	case 16:
	  switch (samplesPerPixel)
	  {
		case 1:
		  format = &GrayShort;
		  break;
		case 3:
		  format = &RGBShort;
		  break;
		case 4:
		  format = &RGBAShort;
	  }
	  break;
	case 32:
	  if (sampleFormat == SAMPLEFORMAT_IEEEFP)
	  {
		switch (samplesPerPixel)
		{
		  case 1:
			format = &GrayFloat;
			break;
		  case 4:
			format = &RGBAFloat;
		}
	  }
  }
  if (! format)
  {
	throw "No PixelFormat available that matches file contents";
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
  image.format = format;
  image.resize (w, h);

  unsigned char * p = (unsigned char *) buffer->memory;
  int stride = image.format->depth * w;
  for (int y = 0; y < h; y++)
  {
	TIFFReadScanline (tif, p, y);
	p += stride;
  }
}

void
ImageFileTIFF::write (const Image & image)
{
  if (! tif) throw "ImageFileTIFF not open";

  if (image.format->monochrome)
  {
	if (*image.format != GrayChar  &&  *image.format != GrayShort  &&  *image.format != GrayFloat  &&  *image.format != GrayDouble)
	{
	  write (image * GrayChar);
	  return;
	}
  }
  else if (image.format->hasAlpha)
  {
	if (*image.format != RGBAChar  &&  *image.format != RGBAShort  &&  *image.format != RGBAFloat)
	{
	  write (image * RGBAChar);
	  return;
	}
  }
  else  // Three color channels
  {
	if (*image.format != RGBChar  &&  *image.format != RGBShort)
	{
	  write (image * RGBChar);
	  return;
	}
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "TIFF only handles packed buffers for now";


  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, image.width);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, image.height);
  TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  if (image.format->monochrome)
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

	if (*image.format == GrayChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == GrayShort)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == GrayFloat)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
	else if (*image.format == GrayDouble)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 64);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
  }
  else if (image.format->hasAlpha)
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if (*image.format == RGBAChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == RGBAShort)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else  // RGBAFloat
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
  }
  else  // 3 color channels
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if (*image.format == RGBChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else  // RGBShort
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
  }

  TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

  unsigned char * p = (unsigned char *) buffer->memory;
  int stride = image.format->depth * buffer->stride;
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (int) ceil (8.0 * 1024 / stride));  // Manual recommends 8K strips
  for (int y = 0; y < image.height; y++)
  {
	TIFFWriteScanline (tif, p, y, 0);
    p += stride;
  }
}


// class ImageFileFormatTIFF --------------------------------------------------

ImageFile *
ImageFileFormatTIFF::open (const string & fileName, const string & mode) const
{
  TIFF * tif = TIFFOpen (fileName.c_str (), mode.c_str ());
  if (! tif)
  {
	throw "Unable to open file.";
  }
  return new ImageFileTIFF (tif);
}

ImageFile *
ImageFileFormatTIFF::open (istream & stream) const
{
  throw "ImageFileFormatTIFF does not yet support stream style I/O";
}

ImageFile *
ImageFileFormatTIFF::open (ostream & stream) const
{
  throw "ImageFileFormatTIFF does not yet support stream style I/O";
}

float
ImageFileFormatTIFF::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);

  float result = 0;
  if ( magic.substr (0, 2) == "II"  ||  magic.substr (0, 2) == "MM")  // endian indicator
  {
	result += 0.5;
  }
  // Apparently, some implementation don't store the magic number 42 according
  // to the endian indicated by the first two bytes, so we have to handle the
  // wrong order as well.
  if ((magic[2] == '\x00'  ||  magic[2] == '\x2A')  &&  (magic[3] == '\x2A'  ||  magic[3] == '\x00'))
  {
	result += 0.5;
  }

  return result;
}

float
ImageFileFormatTIFF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "tiff") == 0)
  {
	return 1;
  }
  if (strcasecmp (formatName.c_str (), "tif") == 0)
  {
	return 0.8;
  }
  return 0;
}
