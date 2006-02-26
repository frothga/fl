/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
02/2006 Fred Rothganger -- Change Image structure.
*/


#include "fl/image.h"
#include "fl/string.h"


using namespace std;
using namespace fl;


// class ImageFileFormatEPS ---------------------------------------------------

void
ImageFileFormatEPS::read (std::istream & stream, Image & image) const
{
  throw "There's no way we are going to read an EPS!";

  // We may consider detecting and extracting a preview
}

void
ImageFileFormatEPS::write (std::ostream & stream, const Image & image) const
{
  if (*image.format != GrayChar)
  {
	Image temp = image * GrayChar;
	write (stream, temp);
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
  stream << "%!PS-Adobe-2.0" << endl;
  stream << "%%BoundingBox: 72 72 " << (h + 72) << " " << (v + 72) << " " << endl;
  stream << "%%EndComments" << endl;
  stream << endl;
  stream << "72 72 translate" << endl;
  stream << h << " " << v << " scale" << endl;
  stream << "/grays 1000 string def" << endl;
  stream << image.width << " " << image.height << " 8" << endl;
  stream << "[" << image.width << " 0 0 " << -image.height << " 0 " << image.height << "]" << endl;
  stream << "{ currentfile grays readhexstring pop } image" << endl;

  // Dump image as hex
  stream << hex;
  unsigned char * begin = (unsigned char *) buffer->memory;
  int end = image.width * image.height;
  for (int i = 0; i < end; i++)
  {
	if (i % 35 == 0)
	{
	  stream << endl;
	}
	stream << (int) begin[i];
  }
  stream << endl;

  // Trailer
  stream << "%%Trailer" << endl;
  stream << "%%EOF" << endl;
}

bool
ImageFileFormatEPS::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);
  if (magic == "%!PS")
  {
	return true;
  }
  return false;
}

bool
ImageFileFormatEPS::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "eps") == 0)
  {
	return true;
  }
  if (strcasecmp (formatName.c_str (), "ps") == 0)
  {
	return true;
  }
  if (strcasecmp (formatName.c_str (), "epsf") == 0)
  {
	return true;
  }
  return false;
}
