#include "fl/image.h"

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


// class ImageFileFormatJPEG --------------------------------------------------

void
ImageFileFormatJPEG::read (std::istream & stream, Image & image) const
{
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
  sm->stream = &stream;
  cinfo.src = (jpeg_source_mgr *) sm;

  jpeg_read_header (&cinfo, TRUE);

  jpeg_start_decompress (&cinfo);

  // Should do something more sophisticated here, like handle different color
  // spaces or different number of components.
  if (cinfo.output_components == 1)
  {
	image.format = &GrayChar;
  }
  else
  {
	image.format = &BGRChar;
  }
  image.resize (cinfo.output_width, cinfo.output_height);

  while (cinfo.output_scanline < cinfo.output_height)
  {
	JSAMPROW row[1];
	row[0] = (JSAMPLE *) image (0, cinfo.output_scanline).pixel;
    jpeg_read_scanlines (&cinfo, row, 1);
  }
  jpeg_finish_decompress (&cinfo);

  jpeg_destroy_decompress (&cinfo);
  delete sm;
}

void
ImageFileFormatJPEG::write (std::ostream & stream, const Image & image) const
{
  if (*image.format != BGRChar)
  {
	Image temp = image * BGRChar;
	write (stream, temp);
	return;
  }

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  DestinationManager * dm = new DestinationManager;
  dm->jdm.init_destination = init_destination;
  dm->jdm.empty_output_buffer = empty_output_buffer;
  dm->jdm.term_destination = term_destination;
  dm->stream = &stream;
  cinfo.dest = (jpeg_destination_mgr *) dm;

  cinfo.image_width      = image.width;
  cinfo.image_height     = image.height;
  cinfo.input_components = 3;
  cinfo.in_color_space   = JCS_RGB;
  jpeg_set_defaults (&cinfo);
  //jpeg_set_quality (&cinfo, quality, TRUE);

  jpeg_start_compress (&cinfo, TRUE);
  while (cinfo.next_scanline < cinfo.image_height)
  {
	JSAMPROW row[1];
    row[0] = (JSAMPLE *) image (0, cinfo.next_scanline).pixel;
    jpeg_write_scanlines (&cinfo, row, 1);
  }
  jpeg_finish_compress (&cinfo);

  jpeg_destroy_compress (&cinfo);
  delete dm;
}

bool
ImageFileFormatJPEG::isIn (std::istream & stream) const
{
  // JFIF header: OxFF 0xD8 0xFF 0xE0 <skip 2 bytes> "JFIF" 0x00
  // EXIF header: OxFF 0xD8 0xFF 0xE1 <skip 2 bytes> "Exif" 0x00
  string magic = "          ";  // 10 spaces
  getMagic (stream, magic);
  if (magic.substr (0, 4) == "\xFF\xD8\xFF\xE0"  &&  magic.substr (6, 4) == "JFIF")
  {
	return true;
  }
  if (magic.substr (0, 4) == "\xFF\xD8\xFF\xE1"  &&  magic.substr (6, 4) == "Exif")
  {
	return true;
  }
  return false;
}

bool
ImageFileFormatJPEG::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "jpg") == 0)
  {
	return true;
  }
  if (strcasecmp (formatName.c_str (), "jpeg") == 0)
  {
	return true;
  }
  return false;
}
