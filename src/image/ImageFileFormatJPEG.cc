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
#include "fl/string.h"
#include "fl/parms.h"

extern "C"
{
#  ifdef _MSC_VER
#    define XMD_H   // Hack to trick jmorecfg.h into not messing with INT32
#  endif
#  include <jpeglib.h>
}

#include <sstream>
#include <typeinfo>


using namespace std;
using namespace fl;


// Error manager --------------------------------------------------------------

static void
output_message (j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message) (cinfo, buffer);
  fprintf (stderr, "%s\n", buffer);  // better to use printf than cerr, because printf is thread-safe (avoids intermixed characters in stderr)
}

static void
error_exit (j_common_ptr cinfo)
{
  output_message (cinfo);
  throw "Fatal error in JPEG";
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
  ImageFileDelegateJPEG (istream * in, ostream * out, bool ownStream = false);
  ~ImageFileDelegateJPEG ();

  void parseComments (jpeg_saved_marker_ptr marker);

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name,       string & value);
  virtual void set (const string & name, const string & value);

  istream * in;
  ostream * out;
  bool ownStream;

  // It is a little wasteful of space to hold all the structures for both
  // read and write.  However, it shouldn't waste any time.
  struct jpeg_decompress_struct dinfo;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  SourceManager sm;
  DestinationManager dm;

  int quality;  ///< Value in [0,100] that guides compression level.
  map<string, string> namedValues;
  string comments;  ///< A concatenation of any JPEG_COM data read which does not fit the name=value form.
};

ImageFileDelegateJPEG::ImageFileDelegateJPEG (istream * in, ostream * out, bool ownStream)
{
  this->in = in;
  this->out = out;
  this->ownStream = ownStream;
  quality = 75;

  jpeg_std_error (&jerr);
  jerr.output_message = output_message;
  jerr.error_exit     = error_exit;
  
  if (in)
  {
	dinfo.err = &jerr;
	jpeg_create_decompress (&dinfo);

	sm.jsm.init_source       = init_source;
	sm.jsm.fill_input_buffer = fill_input_buffer;
	sm.jsm.skip_input_data   = skip_input_data;
	sm.jsm.resync_to_restart = jpeg_resync_to_restart;
	sm.jsm.term_source       = term_source;
	sm.stream = in;
	dinfo.src = (jpeg_source_mgr *) &sm;

	jpeg_save_markers (&dinfo, JPEG_COM, 0xFFFF);
	jpeg_read_header (&dinfo, TRUE);
	jpeg_calc_output_dimensions (&dinfo);

	parseComments (dinfo.marker_list);  // Parse any comments that arrived with header
  }

  if (out)
  {
	cinfo.err = &jerr;
	jpeg_create_compress (&cinfo);

	dm.jdm.init_destination    = init_destination;
	dm.jdm.empty_output_buffer = empty_output_buffer;
	dm.jdm.term_destination    = term_destination;
	dm.stream = out;
	cinfo.dest = (jpeg_destination_mgr *) &dm;
  }
}

ImageFileDelegateJPEG::~ImageFileDelegateJPEG ()
{
  if (in)  jpeg_destroy_decompress (&dinfo);
  if (out) jpeg_destroy_compress   (&cinfo);

  if (ownStream)
  {
	if (in)  delete in;
	if (out) delete out;
  }
}

void
ImageFileDelegateJPEG::parseComments (jpeg_saved_marker_ptr marker)
{
  namedValues.clear ();
  comments.clear ();

  while (marker)
  {
	string temp ((char *) marker->data, marker->data_length);

	string name;
	string value;
	split (temp, "=", name, value);
	trim (name);
	trim (value);
	if (value.size ())
	{
	  namedValues.insert (make_pair (name, value));
	}
	else
	{
	  comments = comments + name;
	}

	marker = marker->next;
  }
}

struct FormatMapping
{
  PixelFormat * format;   // Which pre-fab format to use. A value of zero ends the format map.
  J_COLOR_SPACE colorspace;
  int           components;
};

static FormatMapping formatMap[] =
{
  {&GrayChar,         JCS_GRAYSCALE, 1},
  {&RGBChar,          JCS_RGB,       3},
  //{&YUV,              JCS_YCbCr,     3},
  {&CMYK,             JCS_CMYK,      4},
  //{&YUVK,             JCS_YCCK,      4},
  {&RGBChar,          JCS_EXT_RGB,   3},
  //{&RGBChar4,         JCS_EXT_RGBX,  4},
  {&BGRChar,          JCS_EXT_BGR,   3},
  {&BGRChar4,         JCS_EXT_BGRX,  4},
  //{&BGRChar4,         JCS_EXT_XBGR,  4},  // and offset base of buffer by 1 byte
  //{&RGBChar4,         JCS_EXT_XRGB,  4},  // ditto
  {&RGBAChar,         JCS_EXT_RGBA,  4},
  {&BGRAChar,         JCS_EXT_BGRA,  4},
  //{&ABGRChar,         JCS_EXT_ABGR,  4},
  //{&ARGBChar,         JCS_EXT_ARGB,  4},
  //{&R5G6B5,           JCS_RGB565,    3},
  {0}
};

void
ImageFileDelegateJPEG::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileDelegateJPEG not open for reading";

  jpeg_start_decompress (&dinfo);

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;

  // Select pixel format
  PixelFormat * format = 0;
  for (FormatMapping * m = formatMap; m->format; m++)
  {
	if (m->colorspace == dinfo.out_color_space)
	{
	  format = m->format;
	  break;
	}
  }
  if (format == 0) throw "No PixelFormat available that matches file contents";
  image.format = format;
  image.resize (dinfo.output_width, dinfo.output_height);

  // Read image
  char * p = (char *) buffer->base ();
  JSAMPROW row[1];
  while (dinfo.output_scanline < dinfo.output_height)
  {
	row[0] = (JSAMPLE *) p;
	jpeg_read_scanlines (&dinfo, row, 1);
	p += buffer->stride;
  }

  // Re-parse comments in case some arrived with image data
  parseComments (dinfo.marker_list);

  jpeg_finish_decompress (&dinfo);
}

void
ImageFileDelegateJPEG::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateJPEG not open for writing";

  PixelFormat * format = 0;
  for (FormatMapping * m = formatMap; m->format; m++)
  {
	if (*m->format == *image.format)
	{
	  format = m->format;
	  cinfo.input_components = m->components;
	  cinfo.in_color_space   = m->colorspace;
	  break;
	}
  }
  if (format == 0)
  {
	if (image.format->monochrome)
	{
	  format = &GrayChar;
	  cinfo.input_components = 1;
	  cinfo.in_color_space   = JCS_GRAYSCALE;
	}
	else
	{
	  format = &RGBChar;
	  cinfo.input_components = 3;
	  cinfo.in_color_space   = JCS_RGB;
	}
  }
  Image work = image * *format;

  PixelBufferPacked * buffer = (PixelBufferPacked *) work.buffer;
  if (! buffer) throw "JPEG only handles packed buffers for now.";

  cinfo.image_width  = work.width;
  cinfo.image_height = work.height;
  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);

  jpeg_start_compress (&cinfo, TRUE);

  // Write comments
  map<string, string>::iterator it;
  for (it = namedValues.begin (); it != namedValues.end (); it++)
  {
	string namedValue = it->first + "=" + it->second;
	jpeg_write_marker (&cinfo, JPEG_COM, (JOCTET *) namedValue.c_str (), namedValue.size ());  // stored text does not inlcude null terminator!
  }
  if (comments.size ()) jpeg_write_marker (&cinfo, JPEG_COM, (JOCTET *) comments.c_str (), comments.size ());

  // Write image data
  char * p = (char *) buffer->base ();
  JSAMPROW row[1];
  while (cinfo.next_scanline < cinfo.image_height)
  {
	row[0] = (JSAMPLE *) p;
	jpeg_write_scanlines (&cinfo, row, 1);
	p += buffer->stride;
  }

  jpeg_finish_compress (&cinfo);
}

void
ImageFileDelegateJPEG::get (const string & name, string & value)
{
  if (out)
  {
	if (name == "width"  ||  name == "blockWidth")
	{
	  ostringstream sv;
	  sv << dinfo.output_width;
	  value = sv.str ();
	  return;
	}
	if (name == "height"  ||  name == "blockHeight")
	{
	  ostringstream sv;
	  sv << dinfo.output_height;
	  value = sv.str ();
	  return;
	}
  }

  if (name == "quality")
  {
	ostringstream sv;
	sv << quality;
	value = sv.str ();
	return;
  }

  if (name == "comments"  &&  comments.size ())
  {
	value = comments;
	return;
  }

  map<string, string>::iterator it = namedValues.find (name);
  if (it != namedValues.end ())
  {
	value = it->second;
	return;
  }
}

void
ImageFileDelegateJPEG::set (const string & name, const string & value)
{
  if (name == "quality")
  {
	quality = atoi (value.c_str ());
	return;
  }

  if (name == "comments")
  {
	comments = value;
	return;
  }

  if (name.size ())
  {
	if (value.size ())
	{
	  namedValues.insert (make_pair (name, value));
	}
	else
	{
	  namedValues.erase (name);
	}
  }
  else if (value.size ())
  {
	comments = comments + value;
  }
}


// class ImageFileFormatJPEG --------------------------------------------------

void
ImageFileFormatJPEG::use ()
{
  vector<ImageFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (typeid (**i) == typeid (ImageFileFormatJPEG)) return;
  }
  formats.push_back (new ImageFileFormatJPEG);
}

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
  if (magic.substr (0, 3) == "\xFF\xD8\xFF")
  {
	if (magic[3] == '\xE0'  &&  magic.substr (6, 4) == "JFIF") return 1;
	if (magic[3] == '\xE1'  &&  magic.substr (6, 4) == "Exif") return 1;
	if (magic[3] >= 0xE0  &&  magic[3] <= 0xEF) return 0.8;
  }
  return 0;
}

float
ImageFileFormatJPEG::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "jif" ) == 0) return 0.7;
  if (strcasecmp (formatName.c_str (), "jfif") == 0) return 0.7;
  if (strcasecmp (formatName.c_str (), "jpg" ) == 0) return 0.8;
  if (strcasecmp (formatName.c_str (), "jpeg") == 0) return 0.8;
  return 0;
}
