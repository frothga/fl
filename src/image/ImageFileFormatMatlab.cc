/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2 and 1.4   Copyright 2005 Sandia Corporation.
Revisions 1.6 thru 1.13 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.13  2007/03/23 02:32:04  Fred
Use CVS Log to generate revision history.

Revision 1.12  2006/03/02 03:28:12  Fred
Create new class ImageFileDelegate to do the actual work of the image codec. 
Make ImageFile a tool for accessing image files, including metadata and image
contents.  ImageFileDelegate is now like a strategy object which implements the
specifics, while ImageFile presents a uniform inteface to the programmer.

Revision 1.11  2006/02/28 04:25:45  Fred
No longer necessary to include <fstream>

Revision 1.10  2006/02/27 03:31:19  Fred
Get rid of ImageFileFormat::open() for files, and change interface of open()
for streams to optionally specify that the ImageFile should take responsibility
for destroying the stream.

Revision 1.9  2006/02/27 00:17:24  Fred
Expand ImageFile::read() and write() to allow specification of coordinates in a
larger raster.  These optional parameters support big image processing.

Revision 1.8  2006/02/26 03:09:12  Fred
Create a new class called ImageFile which does the actual work of reading or
writing Images, and separate it from ImageFileFormat.  The job of
ImageFileFormat is now just to reify the format and act as a factory to
ImageFiles.  The purpose of this change is to move toward supporting big
images, which require a file to be open over the lifespan of the Image.

Revision 1.7  2006/02/26 00:14:10  Fred
Switch to probabilistic selection of image file format.

Revision 1.6  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.5  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/09 04:13:14  Fred
Add detail to revision history.

Revision 1.3  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/22 21:21:32  Fred
MSVC compilability fix:  Use fl/string.h to avoid function name discrepancy.

Revision 1.1  2003/12/30 21:10:33  rothgang
Create new file format for Matlab matrices.
-------------------------------------------------------------------------------
*/


#include "fl/image.h"
#include "fl/string.h"


using namespace std;
using namespace fl;


// Supporting routine ---------------------------------------------------------

static void
parseType (int type, int & numericType)
{
  if (type >= 10000  ||  type < 0)
  {
	throw "Type number out of range";
  }

  int n = type / 1000;
  if (n > 4)
  {
	throw "Machine id out of range";
  }
  type %= 1000;

  n = type / 100;
  if (n)
  {
	throw "Type number out of range";
  }
  type %= 100;

  numericType = type / 10;
  if (numericType > 5)
  {
	throw "Numeric type id out of range";
  }
  type %= 10;

  if (type > 2)
  {
	throw "Matrix type id out of range";
  }
}


// class ImageFileDelegateMatlab ----------------------------------------------

class ImageFileDelegateMatlab : public ImageFileDelegate
{
public:
  ImageFileDelegateMatlab (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFileDelegateMatlab ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFileDelegateMatlab::~ImageFileDelegateMatlab ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileDelegateMatlab::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileDelegateMatlab not open for reading";

  // Parse header...

  int type;
  int rows;
  int columns;
  int imaginaryFlag;
  int nameLength;

  in->read ((char *) &type, sizeof (type));
  in->read ((char *) &rows, sizeof (rows));
  in->read ((char *) &columns, sizeof (columns));
  in->read ((char *) &imaginaryFlag, sizeof (imaginaryFlag));
  in->read ((char *) &nameLength, sizeof (nameLength));

  const int maxNameLength = 2000;
  if (! in->good ()  ||  nameLength > maxNameLength)
  {
	throw "Can't finish reading Matlab file becasue stream is bad.";
  }
  char name[maxNameLength];
  in->read (name, nameLength);

  if (imaginaryFlag)
  {
	throw "Currently there are no complex pixel formats.";
  }

  int numericType;
  parseType (type, numericType);
  switch (numericType)
  {
    case 0:
	  image.format = &GrayDouble;
	  break;
    case 1:
    case 2:
	  image.format = &GrayFloat;
	  break;
	case 4:
	  image.format = &GrayShort;
	  break;
    case 5:
	  image.format = &GrayChar;
	  break;
    default:
	  throw "No image format equivalent to numeric type.";
  }

  // Read data...
  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
  image.resize (columns, rows);
  for (int x = 0; x < columns; x++)
  {
	char * p = (char *) buffer->memory;
	p += x * image.format->depth;
	for (int y = 0; y < rows; y++)
	{
	  in->read (p, image.format->depth);
	  p += columns * image.format->depth;
	}
  }

  // Special numeric conversions.  This is a hack to avoid implementing
  // specialty PixelFormats.
  if (numericType == 2)  // Convert int to float
  {
	float * i = (float *) buffer->memory;
	int * j = (int *) i;
	float * end = i + image.width * image.height;
	while (i < end)
	{
	  *i++ = *j++;
	}
  }
}

void
ImageFileDelegateMatlab::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateMatlab not open for writing";

  int numericType;
  if (*image.format == GrayChar)
  {
	numericType = 5;
  }
  else if (*image.format == GrayShort)
  {
	numericType = 4;
  }
  else if (*image.format == GrayFloat)
  {
	numericType = 1;
  }
  else if (*image.format == GrayDouble)
  {
	numericType = 0;
  }
  else
  {
	Image temp = image * GrayDouble;
	write (temp);
	return;
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "Matlab format only handle packed buffers for now";

  // Write header ...

  numericType *= 10;  // Recycle "numericType" as "type".  All other digits in type field should be zero.
  out->write ((char *) &numericType, sizeof (numericType));

  out->write ((char *) &image.height, sizeof (image.height));
  out->write ((char *) &image.width, sizeof (image.width));
  int imaginaryFlag = 0;
  out->write ((char *) &imaginaryFlag, sizeof (imaginaryFlag));

  string bogusName = "bogusName";
  int nameLength = bogusName.size () + 1;
  out->write ((char *) &nameLength, sizeof (nameLength));
  out->write (bogusName.c_str (), nameLength);

  // Write data...
  for (int x = 0; x < image.width; x++)
  {
	char * p = (char *) buffer->memory;
	p += x * image.format->depth;
	for (int y = 0; y < image.height; y++)
	{
	  out->write (p, image.format->depth);
	  p += buffer->stride * image.format->depth;
	}
  }
}


// class ImageFileFormatMatlab ------------------------------------------------

ImageFileDelegate *
ImageFileFormatMatlab::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegateMatlab (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatMatlab::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateMatlab (0, &stream, ownStream);
}

float
ImageFileFormatMatlab::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces = 4 bytes = 1 int
  getMagic (stream, magic);

  int type = *((int *) magic.c_str ());
  int numericType;
  try
  {
	parseType (type, numericType);
  }
  catch (const char * error)
  {
	return 0;
  }

  return 1;
}

float
ImageFileFormatMatlab::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "mat") == 0)
  {
	return 0.8;
  }
  if (strcasecmp (formatName.c_str (), "matlab") == 0)
  {
	return 1;
  }
  return 0;
}
