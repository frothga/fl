#include "fl/image.h"
#include "fl/string.h"


using namespace std;
using namespace fl;


// class ImageFileFormatMatlab ------------------------------------------------

void
ImageFileFormatMatlab::read (std::istream & stream, Image & image) const
{
  // Parse header...

  int type;
  int rows;
  int columns;
  int imaginaryFlag;
  int nameLength;

  stream.read ((char *) &type, sizeof (type));
  stream.read ((char *) &rows, sizeof (rows));
  stream.read ((char *) &columns, sizeof (columns));
  stream.read ((char *) &imaginaryFlag, sizeof (imaginaryFlag));
  stream.read ((char *) &nameLength, sizeof (nameLength));

  const int maxNameLength = 2000;
  if (! stream.good ()  ||  nameLength > maxNameLength)
  {
	throw "Can't finish reading Matlab file becasue stream is bad.";
  }
  char name[maxNameLength];
  stream.read (name, nameLength);

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
    case 5:
	  image.format = &GrayChar;
	  break;
    default:
	  // Should create formats GrayInt, GrayShort and GrayShortUnsigned to
	  // handle remaining numeric formats.
	  throw "No image format equivalent to numeric type.";
  }

  // Read data...
  image.resize (columns, rows);
  for (int x = 0; x < columns; x++)
  {
	char * buffer = (char *) image.buffer;
	buffer += x * image.format->depth;
	for (int y = 0; y < rows; y++)
	{
	  stream.read (buffer, image.format->depth);
	  buffer += columns * image.format->depth;
	}
  }

  // Special numeric conversions.  This is a hack to avoid implementing
  // specialty PixelFormats.
  if (numericType == 2)  // Convert int to float
  {
	float * i = (float *) image.buffer;
	int * j = (int *) i;
	float * end = i + image.width * image.height;
	while (i < end)
	{
	  *i++ = *j++;
	}
  }
}

void
ImageFileFormatMatlab::write (std::ostream & stream, const Image & image) const
{
  // Write header ...

  int numericType;
  if (*image.format == GrayChar)
  {
	numericType = 5;
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
	Image temp = image * GrayChar;
	write (stream, temp);
	return;
  }
  numericType *= 10;  // Recycle "numericType" as "type".  All other digits in type field should be zero.
  stream.write ((char *) &numericType, sizeof (numericType));

  stream.write ((char *) &image.height, sizeof (image.height));
  stream.write ((char *) &image.width, sizeof (image.width));
  int imaginaryFlag = 0;
  stream.write ((char *) &imaginaryFlag, sizeof (imaginaryFlag));

  string bogusName = "bogusName";
  int nameLength = bogusName.size () + 1;
  stream.write ((char *) &nameLength, sizeof (nameLength));
  stream.write (bogusName.c_str (), nameLength);

  // Write data...
  for (int x = 0; x < image.width; x++)
  {
	char * buffer = (char *) image.buffer;
	buffer += x * image.format->depth;
	for (int y = 0; y < image.height; y++)
	{
	  stream.write (buffer, image.format->depth);
	  buffer += image.width * image.format->depth;
	}
  }
}

bool
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
	return false;
  }

  return true;
}

bool
ImageFileFormatMatlab::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "mat") == 0)
  {
	return true;
  }
  if (strcasecmp (formatName.c_str (), "matlab") == 0)
  {
	return true;
  }
  return false;
}

void
ImageFileFormatMatlab::parseType (int type, int & numericType) const
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
