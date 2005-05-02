/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
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
		image.width = atoi (token.c_str ());
		break;
	  case 2:
		image.height = atoi (token.c_str ());
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
  int size = image.width * image.height * image.format->depth;
  image.buffer.grow (size);
  stream.read ((char *) image.buffer, size);
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

  if (*image.format == GrayChar)
  {
	stream << "P5" << endl;
  }
  else  // RGBChar
  {
	stream << "P6" << endl;
  }
  stream << image.width << " " << image.height << " 255" << endl;
  stream.write ((char *) image.buffer, image.width * image.height * image.format->depth);
}

bool
ImageFileFormatPGM::isIn (std::istream & stream) const
{
  string magic = "  ";  // 2 spaces
  getMagic (stream, magic);
  if (magic == "P5")
  {
	return true;
  }
  if (magic == "P6")
  {
	return true;
  }
  return false;
}

bool
ImageFileFormatPGM::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "pgm") == 0)
  {
	return true;
  }
  if (strcasecmp (formatName.c_str (), "ppm") == 0)
  {
	return true;
  }
  return false;
}
