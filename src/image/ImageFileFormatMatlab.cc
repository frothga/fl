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


// class ImageFileMatlab ------------------------------------------------------

class ImageFileMatlab : public ImageFile
{
public:
  ImageFileMatlab (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFileMatlab ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFileMatlab::~ImageFileMatlab ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileMatlab::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileMatlab not open for reading";

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
ImageFileMatlab::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileMatlab not open for writing";

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

ImageFile *
ImageFileFormatMatlab::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileMatlab (&stream, 0, ownStream);
}

ImageFile *
ImageFileFormatMatlab::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileMatlab (0, &stream, ownStream);
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
