/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 thru 1.6  Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.16 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.16  2007/08/12 14:36:41  Fred
Use stride directly for byte size of rows.

Revision 1.15  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.14  2006/03/02 03:26:48  Fred
Create new class ImageFileDelegate to do the actual work of the image codec. 
Make ImageFile a tool for accessing image files, including metadata and image
contents.  ImageFileDelegate is now like a strategy object which implements the
specifics, while ImageFile presents a uniform inteface to the programmer.

Revision 1.13  2006/02/28 04:25:45  Fred
No longer necessary to include <fstream>

Revision 1.12  2006/02/27 03:31:19  Fred
Get rid of ImageFileFormat::open() for files, and change interface of open()
for streams to optionally specify that the ImageFile should take responsibility
for destroying the stream.

Revision 1.11  2006/02/27 00:17:24  Fred
Expand ImageFile::read() and write() to allow specification of coordinates in a
larger raster.  These optional parameters support big image processing.

Revision 1.10  2006/02/26 03:09:12  Fred
Create a new class called ImageFile which does the actual work of reading or
writing Images, and separate it from ImageFileFormat.  The job of
ImageFileFormat is now just to reify the format and act as a factory to
ImageFiles.  The purpose of this change is to move toward supporting big
images, which require a file to be open over the lifespan of the Image.

Revision 1.9  2006/02/26 00:14:10  Fred
Switch to probabilistic selection of image file format.

Revision 1.8  2006/02/25 22:38:31  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.7  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 05:18:47  Fred
Add revision history and Sandia copyright notice.

Revision 1.5  2005/05/29 15:54:52  Fred
Add includes to overcome MSVC compilation issues.

Revision 1.4  2005/05/01 21:03:34  Fred
Adjust to more rigorous naming of PixelFormats.

Revision 1.3  2005/04/23 19:39:05  Fred
Add UIUC copyright notice.

Revision 1.2  2003/12/29 23:31:38  rothgang
Fix severe error in buffer handling.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/image.h"
#include "fl/string.h"

extern "C"
{
  #include <jpeglib.h>
  #include <jmorecfg.h>
}


using namespace std;
using namespace fl;


// Error manager --------------------------------------------------------------

static void
output_message (j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  (*cinfo->err->format_message) (cinfo, buffer);

  //fprintf(stderr, "%s\n", buffer);
  cerr << buffer << endl;
}


// Destination manager --------------------------------------------------------

struct DestinationManager
{
  jpeg_destination_mgr jdm;
  ostream * stream;
  JOCTET buffer[4096];
};

static void
init_destination (j_compress_ptr cinfo)
{
  DestinationManager * dm = (DestinationManager *) cinfo->dest;
  dm->jdm.next_output_byte = dm->buffer;
  dm->jdm.free_in_buffer = sizeof (dm->buffer);
}

static boolean
empty_output_buffer (j_compress_ptr cinfo)
{
  DestinationManager * dm = (DestinationManager *) cinfo->dest;
  dm->stream->write ((char *) dm->buffer, sizeof (dm->buffer));
  dm->jdm.next_output_byte = dm->buffer;
  dm->jdm.free_in_buffer = sizeof (dm->buffer);
  return TRUE;
}

static void
term_destination (j_compress_ptr cinfo)
{
  DestinationManager * dm = (DestinationManager *) cinfo->dest;
  dm->stream->write ((char *) dm->buffer, sizeof (dm->buffer) - dm->jdm.free_in_buffer);
}


// Source manager -------------------------------------------------------------

struct SourceManager
{
  jpeg_source_mgr jsm;
  istream * stream;
  JOCTET buffer[4096];
};

static void
init_source (j_decompress_ptr cinfo)
{
  SourceManager * sm = (SourceManager *) cinfo->src;
  sm->jsm.next_input_byte = sm->buffer;
  sm->jsm.bytes_in_buffer = 0;
}

static boolean
fill_input_buffer (j_decompress_ptr cinfo)
{
  SourceManager * sm = (SourceManager *) cinfo->src;
  sm->stream->peek ();  // Force us to wait until data is available.
  sm->jsm.bytes_in_buffer = sm->stream->readsome ((char *) sm->buffer, sizeof (sm->buffer));
  sm->jsm.next_input_byte = sm->buffer;
  return TRUE;
}

static void
skip_input_data (j_decompress_ptr cinfo, long count)
{
  if (! count)
  {
	return;
  }

  SourceManager * sm = (SourceManager *) cinfo->src;
  while (sm->jsm.bytes_in_buffer < count)
  {
	count -= sm->jsm.bytes_in_buffer;
	fill_input_buffer (cinfo);
  }
  sm->jsm.next_input_byte += count;
  sm->jsm.bytes_in_buffer -= count;
}

static void
term_source (j_decompress_ptr cinfo)
{
  // do nothing
}


// class ImageFileDelegateJPEG ------------------------------------------------

class ImageFileDelegateJPEG : public ImageFileDelegate
{
public:
  ImageFileDelegateJPEG (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFileDelegateJPEG ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFileDelegateJPEG::~ImageFileDelegateJPEG ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileDelegateJPEG::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileDelegateJPEG not open for reading";

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error (&jerr);
  jerr.output_message = output_message;
  jpeg_create_decompress (&cinfo);

  SourceManager * sm = new SourceManager;
  sm->jsm.init_source = init_source;
  sm->jsm.fill_input_buffer = fill_input_buffer;
  sm->jsm.skip_input_data = skip_input_data;
  sm->jsm.resync_to_restart = jpeg_resync_to_restart;
  sm->jsm.term_source = term_source;
  sm->stream = in;
  cinfo.src = (jpeg_source_mgr *) sm;

  jpeg_read_header (&cinfo, TRUE);

  jpeg_start_decompress (&cinfo);

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;

  // Should do something more sophisticated here, like handle different color
  // spaces or different number of components.
  if (cinfo.output_components == 1)
  {
	image.format = &GrayChar;
  }
  else
  {
	image.format = &RGBChar;
  }
  image.resize (cinfo.output_width, cinfo.output_height);

  char * p = (char *) buffer->memory;
  JSAMPROW row[1];
  while (cinfo.output_scanline < cinfo.output_height)
  {
	row[0] = (JSAMPLE *) p;
    jpeg_read_scanlines (&cinfo, row, 1);
	p += buffer->stride;
  }
  jpeg_finish_decompress (&cinfo);

  jpeg_destroy_decompress (&cinfo);
  delete sm;
}

void
ImageFileDelegateJPEG::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateJPEG not open for writing";

  if (*image.format != RGBChar)
  {
	Image temp = image * RGBChar;
	write (temp);
	return;
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "JPEG only handles packed buffers for now.";

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  DestinationManager * dm = new DestinationManager;
  dm->jdm.init_destination = init_destination;
  dm->jdm.empty_output_buffer = empty_output_buffer;
  dm->jdm.term_destination = term_destination;
  dm->stream = out;
  cinfo.dest = (jpeg_destination_mgr *) dm;

  cinfo.image_width      = image.width;
  cinfo.image_height     = image.height;
  cinfo.input_components = 3;
  cinfo.in_color_space   = JCS_RGB;
  jpeg_set_defaults (&cinfo);
  //jpeg_set_quality (&cinfo, quality, TRUE);

  jpeg_start_compress (&cinfo, TRUE);
  char * p = (char *) buffer->memory;
  JSAMPROW row[1];
  while (cinfo.next_scanline < cinfo.image_height)
  {
    row[0] = (JSAMPLE *) p;
    jpeg_write_scanlines (&cinfo, row, 1);
	p += buffer->stride;
  }
  jpeg_finish_compress (&cinfo);

  jpeg_destroy_compress (&cinfo);
  delete dm;
}

// class ImageFileFormatJPEG --------------------------------------------------

ImageFileDelegate *
ImageFileFormatJPEG::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegateJPEG (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatJPEG::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateJPEG (0, &stream, ownStream);
}

float
ImageFileFormatJPEG::isIn (std::istream & stream) const
{
  // JFIF header: OxFF 0xD8 0xFF 0xE0 <skip 2 bytes> "JFIF" 0x00
  // EXIF header: OxFF 0xD8 0xFF 0xE1 <skip 2 bytes> "Exif" 0x00
  string magic = "          ";  // 10 spaces
  getMagic (stream, magic);
  if (magic.substr (0, 4) == "\xFF\xD8\xFF\xE0"  &&  magic.substr (6, 4) == "JFIF")
  {
	return 1;
  }
  if (magic.substr (0, 4) == "\xFF\xD8\xFF\xE1"  &&  magic.substr (6, 4) == "Exif")
  {
	return 1;
  }
  return 0;
}

float
ImageFileFormatJPEG::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "jpg") == 0)
  {
	return 0.8;
  }
  if (strcasecmp (formatName.c_str (), "jpeg") == 0)
  {
	return 1;
  }
  return 0;
}
