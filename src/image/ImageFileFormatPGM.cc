/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3, 1.5 and 1.6 Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.17    Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.17  2007/08/13 02:55:06  Fred
Make sure rowBytes is exact.

Revision 1.16  2007/08/13 00:15:39  Fred
Use stride directly for byte size of rows.  Handle depth as a float value.

Revision 1.15  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.14  2006/03/02 03:26:48  Fred
Create new class ImageFileDelegate to do the actual work of the image codec. 
Make ImageFile a tool for accessing image files, including metadata and image
contents.  ImageFileDelegate is now like a strategy object which implements the
specifics, while ImageFile presents a uniform inteface to the programmer.

Revision 1.13  2006/02/28 04:25:45  Fred
No longer necessary to include <fstream>

Revision 1.12  2006/02/27 03:31:19  Fred
Get rid of ImageFileFormat::open() for files, and change interface of open()
for streams to optionally specify that the ImageFile should take responsibility
for destroying the stream.

Revision 1.11  2006/02/27 00:17:24  Fred
Expand ImageFile::read() and write() to allow specification of coordinates in a
larger raster.  These optional parameters support big image processing.

Revision 1.10  2006/02/26 03:09:12  Fred
Create a new class called ImageFile which does the actual work of reading or
writing Images, and separate it from ImageFileFormat.  The job of
ImageFileFormat is now just to reify the format and act as a factory to
ImageFiles.  The purpose of this change is to move toward supporting big
images, which require a file to be open over the lifespan of the Image.

Revision 1.9  2006/02/26 00:14:10  Fred
Switch to probabilistic selection of image file format.

Revision 1.8  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 05:19:16  Fred
Update revision history and add Sandia copyright notice.

Revision 1.5  2005/05/01 21:04:23  Fred
Adjust to more rigorous naming of PixelFormats.  Take advantage of new RGBChar
format.

Revision 1.4  2005/04/23 19:36:45  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.3  2005/01/22 21:21:45  Fred
MSVC compilability fix:  Use fl/string.h to avoid function name discrepancy.

Revision 1.2  2004/05/03 19:05:29  rothgang
Guard against bad stream when reading.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/image.h"
#include "fl/string.h"


using namespace std;
using namespace fl;


// class ImageFileDelegatePGM -------------------------------------------------

class ImageFileDelegatePGM : public ImageFileDelegate
{
public:
  ImageFileDelegatePGM (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFileDelegatePGM ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFileDelegatePGM::~ImageFileDelegatePGM ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileDelegatePGM::read (Image & image, int x, int y, int ignorewidth, int ignoreheight)
{
  if (! in) throw "ImageFileDelegatePGM not open for reading";

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
  in->read ((char *) buffer->memory, buffer->stride * height);
}

void
ImageFileDelegatePGM::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegatePGM not open for writing";

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
  int rowBytes = image.width * (int) image.format->depth;
  if (buffer->stride == rowBytes)
  {
	out->write ((char *) buffer->memory, rowBytes * image.height);
  }
  else
  {
	char * row = (char *) buffer->memory;
	for (int y = 0; y < image.height; y++)
	{
	  out->write (row, rowBytes);
	  row += buffer->stride;
	}
  }
}


// class ImageFileFormatPGM ---------------------------------------------------

ImageFileDelegate *
ImageFileFormatPGM::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegatePGM (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatPGM::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegatePGM (0, &stream, ownStream);
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
