#include "fl/image.h"


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
		  image.format = &RGBAChar;
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
  if (*image.format == GrayChar)
  {
	image.buffer.grow (image.width * image.height);
	stream.read ((char *) image.buffer, image.width * image.height);
  }
  else if (*image.format == RGBAChar)
  {
	image.buffer.grow (image.width * image.height * RGBAChar.depth);
	for (int i = 0; i < image.width * image.height; i++)
	{
	  unsigned char c[3];
	  stream.read ((char *) c, 3);
	  ((unsigned int *) image.buffer)[i] = 0xFF000000 | (c[0] << 16) | (c[1] << 8) | c[2];
	}
  }
}

void
ImageFileFormatPGM::write (std::ostream & stream, const Image & image) const
{
  if (image.format->monochrome  &&  *image.format != GrayChar)
  {
	Image temp = image * GrayChar;
	write (stream, temp);
	return;
  }

  if (*image.format == GrayChar)
  {
	stream << "P5" << endl;
	stream << image.width << " " << image.height << " 255" << endl;
	stream.write ((char *) image.buffer, image.width * image.height);
  }
  else  // some color format
  {
	stream << "P6" << endl;
	stream << image.width << " " << image.height << " 255" << endl;
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		unsigned int rgb = image.getRGBA (x, y);
		unsigned char c[3];
		c[0] = (rgb & 0xFF0000) >> 16;
		c[1] = (rgb & 0xFF00) >> 8;
		c[2] =  rgb & 0xFF;
		stream.write ((char *) c, 3);
	  }
	}
  }
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
