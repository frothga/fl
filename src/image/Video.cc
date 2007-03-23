/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.6  Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/03/20 05:40:07  Fred
Get rid of hint.  Instead, follow the paradigm of returing an image with no
conversion.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 04:15:05  Fred
Add detail to revision history.

Revision 1.5  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.4  2005/01/22 21:23:37  Fred
MSVC compilability fix:  Be explicit about namespace of PixelFormat, since
FFMPEG defines an enumeration also called PixelFormat.

Revision 1.3  2004/08/10 21:15:28  rothgang
Add ability to get attributes from input stream.  Use this to get duration from
FFMPEG.

Revision 1.2  2003/12/30 16:13:48  rothgang
Create timestamp mode, which switches between frame numbers and seconds for the
value of Image::timestamp.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/video.h"


using namespace std;
using namespace fl;


// class VideoIn ----------------------------------------------------------------

VideoIn::VideoIn (const string & fileName)
{
  file = 0;
  VideoFileFormat * format = VideoFileFormat::find (fileName);
  if (format)
  {
	file = format->openInput (fileName);
  }
}

VideoIn::~VideoIn ()
{
  if (file)
  {
	delete file;
  }
}

void
VideoIn::seekFrame (int frame)
{
  if (file)
  {
	file->seekFrame (frame);
  }
}

void
VideoIn::seekTime (double timestamp)
{
  if (file)
  {
	file->seekTime (timestamp);
  }
}

VideoIn &
VideoIn::operator >> (Image & image)
{
  if (file)
  {
	file->readNext (image);
  }
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
  if (file)
  {
	file->setTimestampMode (frames);
  }
}

void
VideoIn::get (const string & name, double & value)
{
  if (file)
  {
	file->get (name, value);
  }
}


// class VideoOut -------------------------------------------------------------

VideoOut::VideoOut (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  file = 0;
  VideoFileFormat * format = VideoFileFormat::find (formatName, codecName);
  if (format)
  {
	file = format->openOutput (fileName, formatName, codecName);
  }
}

VideoOut::~VideoOut ()
{
  if (file)
  {
	delete file;
  }
}

VideoOut &
VideoOut::operator << (const Image & image)
{
  if (file)
  {
	file->writeNext (image);
  }
  return *this;
}

bool
VideoOut::good () const
{
  return file  &&  file->good ();
}

void
VideoOut::set (const string & name, double value)
{
  if (file)
  {
	file->set (name, value);
  }
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

VideoFileFormat::VideoFileFormat ()
{
  formats.push_back (this);
}

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
