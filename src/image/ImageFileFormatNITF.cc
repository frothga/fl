/*
Author: Fred Rothganger
Created 2/26/2006
*/


#include "fl/image.h"
#include "fl/string.h"


using namespace std;
using namespace fl;


// NITF structure -------------------------------------------------------------

class nitfLUT
{
public:
  nitfLUT (int NELUT)
  {
	this->NELUT = NELUT;
	table = new unsigned char[NELUT];
  }

  ~nitfLUT ()
  {
	delete table;
  }

  void read (istream & stream)
  {
	stream.read ((char *) table, NELUT);
  }

  int NELUT;
  unsigned char * table;
};

class nitfImageMask
{
public:
};

class nitfImageBand
{
public:
  ~nitfImageBand ()
  {
	for (int i = 0; i < luts.size (); i++)
	{
	  delete luts[i];
	}
  }

  void get (const string & name, string & value)
  {
	if (name == "NLUTS")
	{
	  value.assign (&required[12], 5);
	}
  }

  void get (const string & name, int & value)
  {
	string sv;
	get (name, sv);
	value = atoi (sv.c_str ());
  }

  void read (istream & stream)
  {
	stream.read (required, sizeof (required));

	int NLUTS;
	get ("NLUTS", NLUTS);
	int NELUT = 0;
	char buffer[10];
	if (NLUTS)
	{
	  stream.read (buffer, 5);
	  buffer[5] = 0;
	  NELUT = atoi (buffer);
	}
	for (int i = 0; i < NLUTS; i++)
	{
	  nitfLUT * l = new nitfLUT (NELUT);
	  luts.push_back (l);
	  l->read (stream);
	}
  }

  char required[17];
  vector<nitfLUT *> luts;
};

class nitfImageHeader
{
public:
  ~nitfImageHeader ()
  {
	for (int i = 0; i < bands.size (); i++)
	{
	  delete bands[i];
	}
  }

  void get (const string & name, string & value)
  {
	if (name == "IM")
	{
	  value.assign (&required[0], 2);
	}
	else if (name == "ICORDS")
	{
	  value.assign (&required[371], 1);
	}
	else if (name == "IREP")
	{
	  value.assign (&required[352], 8);
	}
	else if (name == "NROWS")
	{
	  value.assign (&required[333], 8);
	}
	else if (name == "NCOLS")
	{
	  value.assign (&required[341], 8);
	}
	else if (name == "PVTYPE")
	{
	  value.assign (&required[349], 3);
	}
	else if (name == "NBPP")
	{
	  value.assign (&required2[18], 2);
	}
	else if (name == "ABPP")
	{
	  value.assign (&required[368], 2);
	}
  }

  void get (const string & name, int & value)
  {
	string sv;
	get (name, sv);
	value = atoi (sv.c_str ());
  }

  void read (istream & stream, Image & image, int x, int y, int width, int height)
  {
	int NROWS;
	get ("NROWS", NROWS);
	int NCOLS;
	get ("NCOLS", NCOLS);

	if (IC[0] == 'N'  &&  IC[1] == 'C')
	{
	  PixelFormat * format = 0;

	  string IREP;
	  get ("IREP", IREP);
	  int NBPP;
	  get ("NBPP", NBPP);
	  string PVTYPE;
	  get ("PVTYPE", PVTYPE);

	  int ABPP;
	  get ("ABPP", ABPP);
	  cerr << "NBPP" << NBPP << endl;
	  cerr << "ABPP" << ABPP << endl;

	  if (IREP == "MONO    ")
	  {
		if (PVTYPE == "INT");
		{
		  switch (ABPP)  // should be NBPP!!!!!!!
		  {
			case 8:
			  format = &GrayChar;
			  break;
			case 16:
			  format = &GrayShort;
			  break;
		  }
		}
	  }

	  if (! format) throw "Can't match format";

	  image.format = format;
	  image.resize (NROWS, NCOLS);
	  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;

	  stream.seekg (offset + LISH);
	  stream.read ((char *) buffer->memory, NROWS * NCOLS * format->depth);
	}
  }

  void read (istream & stream)
  {
	offset = stream.tellg ();
	stream.read (required, sizeof (required));

	string IM;
	get ("IM", IM);
	if (IM != "IM") throw "Not an image subheader";

	string ICORDS;
	get ("ICORDS", ICORDS);
	if (ICORDS != " ")
	{
	  stream.read (IGEOLO, sizeof (IGEOLO));
	}

	char buffer[80];
	stream.read (buffer, 1);
	buffer[1] = 0;
	int NICOM = atoi (buffer);
	string line;
	for (int i = 0; i < NICOM; i++)
	{
	  stream.read (buffer, sizeof (buffer));
	  line.assign (buffer, sizeof (buffer));
	  trim (line);  // this will also remove leading spaces; maybe a bad idea
	  if (i) comments += "\n";
	  comments += line;
	}

	stream.read (IC, sizeof (IC));
	if (   (IC[0] == 'I'  &&  IC[1] == '1')
		|| (   (IC[0] == 'C'  ||  IC[0] == 'M')
			&& (IC[1] == '1'  ||  IC[1] == '3'  ||  IC[1] == '4'  ||  IC[1] == '5'  ||  IC[1] == '8')))
	{
	  stream.read (COMRAT, sizeof (COMRAT));
	}

	stream.read (buffer, 1);
	buffer[1] = 0;
	if (buffer[0] == '0')
	{
	  stream.read (buffer, 5);
	  buffer[5] = 0;
	}
	NBANDS = atoi (buffer);

	for (int i = 0; i < NBANDS; i++)
	{
	  nitfImageBand * b = new nitfImageBand;
	  bands.push_back (b);
	  b->read (stream);
	}

	stream.read (required2, sizeof (required2));
  }

  int LISH;
  int LI;
  int offset;
  char required[372];
  char IGEOLO[60];
  string comments;
  char IC[2];
  char COMRAT[4];
  int NBANDS;
  vector<nitfImageBand *> bands;
  char required2[45];
};

class nitfFileHeader
{
public:
  ~nitfFileHeader ()
  {
	for (int i = 0; i < images.size (); i++)
	{
	  delete images[i];
	}
  }

  void get (const string & name, string & value)
  {
	if (name == "HL")
	{
	  value.assign (&required[354], 6);
	}
	else if (name == "NUMI")
	{
	  value.assign (&required[360], 3);
	}
  }

  void get (const string & name, int & value)
  {
	string sv;
	get (name, sv);
	value = atoi (sv.c_str ());
  }

  void read (istream & stream)
  {
	stream.read (required, sizeof (required));

	int HL;
	get ("HL", HL);

	int NUMI;
	get ("NUMI", NUMI);
	for (int i = 0; i < NUMI; i++)
	{
	  nitfImageHeader * h = new nitfImageHeader;
	  images.push_back (h);

	  char buffer[11];
	  stream.read (buffer, 6);
	  buffer[6] = 0;
	  h->LISH = atoi (buffer);
	  stream.read (buffer, 10);
	  buffer[10] = 0;
	  h->LI = atoi (buffer);
	}

	// After parsing all the file header info, parse each segment header
	int offset = HL;
	for (int i = 0; i < images.size (); i++)
	{
	  nitfImageHeader * h = images[i];

	  stream.seekg (offset);
	  h->read (stream);
	  offset += h->LISH + h->LI;
	}
  }

  void write (ostream & stream)
  {
  }

  char required[363];  ///< Initial portion of the header that must be read in before recursive descent parsing can start.
  vector<nitfImageHeader *> images;
};



// class ImageFileDelegateNITF ------------------------------------------------

class ImageFileDelegateNITF : public ImageFileDelegate
{
public:
  ImageFileDelegateNITF (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;

	header = new nitfFileHeader;
	if (in)
	{
	  header->read (*in);
	}
  }
  ~ImageFileDelegateNITF ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name, string & value);

  istream * in;
  ostream * out;
  bool ownStream;
  nitfFileHeader * header;
};

ImageFileDelegateNITF::~ImageFileDelegateNITF ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
  if (header) delete header;
}

void
ImageFileDelegateNITF::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileDelegateNITF not open for reading";
  if (! header->images.size ()) throw "No image to read";

  header->images[0]->read (*in, image, x, y, width, height);
}

void
ImageFileDelegateNITF::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateNITF not open for writing";
}

void
ImageFileDelegateNITF::get (const string & name, string & value)
{
  if (! header) return;
  header->get (name, value);
}


// class ImageFileFormatNITF --------------------------------------------------

ImageFileDelegate *
ImageFileFormatNITF::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegateNITF (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatNITF::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateNITF (0, &stream, ownStream);
}

float
ImageFileFormatNITF::isIn (std::istream & stream) const
{
  string magic = "         ";  // 9 spaces
  getMagic (stream, magic);
  if (magic == "NITF02.10")  // that's all we handle for now
  {
	return 1;
  }
  return 0;
}

float
ImageFileFormatNITF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "nitf") == 0)
  {
	return 1;
  }
  if (strcasecmp (formatName.c_str (), "ntf") == 0)
  {
	return 0.9;
  }
  return 0;
}
