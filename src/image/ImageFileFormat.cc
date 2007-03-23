/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 and 1.6   Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.15 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.15  2007/03/23 02:32:05  Fred
Use CVS Log to generate revision history.

Revision 1.14  2006/03/16 03:19:45  Fred
Add get(int) and set(int).  Rearrange order of functions for prettiness sake.

Revision 1.13  2006/03/05 03:11:07  Fred
Add open() and close() methods, along with a new constructor that does not open
a file.

Revision 1.12  2006/03/03 17:44:51  Fred
Add Matrix<double> as a metadata type.

Revision 1.11  2006/03/02 03:27:56  Fred
Create new class ImageFileDelegate to do the actual work of the image codec. 
Make ImageFile a tool for accessing image files, including metadata and image
contents.  ImageFileDelegate is now like a strategy object which implements the
specifics, while ImageFile presents a uniform inteface to the programmer.

Fix the comments on getMagic(), since there's no way the strategy described
there will work.

Revision 1.10  2006/02/27 00:17:24  Fred
Expand ImageFile::read() and write() to allow specification of coordinates in a
larger raster.  These optional parameters support big image processing.

Revision 1.9  2006/02/26 03:09:12  Fred
Create a new class called ImageFile which does the actual work of reading or
writing Images, and separate it from ImageFileFormat.  The job of
ImageFileFormat is now just to reify the format and act as a factory to
ImageFiles.  The purpose of this change is to move toward supporting big
images, which require a file to be open over the lifespan of the Image.

Revision 1.8  2006/02/26 00:14:10  Fred
Switch to probabilistic selection of image file format.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 05:18:23  Fred
Add revision history and Sandia copyright notice.

Revision 1.5  2005/05/01 21:02:00  Fred
Add ios::binary flag to file stream opens, to adapt to quirks of MS Windows
environment.

Revision 1.4  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/05/03 19:05:03  rothgang
Note todo about rewinding streams when probing for file type.

Revision 1.2  2003/12/29 23:31:07  rothgang
If we can't identify image type by the file content, fall back to using suffix.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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

void
ImageFileDelegate::get (const std::string & name, std::string & value)
{
}

void
ImageFileDelegate::get (const std::string & name, int & value)
{
}

void
ImageFileDelegate::get (const std::string & name, double & value)
{
}

void
ImageFileDelegate::get (const std::string & name, Matrix<double> & value)
{
}

void
ImageFileDelegate::set (const std::string & name, const std::string & value)
{
}

void
ImageFileDelegate::set (const std::string & name, int value)
{
}

void
ImageFileDelegate::set (const std::string & name, double value)
{
}

void
ImageFileDelegate::set (const std::string & name, const Matrix<double> & value)
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
  if (! isnan (timestamp)) image.timestamp = timestamp;
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
ImageFile::get (const std::string & name, int & value)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->get (name, value);
}

void
ImageFile::get (const std::string & name, double & value)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->get (name, value);
}

void
ImageFile::get (const std::string & name, Matrix<double> & value)
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

void
ImageFile::set (const std::string & name, int value)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->set (name, value);
}

void
ImageFile::set (const std::string & name, double value)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->set (name, value);
}

void
ImageFile::set (const std::string & name, const Matrix<double> & value)
{
  if (! delegate.memory) throw "ImageFile not open";
  delegate->set (name, value);
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
   boundary between buffer loads.
 **/
void
ImageFileFormat::getMagic (istream & stream, string & magic)
{
  int position = stream.tellg ();
  stream.read ((char *) magic.c_str (), magic.size ());
  stream.seekg (position);
}
