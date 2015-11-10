/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/image.h"
#include "fl/math.h"


#include <fstream>
#include <stdio.h>
#include <sys/stat.h>


using namespace std;
using namespace fl;


// class ImageFileDelegate ------------------------------------------------------------

ImageFileDelegate::~ImageFileDelegate ()
{
}


// class ImageFile ------------------------------------------------------------

ImageFile::ImageFile ()
{
}

ImageFile::ImageFile (const string & fileName, const string & mode, const string & formatName)
{
  open (fileName, mode, formatName);
}

ImageFile::ImageFile (istream & stream)
{
  open (stream);
}

ImageFile::ImageFile (ostream & stream, const string & formatName)
{
  open (stream, formatName);
}

void
ImageFile::open (const std::string & fileName, const string & mode, const string & formatName)
{
  if (mode == "w")  // write
  {
	string suffix = formatName;
	if (! suffix.size ())
	{
	  suffix = fileName.substr (fileName.find_last_of ('.') + 1);
	}

	ImageFileFormat * ff;
	float P = ImageFileFormat::findName (suffix, ff);
	if (P == 0.0f  ||  ! ff) throw "Unrecognized file format for image.";
	delegate = ff->open (*(new ofstream (fileName.c_str (), ios::binary)), true);
	timestamp = NAN;
  }
  else  // read
  {
	ImageFileFormat * ff;
	float P = ImageFileFormat::find (fileName, ff);
	if (P == 0.0f  ||  ! ff) throw "Unrecognized file format for image.";
	delegate = ff->open (*(new ifstream (fileName.c_str (), ios::binary)), true);

	// Use stat () to determine timestamp.
	struct stat info;
	stat (fileName.c_str (), &info);
	timestamp = info.st_mtime;  // Does this need more work to align it with getTimestamp () values?
  }
}

void
ImageFile::open (istream & stream)
{
  if (! stream.good ()) throw "Can't read image due to bad stream";

  ImageFileFormat * ff;
  float P = ImageFileFormat::find (stream, ff);
  if (P == 0.0f  ||  ! ff) throw "Unrecognized file format for image.";
  delegate = ff->open (stream);
  timestamp = NAN;
}

void
ImageFile::open (ostream & stream, const string & formatName)
{
  ImageFileFormat * ff;
  float P = ImageFileFormat::findName (formatName, ff);
  if (P == 0.0f  ||  ! ff) throw "Unrecognized file format for image.";
  delegate = ff->open (stream);
  timestamp = NAN;
}

void
ImageFile::close ()
{
  delegate.detach ();
}

void
ImageFile::read (Image & image, int x, int y, int width, int height)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->read (image, x, y, width, height);
  if (! std::isnan (timestamp)) image.timestamp = timestamp;
}

void
ImageFile::write (const Image & image, int x, int y)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->write (image, x, y);
}

void
ImageFile::get (const std::string & name, std::string & value)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->get (name, value);
}

void
ImageFile::set (const std::string & name, const std::string & value)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->set (name, value);
}


// class ImageFileFormat ------------------------------------------------------

vector<ImageFileFormat *> ImageFileFormat::formats;

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

float
ImageFileFormat::find (const string & fileName, ImageFileFormat *& result)
{
  ifstream ifs (fileName.c_str (), ios::binary);
  string suffix = fileName.substr (fileName.find_last_of ('.') + 1);

  float P = 0;
  result = 0;
  vector<ImageFileFormat *>::iterator it;
  for (it = formats.begin (); it != formats.end (); it++)
  {
	// It might be better to combine isIn() and handles() in a single function
	// that does its own mixing.
	float q1 = (*it)->isIn (ifs);
	float q2 = (*it)->handles (suffix);
	float q = (q1 + q2) / 2.0f;
	if (q >= P)
	{
	  P = q;
	  result = *it;
	}
  }

  return P;
}

float
ImageFileFormat::find (istream & stream, ImageFileFormat *& result)
{
  float P = 0;
  result = 0;
  vector<ImageFileFormat *>::iterator it;
  for (it = formats.begin (); it != formats.end (); it++)
  {
	float q = (*it)->isIn (stream);
	if (q >= P)
	{
	  P = q;
	  result = *it;
	}
  }

  return P;
}

float
ImageFileFormat::findName (const string & formatName, ImageFileFormat *& result)
{
  float P = 0;
  result = 0;
  vector<ImageFileFormat *>::iterator it;
  for (it = formats.begin (); it != formats.end (); it++)
  {
	float q = (*it)->handles (formatName);
	if (q >= P)
	{
	  P = q;
	  result = *it;
	}
  }

  return P;
}

/**
   \todo Currently there is no guarantee that the stream can actually
   rewind to the position at the beginning of the magic string.
   Some streams can go bad at this point because they don't support
   seekg().  One possibility is to use sputbackc() to return magic
   to the stream.  This could still fail if magic straddles the
   boundary between buffer loads.
 **/
void
ImageFileFormat::getMagic (istream & stream, string & magic)
{
  int position = stream.tellg ();
  stream.read ((char *) magic.c_str (), magic.size ());
  stream.seekg (position);
}
