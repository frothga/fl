/*
Author: Fred Rothganger
Created 3/26/08


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/image.h"
#include "fl/string.h"

#include <png.h>

#include <typeinfo>


using namespace std;
using namespace fl;


// class ImageFileDelegatePNG -------------------------------------------------

class ImageFileDelegatePNG : public ImageFileDelegate
{
public:
  ImageFileDelegatePNG (istream * in, ostream * out, bool ownStream = false);
  ~ImageFileDelegatePNG ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  static void errorHandler   (png_structp png, png_const_charp message);
  static void warningHandler (png_structp png, png_const_charp message);
  static void readHandler    (png_structp png, png_bytep data, png_size_t length);
  static void writeHandler   (png_structp png, png_bytep data, png_size_t length);
  static void flushHandler   (png_structp png);

  istream * in;
  ostream * out;
  bool ownStream;

  png_structp png;
  png_infop   info;
  png_bytep * rows;  ///< Row array used to read or writ image.  Size varies, of course, based on image.height
  png_colorp  palette;
  int         paletteCount;
  int         colorFormat;
  int         depth;
  png_uint_32 totalWidth;
  png_uint_32 totalHeight;

  PointerPoly<const PixelFormat> format;
};

ImageFileDelegatePNG::ImageFileDelegatePNG (istream * in, ostream * out, bool ownStream)
{
  this->in = in;
  this->out = out;
  this->ownStream = ownStream;

  png = 0;
  info = 0;
  palette = 0;
  rows = 0;

  if (in)
  {
	png = png_create_read_struct (PNG_LIBPNG_VER_STRING, this, errorHandler, warningHandler);
	if (!png) throw "Unable to initialize libpng for reading";
	info = png_create_info_struct (png);
	if (!info) throw "Unable to initialize libpng for reading";
	png_set_read_fn (png, this, readHandler);
	png_read_info (png, info);
  }

  if (out)
  {
	png = png_create_write_struct (PNG_LIBPNG_VER_STRING, this, errorHandler, warningHandler);
	if (!png) throw "Unable to initialize libpng for writing";
	info = png_create_info_struct (png);
	if (!info) throw "Unable to initialize libpng for writing";
	png_set_write_fn (png, this, writeHandler, flushHandler);
  }
}

ImageFileDelegatePNG::~ImageFileDelegatePNG ()
{
  // Assumes libpng handles null pointers gracefully.
  if      (in)  png_destroy_read_struct  (&png, &info, 0);
  else if (out) png_destroy_write_struct (&png, &info);

  free (palette);  // palette is never owned by libpng; palette is generally null
  free (rows);

  if (ownStream)
  {
	if (in)  delete in;
	if (out) delete out;
  }
}

void
ImageFileDelegatePNG::read (Image & image, int x, int y, int width, int height)
{
  if (! in  ||  ! png  ||  ! info) throw "ImageFileDelegatePNG not open for reading";

  // Select pixel format
  // format also acts as flag to indicate that header has been read
  if (format == 0)
  {
	int interlace;
	png_get_IHDR (png, info, &totalWidth, &totalHeight, &depth, &colorFormat, &interlace, 0, 0);

	switch (colorFormat)
	{
	  case PNG_COLOR_TYPE_GRAY:
		if      (depth < 8)   format = new PixelFormatGrayBits (depth);
		else if (depth == 8)  format = &GrayChar;
		else if (depth == 16) format = &GrayShort;
		break;
	  case PNG_COLOR_TYPE_PALETTE:
		if (depth <= 8)
		{
		  if (! png_get_valid (png, info, PNG_INFO_PLTE)) throw "Palette type image, but palette table is missing.";
		  png_colorp palette;  // hides this->palette, which is only used during writing
		  int count;
		  png_get_PLTE (png, info, &palette, &count);
		  format = new PixelFormatPalette (&palette->red, &palette->green, &palette->blue, sizeof (png_color), depth);
		}
		break;
	  case PNG_COLOR_TYPE_RGB:
		if      (depth == 8)  format = &RGBChar;
		else if (depth == 16) format = &RGBShort;
		break;
	  case PNG_COLOR_TYPE_RGB_ALPHA:
		if      (depth == 8)  format = &RGBAChar;
		else if (depth == 16) format = &RGBAShort;
		break;
	  case PNG_COLOR_TYPE_GRAY_ALPHA:
		if      (depth == 8)  format = &GrayAlphaChar;
		else if (depth == 16) format = &GrayAlphaShort;
		break;
	}
  }
  if (format == 0) throw "No matching PiexlFormat found.";

# if BYTE_ORDER == LITTLE_ENDIAN
  png_set_swap (png);
# endif

  // Read image
  image.format = format;
  image.resize (totalWidth, totalHeight);  // ignore the requested width and height; we only read full images
  rows = (png_bytep *) malloc (totalHeight * sizeof (png_bytep));
  if (PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer)
  {
	png_bytep pixel = (png_bytep) buffer->memory;
	for (int y = 0; y < totalHeight; y++)
	{
	  rows[y] = pixel;
	  pixel += buffer->stride;
	}
  }
  else if (PixelBufferGroups * buffer = (PixelBufferGroups *) image.buffer)
  {
	png_bytep pixel = (png_bytep) buffer->memory;
	for (int y = 0; y < totalHeight; y++)
	{
	  rows[y] = pixel;
	  pixel += buffer->stride;
	}
  }
  else throw "PixelBuffer type not yet handled";
  png_read_image (png, rows);

  png_read_end (png, info);
}

void
ImageFileDelegatePNG::write (const Image & image, int x, int y)
{
  if (! out  ||  ! png  ||  ! info) throw "ImageFileDelegatePNG not open for writing";

  if (format == 0)
  {
	colorFormat = 0;  // default is gray
	if (image.format->monochrome)
	{
	  if      (image.format->depth == 1.0f / 8.0f) format = new PixelFormatGrayBits (1);
	  else if (image.format->depth == 2.0f / 8.0f) format = new PixelFormatGrayBits (2);
	  else if (image.format->depth == 4.0f / 8.0f) format = new PixelFormatGrayBits (4);
	  else if (image.format->depth <= 1.0f)        format = &GrayChar;
	  else                                         format = &GrayShort;
	  depth = (int) roundp (format->depth * 8);
	}
	else
	{
	  colorFormat |= PNG_COLOR_MASK_COLOR;

	  if (const PixelFormatPalette * pfp = (const PixelFormatPalette *) image.format)
	  {
		colorFormat |= PNG_COLOR_MASK_PALETTE;
		format = pfp;
		depth = (int) roundp (format->depth * 8);

		paletteCount = (0x1 << pfp->bits);
		palette = (png_colorp) malloc (sizeof (png_color) * paletteCount);
		png_color          * o   = palette;
		const unsigned int * i   = pfp->palette;
		const unsigned int * end = i + paletteCount;
		while (i < end)
		{
		  o->red   = (*i & 0xFF000000) >> 24;
		  o->green = (*i &   0xFF0000) >> 16;
		  o->blue  = (*i &     0xFF00) >>  8;
		  i++;
		  o++;
		}
	  }
	  else if (image.format->hasAlpha)
	  {
		colorFormat |= PNG_COLOR_MASK_ALPHA;
		if (image.format->depth <= 4.0f)
		{
		  format = &RGBAChar;
		  depth = 8;
		}
		else
		{
		  format = &RGBAShort;
		  depth = 16;
		}
	  }
	  else
	  {
		if (image.format->depth <= 3.0f)
		{
		  format = &RGBChar;
		  depth = 8;
		}
		else
		{
		  format = &RGBShort;
		  depth = 16;
		}
	  }
	}
  }
  assert (format != 0);
  Image work = image * *format;

  rows = (png_bytep *) malloc (work.height * sizeof (png_bytep));
  if (PixelBufferPacked * buffer = (PixelBufferPacked *) work.buffer)
  {
	png_bytep pixel = (png_bytep) buffer->memory;
	for (int y = 0; y < work.height; y++)
	{
	  rows[y] = pixel;
	  pixel += buffer->stride;
	}
  }
  else if (PixelBufferGroups * buffer = (PixelBufferGroups *) work.buffer)
  {
	png_bytep pixel = (png_bytep) buffer->memory;
	for (int y = 0; y < work.height; y++)
	{
	  rows[y] = pixel;
	  pixel += buffer->stride;
	}
  }
  else throw "PixelBuffer type not yet handled";

  png_set_IHDR (png, info,
                work.width, work.height, depth, colorFormat,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  if (palette) png_set_PLTE (png, info, palette, paletteCount);
  png_write_info (png, info);

# if BYTE_ORDER == LITTLE_ENDIAN
  png_set_swap (png);
# endif

  png_write_image (png, rows);
  png_write_end (png, info);
}

void
ImageFileDelegatePNG::errorHandler (png_structp png, png_const_charp message)
{
  throw message;
}

void
ImageFileDelegatePNG::warningHandler (png_structp png, png_const_charp message)
{
  cerr << message << endl;
}

void
ImageFileDelegatePNG::readHandler (png_structp png, png_bytep data, png_size_t length)
{
  ImageFileDelegatePNG * me = (ImageFileDelegatePNG *) png_get_io_ptr (png);
  me->in->read ((char *) data, length);
}

void
ImageFileDelegatePNG::writeHandler (png_structp png, png_bytep data, png_size_t length)
{
  ImageFileDelegatePNG * me = (ImageFileDelegatePNG *) png_get_io_ptr (png);
  me->out->write ((char *) data, length);
}

void
ImageFileDelegatePNG::flushHandler (png_structp png)
{
  ImageFileDelegatePNG * me = (ImageFileDelegatePNG *) png_get_io_ptr (png);
  me->out->flush ();
}


// class ImageFileFormatPNG --------------------------------------------------

void
ImageFileFormatPNG::use ()
{
  vector<ImageFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (typeid (**i) == typeid (ImageFileFormatPNG)) return;
  }
  formats.push_back (new ImageFileFormatPNG);
}

ImageFileDelegate *
ImageFileFormatPNG::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegatePNG (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatPNG::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegatePNG (0, &stream, ownStream);
}

float
ImageFileFormatPNG::isIn (std::istream & stream) const
{
  string magic = "        ";  // 8 spaces
  getMagic (stream, magic);
  if (png_sig_cmp ((png_bytep) magic.c_str (), (png_size_t) 0, magic.size ()) == 0) return 1;
  return 0;
}

float
ImageFileFormatPNG::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "png") == 0)
  {
	return 1;
  }
  return 0;
}
