/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


05/2005 Fred Rothganger -- Use new PixelFormat names.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


02/2006 Fred Rothganger -- Change Image structure.  Separate ImageFile.
*/


#include "fl/image.h"
#include "fl/string.h"

#include <fstream>

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


// class ImageFileJPEG --------------------------------------------------------

class ImageFileJPEG : public ImageFile
{
public:
  ImageFileJPEG (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;
  }
  ~ImageFileJPEG ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  istream * in;
  ostream * out;
  bool ownStream;
};

ImageFileJPEG::~ImageFileJPEG ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
}

void
ImageFileJPEG::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileJPEG not open for reading";

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
  int stride = buffer->stride * image.format->depth;
  JSAMPROW row[1];
  while (cinfo.output_scanline < cinfo.output_height)
  {
	row[0] = (JSAMPLE *) p;
    jpeg_read_scanlines (&cinfo, row, 1);
	p += stride;
  }
  jpeg_finish_decompress (&cinfo);

  jpeg_destroy_decompress (&cinfo);
  delete sm;
}

void
ImageFileJPEG::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileJPEG not open for writing";

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
  int stride = buffer->stride * image.format->depth;
  JSAMPROW row[1];
  while (cinfo.next_scanline < cinfo.image_height)
  {
    row[0] = (JSAMPLE *) p;
    jpeg_write_scanlines (&cinfo, row, 1);
	p += stride;
  }
  jpeg_finish_compress (&cinfo);

  jpeg_destroy_compress (&cinfo);
  delete dm;
}

// class ImageFileFormatJPEG --------------------------------------------------

ImageFile *
ImageFileFormatJPEG::open (const string & fileName, const string & mode) const
{
  if (mode == "r")
  {
	return new ImageFileJPEG (new ifstream (fileName.c_str (), ios::binary), 0, true);
  }
  else
  {
	return new ImageFileJPEG (0, new ofstream (fileName.c_str (), ios::binary), true);
  }
}

ImageFile *
ImageFileFormatJPEG::open (std::istream & stream) const
{
  return new ImageFileJPEG (&stream, 0);
}

ImageFile *
ImageFileFormatJPEG::open (std::ostream & stream) const
{
  return new ImageFileJPEG (0, &stream);
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
