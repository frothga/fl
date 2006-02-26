/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


05/2005 Fred Rothganger -- Use ios::binary for compatibility with Windows.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/image.h"


#include <fstream>


using namespace std;
using namespace fl;


// class ImageFile ------------------------------------------------------------

ImageFile::~ImageFile ()
{
}


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
   boundary between buffer loads.  Another possibility is to have
   ImageFileFormat::find() wrap the given stream in a special stream
   that can always put back at least 16 or so characters regardless
   of underlying state.
 **/
void
ImageFileFormat::getMagic (istream & stream, string & magic)
{
  int position = stream.tellg ();
  stream.read ((char *) magic.c_str (), magic.size ());
  stream.seekg (position);
}
