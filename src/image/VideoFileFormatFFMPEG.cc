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
  timestampMode = false;

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
  if (cc  &&  frame < cc->frame_number)
  {
	close ();
	open (fileName);
  }

  while (cc  &&  cc->frame_number < frame)
  {
	readNext (NULL);
	if (! gotPicture)
	{
	  return;
	}
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
	if (size <= 0)
	{
	  if (packet.size)
	  {
		av_free_packet (&packet);  // sets packet.size to zero
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

	while (size > 0  &&  ! gotPicture)
	{
      int used = avcodec_decode_video (cc, &picture, &gotPicture, data, size);
      if (used < 0)
	  {
		state = used;
		return;
	  }
      size -= used;
      data += used;
	}
  }

  if (gotPicture  &&  image)
  {
	extractImage (*image);
  }
}

/**
   Registry of states:
   0 = everything good
   [-7,-1] = FFMPEG libavformat errors, see avformat.h
   -10 = did not find a video stream
   -11 = did not find a codec
   -12 = VideoInFile is closed
 **/
bool
VideoInFileFFMPEG::good () const
{
  return ! state;
}

void
VideoInFileFFMPEG::setTimestampMode (bool frames)
{
  timestampMode = frames;
}

void
VideoInFileFFMPEG::open (const string & fileName)
{
  size = 0;
  memset (&picture, 0, sizeof (picture));
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

  codec = avcodec_find_decoder (cc->codec_id);
  if (! codec)
  {
	state = -11;
	return;
  }

  if (codec->capabilities & CODEC_CAP_TRUNCATED)
  {
	cc->flags |= CODEC_FLAG_TRUNCATED;
  }
  if (hint->monochrome)
  {
	cc->flags |= CODEC_FLAG_GRAY;
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
	av_free_packet (&packet);  // sets size field to zero
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

PixelFormatRGBABits RGB24 (3, 0xFF0000, 0xFF00, 0xFF, 0x0);

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
  else if (cc->pix_fmt == PIX_FMT_YUV411P)
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

	  for (int y = 0; y < cc->height; y++)
	  {
		for (int x = 0; x < cc->width; x += 4)
		{
		  int hx = x / 2;
		  int fx = x / 4;

		  unsigned int U = picture.data[1][y * picture.linesize[1] + fx];
		  unsigned int V = picture.data[2][y * picture.linesize[2] + fx] << 16;

		  unsigned int Y0 = picture.data[0][y * picture.linesize[0] + x];
		  unsigned int Y1 = picture.data[0][y * picture.linesize[0] + x+1];
		  unsigned int Y2 = picture.data[0][y * picture.linesize[0] + x+2];
		  unsigned int Y3 = picture.data[0][y * picture.linesize[0] + x+3];

		  that (hx,   y) = (Y1 << 24) | V | (Y0 << 8) | U;
		  that (hx+1, y) = (Y3 << 24) | V | (Y2 << 8) | U;
		}
	  }
	}
  }
  else if (cc->pix_fmt == PIX_FMT_YUV422)
  {
	image.attach (picture.data[0], cc->width, cc->height, YVYUChar);
  }
  else if (cc->pix_fmt == PIX_FMT_BGR24)
  {
	image.attach (picture.data[0], cc->width, cc->height, RGB24);
  }
  else
  {
	throw "Unknown pixel format";
  }

  if (timestampMode)
  {
	image.timestamp = cc->frame_number - 1;
  }
  else
  {
	image.timestamp = (double) (cc->frame_number - 1) * stream->r_frame_rate_base / stream->r_frame_rate;
  }
  gotPicture = 0;
}


// class VideoOutFileFFMPEG ---------------------------------------------------

VideoOutFileFFMPEG::VideoOutFileFFMPEG (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  stream = 0;
  codec = 0;
  needHeader = true;

  fc = av_alloc_format_context ();
  if (! fc)
  {
	state = -10;
	return;
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

  strcpy (fc->filename, fileName.c_str ());

  stream = av_new_stream (fc, 0);
  if (! stream)
  {
	state = -10;
	return;
  }

  // Add code here to search for named codec as well
  codec = avcodec_find_encoder (fc->oformat->video_codec);
  if (! codec)
  {
	state = -12;
	return;
  }

  // Set codec parameters.
  AVCodecContext & cc = stream->codec;
  cc.codec_type = codec->type;
  cc.codec_id   = (CodecID) codec->id;
  cc.gop_size     = 12; // default = 50; industry standard is 12
  //cc.max_b_frames = 2;  // default = 0; when nonzero, FFMPEG crashes sometimes

  state = av_set_parameters (fc, 0);
  if (state < 0)
  {
	return;
  }

  state = url_fopen (& fc->pb, fileName.c_str (), URL_WRONLY);
  if (state < 0)
  {
	return;
  }

  state = 0;
}

VideoOutFileFFMPEG::~VideoOutFileFFMPEG ()
{
  // Some members are zeroed here, even though this object is being destructed
  // and the members will never be touched again.  The rationale is that
  // this code could eventually be moved into a close() method, with a
  // corresponding open() method, so the the members may eventually be reused.

  if (codec)
  {
	if (! needHeader)
	{
	  avcodec_close (& stream->codec);
	}
	codec = 0;
  }

  if (! state  &&  fc  &&  ! needHeader)
  {
	av_write_trailer (fc);  // Clears private data used by avformat.  Private data is not allocated until av_write_header(), so this is balanced.
  }

  needHeader = true;

  if (stream)
  {
	if (stream->codec.stats_in)
	{
	  av_free (stream->codec.stats_in);
	}

	av_free (stream);
	stream = 0;
  }

  url_fclose (& fc->pb);

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
	  AVPacket packet;
	  av_init_packet (&packet);
	  packet.pts = stream->codec.coded_frame->pts;
	  if (stream->codec.coded_frame->key_frame)
	  {
		packet.flags |= PKT_FLAG_KEY;
	  }
	  packet.stream_index = 0;
	  packet.data = videoBuffer;
	  packet.size = size;
	  state = av_write_frame (fc, &packet);
	  if (state >= 0)
	  {
		state = 0;
	  }
	}
	delete [] videoBuffer;
  }
  delete [] destBuffer;
}

bool
VideoOutFileFFMPEG::good () const
{
  return ! state;
}

void
VideoOutFileFFMPEG::set (const std::string & name, double value)
{
  if (stream)
  {
	if (name == "framerate")
	{
	  stream->codec.frame_rate = (int) rint (value * stream->codec.frame_rate_base);
	}
	else if (name == "bitrate")
	{
	  stream->codec.bit_rate = (int) rint (value);
	}
	else if (name == "gop")
	{
	  stream->codec.gop_size = (int) rint (value);
	}
	else if (name == "bframes")
	{
	  stream->codec.max_b_frames = (int) rint (value);
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
