#include "fl/image.h"


#include <fstream>


using namespace std;
using namespace fl;


// class ImageFileFormat ------------------------------------------------------

vector<ImageFileFormat *> ImageFileFormat::formats;

ImageFileFormat::ImageFileFormat ()
{
  formats.push_back (this);
}

ImageFileFormat::~ImageFileFormat ()
{
  vector<ImageFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (*i == this)
	{
	  formats.erase (i);
	  break;
	}
  }
}

void
ImageFileFormat::read (const std::string & fileName, Image & image) const
{
  ifstream ifs (fileName.c_str ());
  read (ifs, image);
}

void
ImageFileFormat::write (const std::string & fileName, const Image & image) const
{
  ofstream ofs (fileName.c_str ());
  write (ofs, image);
}

ImageFileFormat *
ImageFileFormat::find (const std::string & fileName)
{
  ifstream ifs (fileName.c_str ());
  ImageFileFormat * result = find (ifs);
  if (! result)
  {
	string suffix = fileName.substr (fileName.find_last_of ('.') + 1);
	result = findName (suffix);
  }
  return result;
}

ImageFileFormat *
ImageFileFormat::find (std::istream & stream)
{
  vector<ImageFileFormat *>::reverse_iterator i;
  for (i = formats.rbegin (); i != formats.rend (); i++)
  {
	if ((*i)->isIn (stream))
	{
	  return *i;
	}
  }

  return NULL;
}

ImageFileFormat *
ImageFileFormat::findName (const std::string & formatName)
{
  vector<ImageFileFormat *>::reverse_iterator i;
  for (i = formats.rbegin (); i != formats.rend (); i++)
  {
	if ((*i)->handles (formatName))
	{
	  return *i;
	}
  }

  return NULL;
}

/**
   \todo Currently there is no guarantee that the stream can actually
   rewind to the position at the beginning of the magic string.
   Some streams can go bad at this point because they don't support
   seekg().  One possibility is to use sputbackc() to return magic
   to the stream.  This could still fail if magic straddles the
   boundary between buffer loads.  Another possibility is to have
   ImageFileFormat::find() wrap the given stream in a special stream
   that can always put back at least 16 or so characters regardless
   of underlying state.
 **/
void
ImageFileFormat::getMagic (std::istream & stream, std::string & magic)
{
  int position = stream.tellg ();
  stream.read ((char *) magic.c_str (), magic.size ());
  stream.seekg (position);
}
