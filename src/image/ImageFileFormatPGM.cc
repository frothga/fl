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


02/2006 Fred Rothganger -- Change Image structure.  Separate ImageFile.
*/


#include "fl/image.h"
#include "fl/string.h"

#include <fstream>


using namespace std;
using namespace fl;


// class ImageFilePGM ---------------------------------------------------------

class ImageFilePGM : public ImageFile
{
public:
  ImageFilePGM (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFilePGM ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFilePGM::~ImageFilePGM ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFilePGM::read (Image & image, int x, int y, int ignorewidth, int ignoreheight)
{
  if (! in) throw "ImageFilePGM not open for reading";

  // Parse header...

  int gotParm = 0;
  int width = 0;
  int height = 0;
  while (in->good ()  &&  gotParm < 4)
  {
	// Eat all leading white space and comments
	bool comment = false;
	while (in->good ())
	{
	  int c = in->get ();
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
		  in->unget ();
		  break;
		}
	  }
	}

	// Build up one token
	string token;
	while (in->good ()  &&  token.size () < 70)
	{
	  int c = in->get ();
	  if (c == '\n'  ||  c == '\r'  ||  c == ' '  ||  c == '\t')  // whitespace ends token
	  {
		in->unget ();
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
  in->get ();

  // Read data
  if (! in->good ())
  {
	throw "Unable to finish reading image: stream bad.";
  }
  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
  image.resize (width, height);
  in->read ((char *) buffer->memory, width * height * image.format->depth);
}

void
ImageFilePGM::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFilePGM not open for writing";

  if (image.format->monochrome)
  {
	if (*image.format != GrayChar)
	{
	  write (image * GrayChar);
	  return;
	}
  }
  else  // color format
  {
	if (*image.format != RGBChar)
	{
	  write (image * RGBChar);
	  return;
	}
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "PGM can only handle packed buffers for now";

  if (*image.format == GrayChar)
  {
	(*out) << "P5" << endl;
  }
  else  // RGBChar
  {
	(*out) << "P6" << endl;
  }
  (*out) << image.width << " " << image.height << " 255" << endl;
  out->write ((char *) buffer->memory, image.width * image.height * image.format->depth);
}


// class ImageFileFormatPGM ---------------------------------------------------

ImageFile *
ImageFileFormatPGM::open (std::istream & stream, bool ownStream) const
{
  return new ImageFilePGM (&stream, 0, ownStream);
}

ImageFile *
ImageFileFormatPGM::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFilePGM (0, &stream, ownStream);
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
