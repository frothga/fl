#include "fl/video.h"


// For debugging only
//#include "fl/slideshow.h"


using namespace std;
using namespace fl;


// class VideoInFileFFMPEG ----------------------------------------------------

VideoInFileFFMPEG::VideoInFileFFMPEG (const std::string & fileName, const PixelFormat & hint)
{
  fc = 0;
  cc = 0;
  packet.size = 0;

  this->hint = &hint;
  this->fileName = fileName;

  open (fileName);
}

VideoInFileFFMPEG::~VideoInFileFFMPEG ()
{
  close ();
}

void
VideoInFileFFMPEG::seekFrame (int frame)
{
  if (frame < this->frame)
  {
	close ();
	open (fileName);
  }

  while (this->frame < frame)
  {
	readNext (NULL);
	if (! gotPicture)
	{
	  return;
	}
	this->frame++;
	gotPicture = 0;
  }
}

void
VideoInFileFFMPEG::seekTime (double timestamp)
{
  if (stream)
  {
	seekFrame ((int) ceil (timestamp * stream->r_frame_rate / stream->r_frame_rate_base));
  }
}

void
VideoInFileFFMPEG::readNext (Image & image)
{
  readNext (&image);
}

void
VideoInFileFFMPEG::readNext (Image * image)
{
  if (state)
  {
	return;  // Don't attempt to read when we are in an error state.
  }

  while (! gotPicture)
  {
	while (size > 0)
	{
      int used = avcodec_decode_video (cc, &picture, &gotPicture, data, size);
      if (used < 0)
	  {
		state = used;
		return;
	  }
      size -= used;
      data += used;

      if (gotPicture)
	  {
		break;
	  }
	}

	if (size <= 0)
	{
	  if (packet.size)
	  {
		av_free_packet (&packet);
	  }

	  state = av_read_packet (fc, &packet);
	  if (state < 0)
	  {
		break;
	  }
	  else
	  {
		size = packet.size;
		data = packet.data;
		state = 0;
	  }
	}
  }

  if (gotPicture  &&  image)
  {
	extractImage (*image);
  }
}

bool
VideoInFileFFMPEG::good () const
{
  return ! state;
}

void
VideoInFileFFMPEG::open (const string & fileName)
{
  size = 0;
  memset (&picture, 0, sizeof (picture));
  frame = 0;
  gotPicture = 0;

  state = av_open_input_file (&fc, fileName.c_str (), NULL, 0, NULL);
  if (state < 0)
  {
	return;
  }
    
  state = av_find_stream_info (fc);
  if (state < 0)
  {
	return;
  }

  stream = NULL;
  for (int i = 0; i < fc->nb_streams; i++)
  {
	if (fc->streams[i]->codec.codec_type == CODEC_TYPE_VIDEO)
	{
	  stream = fc->streams[i];
	  break;
	}
  }
  if (! stream)
  {
	state = -10;
	return;
  }
  cc = &stream->codec;

  if (cc->codec_id == CODEC_ID_MPEG1VIDEO)
  {
	cc->flags |= CODEC_FLAG_TRUNCATED;
  }
  if (hint->monochrome)
  {
	cc->flags |= CODEC_FLAG_GRAY;
  }

  codec = avcodec_find_decoder (cc->codec_id);
  if (! codec)
  {
	state = -11;
	return;
  }

  state = avcodec_open (cc, codec);
  if (state < 0)
  {
	return;
  }

  state = 0;
}

void
VideoInFileFFMPEG::close ()
{
  if (packet.size)
  {
	av_free_packet (&packet);
	packet.size = 0;
  }
  if (cc  &&  cc->codec)
  {
	avcodec_close (cc);
	cc = 0;
  }
  if (fc)
  {
	av_close_input_file (fc);
	fc = 0;
  }

  state = -12;
}

// This is really the job of PixelFormat, so create one for YUV.
// Also consider enabling Image to handle planar storage.
inline unsigned int
yuv2rgb (const float Y, const float U, const float V)
{
  unsigned int r = (unsigned int) (max (min (1.0f, (Y                + 1.40200f * V)), 0.0f) * 255);
  unsigned int g = (unsigned int) (max (min (1.0f, (Y - 0.34414f * U - 0.71414f * V)), 0.0f) * 255);
  unsigned int b = (unsigned int) (max (min (1.0f, (Y + 1.77200f * U               )), 0.0f) * 255);
  return (r << 16) | (g << 8) | b;
}

void
VideoInFileFFMPEG::extractImage (Image & image)
{
  if (cc->pix_fmt == PIX_FMT_YUV420P)
  {
	if (hint->monochrome  ||  cc->flags & CODEC_FLAG_GRAY)
	{
	  image.format = &GrayChar;
	  image.resize (cc->width, cc->height);
	  for (int y = 0; y < cc->height; y++)
	  {
		memcpy (((unsigned char *) image.buffer) + y * cc->width, picture.data[0] + y * picture.linesize[0], cc->width);
	  }
	}
	else
	{
	  image.format = &YVYUChar;
	  image.resize (cc->width, cc->height);
	  ImageOf<unsigned int> that (image);
	  that.width /= 2;

	  for (int y = 0; y < cc->height; y += 2)
	  {
		for (int x = 0; x < cc->width; x += 2)
		{
		  int hx = x / 2;
		  int hy = y / 2;

		  unsigned int U = picture.data[1][hy * picture.linesize[1] + hx];
		  unsigned int V = picture.data[2][hy * picture.linesize[2] + hx] << 16;

		  unsigned int Y00 = picture.data[0][ y    * picture.linesize[0] + x];
		  unsigned int Y01 = picture.data[0][(y+1) * picture.linesize[0] + x];
		  unsigned int Y10 = picture.data[0][ y    * picture.linesize[0] + x+1];
		  unsigned int Y11 = picture.data[0][(y+1) * picture.linesize[0] + x+1];

		  that (hx, y)   = (Y10 << 24) | V | (Y00 << 8) | U;
		  that (hx, y+1) = (Y11 << 24) | V | (Y01 << 8) | U;
		}
	  }
	}
  }
  else if (cc->pix_fmt == PIX_FMT_YUV422)
  {
	image.attach (picture.data[0], cc->width, cc->height, YVYUChar);
  }
  else
  {
	throw "Unknown pixel format";
  }

  // picture.display_picture_number does not provide reliable information
  image.timestamp = (double) frame++ * stream->r_frame_rate_base / stream->r_frame_rate;
  gotPicture = 0;
}


// class VideoOutFileFFMPEG ---------------------------------------------------

VideoOutFileFFMPEG::VideoOutFileFFMPEG (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  stream = 0;
  codec = 0;
  needHeader = true;

  fc = (AVFormatContext *) av_mallocz (sizeof (AVFormatContext));
  if (! fc)
  {
	state = -10;
  }

  const char * formatAddress = formatName.size () ? formatName.c_str () : 0;
  fc->oformat = guess_format
  (
    formatAddress,
	fileName.c_str (),
	formatAddress
  );
  if (! fc->oformat)
  {
	state = -11;
	return;
  }

  AVFormatParameters parms;
  memset (&parms, 0, sizeof (parms));
  state = av_set_parameters (fc, &parms);
  if (state < 0)
  {
	return;
  }

  stream = (AVStream *) av_mallocz (sizeof (AVStream));
  if (! stream)
  {
	state = -10;
	return;
  }
  fc->nb_streams = 1;
  fc->streams[0] = stream;

  avcodec_get_context_defaults (& stream->codec);
  stream->codec.codec_id = fc->oformat->video_codec;
  /*
  if (fc->oformat->flags & AVFMT_RGB24)
  {
	stream->codec.pix_fmt = PIX_FMT_RGB24;
  }
  */

  state = url_fopen (& fc->pb, fileName.c_str (), URL_WRONLY);
  if (state < 0)
  {
	return;
  }

  // Add code here to search for named codec as well
  codec = avcodec_find_encoder (stream->codec.codec_id);
  if (! codec)
  {
	state = -12;
  }

  // Initialize other trivia in fc
  strcpy (fc->filename, fileName.c_str ());

  state = 0;
}

VideoOutFileFFMPEG::~VideoOutFileFFMPEG ()
{
  // Some members are zeroed here, even though this object is being destructed
  // and the members will never be touched again.  The rationale is that
  // this code could eventually be moved into a close() method, with a
  // corresponding open() method, so the the members may eventually be reused.

  if (! state  &&  fc)
  {
	// is it necessary to write header before it is safe to call av_write_trailer?
	av_write_trailer (fc);  // clears private data used by avformat
  }

  if (codec)
  {
	if (! needHeader)
	{
	  avcodec_close (& stream->codec);
	  needHeader = true;
	}
	codec = 0;
  }

  url_fclose (& fc->pb);

  if (stream)
  {
	if (stream->codec.stats_in)
	{
	  av_free (stream->codec.stats_in);
	}

	av_free (stream);
	stream = 0;
  }

  if (fc)
  {
	av_free (fc);
	fc = 0;
  }
}

void
VideoOutFileFFMPEG::writeNext (const Image & image)
{
  if (state)
  {
	return;
  }

  stream->codec.width = image.width;
  stream->codec.height = image.height;

  if (needHeader)
  {
	needHeader = false;

	state = avcodec_open (& stream->codec, codec);
	if (state < 0)
	{
	  return;
	}
	state = 0;

	state = av_write_header (fc);
	if (state < 0)
	{
	  return;
	}
	state = 0;
  }

  // First get image into a format that FFMPEG understands...
  Image sourceImage;
  int sourceFormat;
  if (*image.format == BGRChar)
  {
	sourceFormat = PIX_FMT_RGB24;
	sourceImage = image;
  }
  else if (*image.format == YVYUChar  ||  image.format->monochrome)
  {
	sourceFormat = PIX_FMT_YUV422;
	sourceImage = image * VYUYChar;
  }
  else if (*image.format == VYUYChar)
  {
	sourceFormat = PIX_FMT_YUV422;
	sourceImage = image;
  }
  else
  {
	sourceFormat = PIX_FMT_RGBA32;
	sourceImage = image * RGBAChar;
  }

  // ...then let FFMPEG convert it.
  int destFormat = stream->codec.pix_fmt;
  AVPicture source;
  source.data[0] = (unsigned char *) sourceImage.buffer;
  source.linesize[0] = sourceImage.width * sourceImage.format->depth;

  AVFrame dest;
  memset (&dest, 0, sizeof (dest));

  int size = avpicture_get_size (destFormat, image.width, image.height);
  unsigned char * destBuffer = new unsigned char[size];
  avpicture_fill ((AVPicture *) &dest, destBuffer, destFormat, image.width, image.height);
  state = img_convert ((AVPicture *) &dest, destFormat,
					   &source, sourceFormat,
					   image.width, image.height);
  if (state >= 0)
  {
	state = 0;

	// Finally, encode and write the frame
	size = 1024 * 1024;
	unsigned char * videoBuffer = new unsigned char[size];
	size = avcodec_encode_video (& stream->codec, videoBuffer, size, &dest);
	if (size < 0)
	{
	  state = size;
	}
	else
	{
	  state = av_write_frame (fc, 0, videoBuffer, size);
	  if (state >= 0)
	  {
		state = 0;
	  }
	}
	delete videoBuffer;
  }
  delete destBuffer;
}

bool
VideoOutFileFFMPEG::good () const
{
  return ! state;
}

void
VideoOutFileFFMPEG::set (const std::string & name, double value)
{
  if (name == "framerate")
  {
	if (stream)
	{
	  stream->codec.frame_rate = (int) rint (value * stream->codec.frame_rate_base);
	}
  }
}


// class VideoFileFormatFFMPEG ------------------------------------------------

VideoFileFormatFFMPEG::VideoFileFormatFFMPEG ()
{
  av_register_all ();
  //register_avcodec (& CodecY422::codecRecord);
}

VideoInFile *
VideoFileFormatFFMPEG::openInput (const std::string & fileName, const PixelFormat & hint) const
{
  return new VideoInFileFFMPEG (fileName, hint);
}

VideoOutFile *
VideoFileFormatFFMPEG::openOutput (const std::string & fileName, const std::string & formatName, const std::string & codecName) const
{
  return new VideoOutFileFFMPEG (fileName, formatName, codecName);
}

float
VideoFileFormatFFMPEG::isIn (const std::string & fileName) const
{
  return 1.0;  // For now, we pretend to handle everything.
}

float
VideoFileFormatFFMPEG::handles (const std::string & formatName, const std::string & codecName) const
{
  return 1.0;
}
