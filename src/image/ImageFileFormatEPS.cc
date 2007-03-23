/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2 and 1.4   Copyright 2005 Sandia Corporation.
Revisions 1.6 thru 1.13 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.13  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.12  2006/03/02 03:26:48  Fred
Create new class ImageFileDelegate to do the actual work of the image codec. 
Make ImageFile a tool for accessing image files, including metadata and image
contents.  ImageFileDelegate is now like a strategy object which implements the
specifics, while ImageFile presents a uniform inteface to the programmer.

Revision 1.11  2006/02/28 04:25:45  Fred
No longer necessary to include <fstream>

Revision 1.10  2006/02/27 03:31:19  Fred
Get rid of ImageFileFormat::open() for files, and change interface of open()
for streams to optionally specify that the ImageFile should take responsibility
for destroying the stream.

Revision 1.9  2006/02/27 00:17:24  Fred
Expand ImageFile::read() and write() to allow specification of coordinates in a
larger raster.  These optional parameters support big image processing.

Revision 1.8  2006/02/26 03:09:12  Fred
Create a new class called ImageFile which does the actual work of reading or
writing Images, and separate it from ImageFileFormat.  The job of
ImageFileFormat is now just to reify the format and act as a factory to
ImageFiles.  The purpose of this change is to move toward supporting big
images, which require a file to be open over the lifespan of the Image.

Revision 1.7  2006/02/26 00:14:10  Fred
Switch to probabilistic selection of image file format.

Revision 1.6  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.5  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/09 04:12:11  Fred
Add detail to revision history.

Revision 1.3  2005/04/23 19:36:45  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/22 21:21:20  Fred
MSVC compilability fix:  Use fl/string.h to avoid function name discrepancy.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/image.h"
#include "fl/string.h"


using namespace std;
using namespace fl;


// class ImageFileDelegateEPS -------------------------------------------------

class ImageFileDelegateEPS : public ImageFileDelegate
{
public:
  ImageFileDelegateEPS (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFileDelegateEPS ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFileDelegateEPS::~ImageFileDelegateEPS ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileDelegateEPS::read (Image & image, int x, int y, int width, int height)
{
  throw "There's no way we are going to read an EPS!";

  // We may consider detecting and extracting a preview
}

void
ImageFileDelegateEPS::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateEPS not open for writing";

  if (*image.format != GrayChar)
  {
	write (image * GrayChar);
	return;
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "EPS only supports packed buffers for now.";

  // Determine aspect ratio
  const float vunits = 9.0 * 72;
  const float hunits = 6.5 * 72;
  float vscale = vunits / image.height;
  float hscale = hunits / image.width;
  float scale = min (vscale, hscale);
  float v = image.height * scale;
  float h = image.width * scale;

  // Header
  (*out) << "%!PS-Adobe-2.0" << endl;
  (*out) << "%%BoundingBox: 72 72 " << (h + 72) << " " << (v + 72) << " " << endl;
  (*out) << "%%EndComments" << endl;
  (*out) << endl;
  (*out) << "72 72 translate" << endl;
  (*out) << h << " " << v << " scale" << endl;
  (*out) << "/grays 1000 string def" << endl;
  (*out) << image.width << " " << image.height << " 8" << endl;
  (*out) << "[" << image.width << " 0 0 " << -image.height << " 0 " << image.height << "]" << endl;
  (*out) << "{ currentfile grays readhexstring pop } image" << endl;

  // Dump image as hex
  (*out) << hex;
  unsigned char * begin = (unsigned char *) buffer->memory;
  int end = image.width * image.height;
  for (int i = 0; i < end; i++)
  {
	if (i % 35 == 0)
	{
	  (*out) << endl;
	}
	(*out) << (int) begin[i];
  }
  (*out) << endl;

  // Trailer
  (*out) << "%%Trailer" << endl;
  (*out) << "%%EOF" << endl;
}


// class ImageFileFormatEPS ---------------------------------------------------

ImageFileDelegate *
ImageFileFormatEPS::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegateEPS (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatEPS::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateEPS (0, &stream, ownStream);
}

float
ImageFileFormatEPS::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);
  if (magic == "%!PS")
  {
	return 1;
  }
  return 0;
}

float
ImageFileFormatEPS::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "eps") == 0)
  {
	return 0.8;
  }
  if (strcasecmp (formatName.c_str (), "ps") == 0)
  {
	return 0.7;
  }
  if (strcasecmp (formatName.c_str (), "epsf") == 0)
  {
	return 0.8;
  }
  return 0;
}
