/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
05/2005 Fred Rothganger -- Use new PixelFormat names.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


02/2006 Fred Rothganger -- Change Image structure.
*/


#include "fl/image.h"
#include "fl/string.h"


using namespace std;
using namespace fl;


// class ImageFileFormatPGM ---------------------------------------------------

void
ImageFileFormatPGM::read (std::istream & stream, Image & image) const
{
  // Parse header...

  int gotParm = 0;
  int width = 0;
  int height = 0;
  while (stream.good ()  &&  gotParm < 4)
  {
	// Eat all leading white space and comments
	bool comment = false;
	while (stream.good ())
	{
	  int c = stream.get ();
	  if (c == '\r'  ||  c == '\n')
	  {
		comment = false;
	  }
	  else if (c == ' '  ||  c == '\t')
	  {
		// do nothing
	  }
	  else  // c is not whitespace
	  {
		if (c == '#')
		{
		  comment = true;
		}
		if (! comment)
		{
		  stream.unget ();
		  break;
		}
	  }
	}

	// Build up one token
	string token;
	while (stream.good ()  &&  token.size () < 70)
	{
	  int c = stream.get ();
	  if (c == '\n'  ||  c == '\r'  ||  c == ' '  ||  c == '\t')  // whitespace ends token
	  {
		stream.unget ();
		break;
	  }
	  token += c;
	}

	// Interpret token
	switch (gotParm++)
	{
	  case 0:
		if (token == "P5")
		{
		  image.format = &GrayChar;
		}
		else if (token == "P6")
		{
		  image.format = &RGBChar;
		}
		else
		{
		  throw "Unrecognized PGM format";
		}
		break;
	  case 1:
		width = atoi (token.c_str ());
		break;
	  case 2:
		height = atoi (token.c_str ());
	}
  }

  // Assuming P5 or P6, the above parser leaves one unconsumed whitespace
  // before the data block.
  stream.get ();

  // Read data
  if (! stream.good ())
  {
	throw "Unable to finish reading image: stream bad.";
  }
  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
  image.resize (width, height);
  stream.read ((char *) buffer->memory, width * height * image.format->depth);
}

void
ImageFileFormatPGM::write (std::ostream & stream, const Image & image) const
{
  if (image.format->monochrome)
  {
	if (*image.format != GrayChar)
	{
	  write (stream, image * GrayChar);
	  return;
	}
  }
  else  // color format
  {
	if (*image.format != RGBChar)
	{
	  write (stream, image * RGBChar);
	  return;
	}
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "PGM can only handle packed buffers for now";

  if (*image.format == GrayChar)
  {
	stream << "P5" << endl;
  }
  else  // RGBChar
  {
	stream << "P6" << endl;
  }
  stream << image.width << " " << image.height << " 255" << endl;
  stream.write ((char *) buffer->memory, image.width * image.height * image.format->depth);
}

float
ImageFileFormatPGM::isIn (std::istream & stream) const
{
  string magic = "  ";  // 2 spaces
  getMagic (stream, magic);
  if (magic == "P5")
  {
	return 0.8;
  }
  if (magic == "P6")
  {
	return 0.8;
  }
  return 0;
}

float
ImageFileFormatPGM::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "pgm") == 0)
  {
	return 0.9;
  }
  if (strcasecmp (formatName.c_str (), "ppm") == 0)
  {
	return 0.9;
  }
  return 0;
}
