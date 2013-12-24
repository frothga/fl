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


#include "fl/video.h"


using namespace std;
using namespace fl;


// class VideoIn --------------------------------------------------------------

VideoIn::VideoIn (const string & fileName)
{
  file = 0;
  open (fileName);
}

VideoIn::~VideoIn ()
{
  close ();
}

void
VideoIn::open (const string & fileName)
{
  close ();
  VideoFileFormat * format = VideoFileFormat::find (fileName);
  if (format) file = format->openInput (fileName);
}

void
VideoIn::close ()
{
  delete file;
  file = 0;
}

void
VideoIn::seekFrame (int frame)
{
  if (file) file->seekFrame (frame);
}

void
VideoIn::seekTime (double timestamp)
{
  if (file) file->seekTime (timestamp);
}

VideoIn &
VideoIn::operator >> (Image & image)
{
  if (file) file->readNext (image);
  return *this;
}

bool
VideoIn::good () const
{
  return file  &&  file->good ();
}

void
VideoIn::setTimestampMode (bool frames)
{
  if (file) file->setTimestampMode (frames);
}

void
VideoIn::get (const string & name, string & value)
{
  if (file) file->get (name, value);
}

void
VideoIn::set (const string & name, const string & value)
{
  if (file) file->set (name, value);
}


// class VideoOut -------------------------------------------------------------

VideoOut::VideoOut (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  file = 0;
  VideoFileFormat * format = VideoFileFormat::find (formatName, codecName);
  if (format) file = format->openOutput (fileName, formatName, codecName);
}

VideoOut::~VideoOut ()
{
  delete file;
}

VideoOut &
VideoOut::operator << (const Image & image)
{
  if (file) file->writeNext (image);
  return *this;
}

bool
VideoOut::good () const
{
  return file  &&  file->good ();
}

void
VideoOut::get (const string & name, string & value)
{
  if (file) file->get (name, value);
}

void
VideoOut::set (const string & name, const string & value)
{
  if (file) file->set (name, value);
}


// class VideoInFile ----------------------------------------------------------

VideoInFile::~VideoInFile ()
{
}


// class VideoOutFile ---------------------------------------------------------

VideoOutFile::~VideoOutFile ()
{
}


// class VideoFileFormat ------------------------------------------------------

vector<VideoFileFormat *> VideoFileFormat::formats;

VideoFileFormat::~VideoFileFormat ()
{
  vector<VideoFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (*i == this)
	{
	  formats.erase (i);
	  break;
	}
  }
}

VideoFileFormat *
VideoFileFormat::find (const std::string & fileName)
{
  VideoFileFormat * result = NULL;
  float bestProbability = 0;
  vector<VideoFileFormat *>::reverse_iterator i;
  for (i = formats.rbegin (); i != formats.rend (); i++)
  {
	float p = (*i)->isIn (fileName);
	if (p > bestProbability)
	{
	  result = *i;
	  bestProbability = p;
	}
  }

  return result;
}

VideoFileFormat *
VideoFileFormat::find (const std::string & formatName, const std::string & codecName)
{
  VideoFileFormat * result = NULL;
  float bestProbability = 0;
  vector<VideoFileFormat *>::reverse_iterator i;
  for (i = formats.rbegin (); i != formats.rend (); i++)
  {
	float p = (*i)->handles (formatName, codecName);
	if (p > bestProbability)
	{
	  result = *i;
	  bestProbability = p;
	}
  }

  return result;
}
