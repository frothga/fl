#include "fl/image.h"


#include <tiffio.h>


using namespace std;
using namespace fl;


// class ImageFileFormatTIFF --------------------------------------------------

void
ImageFileFormatTIFF::read (const std::string & fileName, Image & image) const
{
  TIFF * tif = TIFFOpen (fileName.c_str (), "r");
  if (! tif)
  {
	throw "Unable to open file.";
  }

  uint32 w, h;
  TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &w);
  TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &h);

  //image.format = &ABGRChar;  // for the more general TIFFReadRGBAImage
  image.format = &RGBAChar;
  image.resize (w, h);  // using malloc rather than _TIFFmalloc

  // Hack just for reading Steve's tiffs.  A more general routine would check
  // samples-per-pixel and bits-per-sample and adjust accordingly.
  int size = TIFFScanlineSize (tif);
  size /= 2;
  unsigned short * buffer = new unsigned short[size];
  for (int y = 0; y < h; y++)
  {
	TIFFReadScanline (tif, buffer, y);
	for (int x = 0; x < w; x++)
	{
	  unsigned int r = buffer[x * 4 + 0] >> 8;
	  unsigned int g = buffer[x * 4 + 1] >> 8;
	  unsigned int b = buffer[x * 4 + 2] >> 8;
	  unsigned int a = buffer[x * 4 + 3] >> 8;
	  image.setRGBA (x, y, (a << 24) | (r << 16) | (g << 8) | b);
	}
  }
  delete buffer;


  // Reader code that works, but loses alpha
  /*
  if (! TIFFReadRGBAImage (tif, w, h, image.buffer, 0))
  {
	throw "TIFFReadRGBAImage failed.";
  }
  */

  TIFFClose(tif);

  /*
  ImageOf<unsigned int> bogus (image);
  for (int y = 0; y < image.height; y++)
  {
	for (int x = 0; x < image.width; x++)
	{
	  if (((bogus (x, y) >> 24) & 0xFF) != 0x0)
	  {
		cerr << x << " " << y << " " << hex << bogus (x, y) << dec << endl;
	  }
	}
  }
  */
}

void
ImageFileFormatTIFF::read (std::istream & stream, Image & image) const
{
  throw "Can't read TIFF on stream (limitation of libtiff and standard).";
}

void
ImageFileFormatTIFF::write (const std::string & fileName, const Image & image) const
{
  if (image.format->monochrome)
  {
	if (*image.format != GrayChar)
	{
	  Image temp = image * GrayChar;
	  write (fileName, temp);
	  return;
	}
  }
  else
  {
	if (*image.format != RGBAChar)
	{
	  Image temp = image * RGBAChar;
	  write (fileName, temp);
	  return;
	}
  }


  TIFF * tif = TIFFOpen (fileName.c_str (), "w");

  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, image.width);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, image.height);
  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  if (*image.format == GrayChar)
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  }
  else  // format == RGBAChar
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  }

  /*
                  tsize_t linebytes = sampleperpixel * width;     // length in memory of one
                  row of pixel in the image. 

                  unsigned char *buf = NULL;        // buffer used to store the row of pixel
                  information for writing to file
                  //    Allocating memory to store the pixels of current row
                  if (TIFFScanlineSize(out)linebytes)
                      buf =(unsigned char *)_TIFFmalloc(linebytes);
                  else
                      buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));

                  // We set the strip size of the file to be size of one row of pixels
                  TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
                  TIFFDefaultStripSize(out, width*sampleperpixel));

                  //Now writing image to the file one strip at a time
                  for (uint32 row = 0; row < h; row++)
                  {
                      memcpy(buf, &image[(h-row-1)*linebytes], linebytes);    // check the
                  index here, and figure out why not using h*linebytes
                      if (TIFFWriteScanline(out, buf, row, 0) < 0)
                      break;
                  }

         Finally we close the output file, and destroy the buffer 

                  (void) TIFFClose(out); 

                  if (buf)
                      _TIFFfree(buf);
  */
}

void
ImageFileFormatTIFF::write (std::ostream & stream, const Image & image) const
{
  throw "Can't write TIFF on stream (limitation of libtiff and standard).";

  // Actually, we could write entire TIFF to a temporary file, and then dump
  // to stream.  Read could be modified to interpret the header and directory
  // blocks.  It could then estimate how many bytes to grab from stream.
}

bool
ImageFileFormatTIFF::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);

  if (magic.substr (0, 2) == "II"  &&  magic[2] == '\x2A'  &&  magic[3] == '\x00')
  {
	return true;
  }
  if (magic.substr (0, 2) == "MM"  &&  magic[2] == '\x00'  &&  magic[3] == '\x2A')
  {
	return true;
  }

  // Apparently, some implementation don't store the magic number 42 according
  // to the endian indicated by the first two bytes, so we have to handle the
  // wrong order as well.
  if (magic.substr (0, 2) == "II"  &&  magic[2] == '\x00'  &&  magic[3] == '\x2A')
  {
	return true;
  }
  if (magic.substr (0, 2) == "MM"  &&  magic[2] == '\x2A'  &&  magic[3] == '\x00')
  {
	return true;
  }

  return false;
}

bool
ImageFileFormatTIFF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "tiff") == 0)
  {
	return true;
  }
  if (strcasecmp (formatName.c_str (), "tif") == 0)
  {
	return true;
  }
  return false;
}
