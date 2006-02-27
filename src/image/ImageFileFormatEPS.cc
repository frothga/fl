/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
02/2006 Fred Rothganger -- Change Image structure.  Separate ImageFile.
*/


#include "fl/image.h"
#include "fl/string.h"

#include <fstream>


using namespace std;
using namespace fl;


// class ImageFileEPS ---------------------------------------------------------

class ImageFileEPS : public ImageFile
{
public:
  ImageFileEPS (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFileEPS ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFileEPS::~ImageFileEPS ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileEPS::read (Image & image, int x, int y, int width, int height)
{
  throw "There's no way we are going to read an EPS!";

  // We may consider detecting and extracting a preview
}

void
ImageFileEPS::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileEPS not open for writing";

  if (*image.format != GrayChar)
  {
	write (image * GrayChar);
	return;
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "EPS only supports packed buffers for now.";

  // Determine aspect ratio
  const float vunits = 9.0 * 72;
  const float hunits = 6.5 * 72;
  float vscale = vunits / image.height;
  float hscale = hunits / image.width;
  float scale = min (vscale, hscale);
  float v = image.height * scale;
  float h = image.width * scale;

  // Header
  (*out) << "%!PS-Adobe-2.0" << endl;
  (*out) << "%%BoundingBox: 72 72 " << (h + 72) << " " << (v + 72) << " " << endl;
  (*out) << "%%EndComments" << endl;
  (*out) << endl;
  (*out) << "72 72 translate" << endl;
  (*out) << h << " " << v << " scale" << endl;
  (*out) << "/grays 1000 string def" << endl;
  (*out) << image.width << " " << image.height << " 8" << endl;
  (*out) << "[" << image.width << " 0 0 " << -image.height << " 0 " << image.height << "]" << endl;
  (*out) << "{ currentfile grays readhexstring pop } image" << endl;

  // Dump image as hex
  (*out) << hex;
  unsigned char * begin = (unsigned char *) buffer->memory;
  int end = image.width * image.height;
  for (int i = 0; i < end; i++)
  {
	if (i % 35 == 0)
	{
	  (*out) << endl;
	}
	(*out) << (int) begin[i];
  }
  (*out) << endl;

  // Trailer
  (*out) << "%%Trailer" << endl;
  (*out) << "%%EOF" << endl;
}


// class ImageFileFormatEPS ---------------------------------------------------

ImageFile *
ImageFileFormatEPS::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileEPS (&stream, 0, ownStream);
}

ImageFile *
ImageFileFormatEPS::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileEPS (0, &stream, ownStream);
}

float
ImageFileFormatEPS::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);
  if (magic == "%!PS")
  {
	return 1;
  }
  return 0;
}

float
ImageFileFormatEPS::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "eps") == 0)
  {
	return 0.8;
  }
  if (strcasecmp (formatName.c_str (), "ps") == 0)
  {
	return 0.7;
  }
  if (strcasecmp (formatName.c_str (), "epsf") == 0)
  {
	return 0.8;
  }
  return 0;
}
