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
  ImageFileDelegateJPEG (istream * in, ostream * out, bool ownStream = false);
  ~ImageFileDelegateJPEG ();

  void parseComments (jpeg_saved_marker_ptr marker);

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name, string & value);
  virtual void get (const string & name, int & value);
  virtual void get (const string & name, double & value);
  virtual void get (const string & name, Matrix<double> & value);

  virtual void set (const string & name, const string & value);
  virtual void set (const string & name, int value);
  virtual void set (const string & name, double value);
  virtual void set (const string & name, const Matrix<double> & value);

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

  if (in)
  {
	dinfo.err = jpeg_std_error (&jerr);
	jerr.output_message = output_message;
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
	cinfo.err = jpeg_std_error (&jerr);
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

void
ImageFileDelegateJPEG::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileDelegateJPEG not open for reading";

  jpeg_start_decompress (&dinfo);

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;

  // Should do something more sophisticated here, like handle different color
  // spaces or different number of components.
  if (dinfo.output_components == 1)
  {
	image.format = &GrayChar;
  }
  else
  {
	image.format = &RGBChar;
  }
  image.resize (dinfo.output_width, dinfo.output_height);

  // Read image
  char * p = (char *) buffer->memory;
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

  PixelFormat * format;
  if (image.format->monochrome) format = &GrayChar;
  else                          format = &RGBChar;
  Image work = image * *format;

  PixelBufferPacked * buffer = (PixelBufferPacked *) work.buffer;
  if (! buffer) throw "JPEG only handles packed buffers for now.";

  cinfo.image_width      = work.width;
  cinfo.image_height     = work.height;
  if (*format == GrayChar)
  {
	cinfo.input_components = 1;
	cinfo.in_color_space   = JCS_GRAYSCALE;
  }
  else
  {
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;
  }
  // It would also be possible to support YCbCr, but currently this is only a planar format.
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
  char * p = (char *) buffer->memory;
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
ImageFileDelegateJPEG::get (const string & name, int & value)
{
  Matrix<double> v;
  get (name, v);
  if (v.rows () > 0  &&  v.columns () > 0)
  {
	value = (int) roundp (v(0,0));
  }
}

void
ImageFileDelegateJPEG::get (const string & name, double & value)
{
  Matrix<double> v;
  get (name, v);
  if (v.rows () > 0  &&  v.columns () > 0)
  {
	value = v(0,0);
  }
}

void
ImageFileDelegateJPEG::get (const string & name, Matrix<double> & value)
{
  if (out)
  {
	if (name == "width"  ||  name == "blockWidth")
	{
	  value.resize (1, 1);
	  value(0,0) = dinfo.output_width;
	  return;
	}
	if (name == "height"  ||  name == "blockHeight")
	{
	  value.resize (1, 1);
	  value(0,0) = dinfo.output_height;
	  return;
	}
  }

  if (name == "quality")
  {
	value.resize (1,1);
	value(0,0) = quality;
	return;
  }

  if (name == "comments"  &&  comments.size ())
  {
	value.resize (1,1);
	value(0,0) = atol (comments.c_str ());
	return;
  }

  map<string, string>::iterator it = namedValues.find (name);
  if (it != namedValues.end ())
  {
	// Scan value to count rows and columns
	int rows = 0;
	int columns = 0;
	istringstream sv (it->second);
	while (sv.good ())
	{
	  string line;
	  getline (sv, line);
	  if (line.size ()) rows++;
	  if (! columns)
	  {
		trim (line);
		while (line.size ())
		{
		  columns++;
		  int pos = line.find_first_of (" \t");
		  if (pos == string::npos) break;
		  line = line.substr (pos);
		  trim (line);
		}
	  }
	}

	// Extract the matrix
	value.resize (rows, columns);
	value << it->second;

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

void
ImageFileDelegateJPEG::set (const string & name, int value)
{
  Matrix<double> v (1, 1);
  v(0,0) = value;
  set (name, v);
}

void
ImageFileDelegateJPEG::set (const string & name, double value)
{
  Matrix<double> v (1, 1);
  v(0,0) = value;
  set (name, v);
}

void
ImageFileDelegateJPEG::set (const string & name, const Matrix<double> & value)
{
  if (name == "quality")
  {
	quality = (int) roundp (value(0,0));
	return;
  }

  if (name == "comments")
  {
	ostringstream sv;
	sv << value;
	comments += sv.str ();
	return;
  }

  if (name.size ())
  {
	if (value.rows () > 0  &&  value.columns () > 0)
	{
	  ostringstream sv;
	  sv << value;
	  namedValues.insert (make_pair (name, sv.str ()));
	}
	else
	{
	  namedValues.erase (name);
	}
  }
  else if (value.rows () > 0  &&  value.columns () > 0)
  {
	ostringstream sv;
	sv << value;
	comments += sv.str ();
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
