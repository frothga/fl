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


// Force stdint.h under Fedora Core (and maybe others) to define the INT64_C
// macro for use with FFMPEG constants (namely AV_NOPTS_VALUE).  This
// definition must occur before stdint.h is otherwise included.
#define __STDC_CONSTANT_MACROS

#include "fl/video.h"
#include "fl/math.h"

extern "C"
{
# include <libavformat/avformat.h>
}

#include <typeinfo>


// For debugging only
//#include "fl/slideshow.h"


using namespace std;
using namespace fl;


// class VideoInFileFFMPEG ----------------------------------------------------

class VideoInFileFFMPEG : public VideoInFile
{
public:
  VideoInFileFFMPEG (const std::string & fileName);
  virtual ~VideoInFileFFMPEG ();

  virtual void seekFrame (int frame);
  virtual void seekTime (double timestamp);
  virtual void readNext (Image & image);
  virtual bool good () const;
  virtual void setTimestampMode (bool frames = false);
  virtual void get (const std::string & name, double & value);

  // private ...
  void open (const std::string & fileName);
  void close ();
  void readNext (Image * image);  ///< same as readNext() above, except if image is null, then don't do extraction step
  void extractImage (Image & image);

  AVFormatContext * fc;
  AVCodecContext * cc;
  AVStream * stream;
  AVCodec * codec;	
  AVFrame picture;
  AVPacket packet;  ///< Ensure that if nextImage() attaches image to packet, the memory won't be freed before next read.
  int size;
  int gotPicture;  ///< indicates that picture contains a full image
  int state;  ///< state == 0 means good; anything else means we can't read more frames
  bool timestampMode;  ///< Indicates that image.timestamp should be frame number rather than presentation time.
  int64_t expectedSkew;  ///< How much before a given target time to read in order to get a keyframe, in stream units.
  bool seekLinear;  ///< Indicates that the file only supports linear seeking, not random seeking.  Generally due to lack of timestamps in stream.
  int64_t nextPTS;  ///< DTS of most recent packet pushed into decoder.  In MPEG, this will be the PTS of the next picture to come out of decoder, which occurs next time a packet is pushed in.
  double startTime;  ///< Best estimate of timestamp on first image in video.

  std::string fileName;
};

VideoInFileFFMPEG::VideoInFileFFMPEG (const std::string & fileName)
{
  fc = 0;
  cc = 0;
  packet.size = 0;
  packet.data = 0;
  av_init_packet (&packet);
  timestampMode = false;
  seekLinear = false;

  this->fileName = fileName;

  open (fileName);
}

VideoInFileFFMPEG::~VideoInFileFFMPEG ()
{
  close ();
}

/**
   Assumes frame rate is constant throughout entire video, or at least
   from the beginning to the requested frame.
 **/
void
VideoInFileFFMPEG::seekFrame (int frame)
{
  if (state  ||  ! stream)
  {
	return;
  }

  if (seekLinear)
  {
	if (frame < cc->frame_number)
	{
	  // Reset to start of file
	  state = av_seek_frame (fc, stream->index, 0, AVSEEK_FLAG_BYTE);
	  if (state < 0)
	  {
		return;
	  }
	  avcodec_flush_buffers (cc);
	  av_free_packet (&packet);
	  size = 0;
	  cc->frame_number = 0;
	}

	// Read forward until finding the exact frame requested.
	while (cc->frame_number < frame)
	{
	  readNext (0);
	  if (! gotPicture)
	  {
		return;
	  }
	  gotPicture = 0;
	}
  }
  else
  {
	seekTime (startTime + (double) frame * stream->r_frame_rate.den / stream->r_frame_rate.num);  // = frame / r_frame_rate
  }
}

/**
   Assumes timestamps are monotonic in video file.

   \todo Seek in c.avi (bear and dog rotating) is screwed up at the FFMPEG
   level.  It has more index than video, and the index seems to be on a
   different time base than the video itself.
 **/
void
VideoInFileFFMPEG::seekTime (double timestamp)
{
  if (state  ||  ! stream)
  {
	return;
  }

  timestamp = max (timestamp, startTime);

  if (seekLinear)
  {
	// The expression below uses floor() because timestamp refers to the
	// frame visible at the time, rather than the nearest frame boundary.
	seekFrame ((int) floor ((timestamp - startTime) * stream->r_frame_rate.num / stream->r_frame_rate.den + 1e-6));
	return;
  }

  // This is counterintuitive, but stream->time_base is apparently defined
  // as the amount to *divide* seconds by to get stream units.
  // targetPTS uses ceil() to force rounding errors in the direction of the
  // next frame.  This produces more intuitive results.
  int64_t targetPTS  = (int64_t) ceil (timestamp * stream->time_base.den / stream->time_base.num);
  int64_t startPTS   = (int64_t) roundp (startTime * stream->time_base.den / stream->time_base.num);
  int64_t horizonPTS = targetPTS - (int64_t) roundp ((double) stream->time_base.den / stream->time_base.num);  // willing to sift forward up to 1 second before using seek
  int64_t framePeriod = (int64_t) roundp ((double) stream->r_frame_rate.den / stream->r_frame_rate.num * stream->time_base.den / stream->time_base.num);

  while (targetPTS < picture.pts  ||  nextPTS <= targetPTS)  // targetPTS is not in [picture.pts, nextPTS), ie: not in the current picture.  This relies on nextPTS always being set to the start of the next image, or to AV_NOPTS_VALUE if end of video.
  {
	if (targetPTS < nextPTS  ||  nextPTS < horizonPTS)  // Must move backwards or a long ways away
	{
	  // Use seek to position at or before the frame
	  int64_t skewDTS = targetPTS - expectedSkew;
	  if (skewDTS < startPTS)
	  {
		state = av_seek_frame (fc, stream->index, 0, AVSEEK_FLAG_BYTE);
	  }
	  else
	  {
		bool backwards = false;
		if (packet.size)
		{
		  backwards = packet.dts > skewDTS;
		}
		state = av_seek_frame (fc, stream->index, skewDTS, backwards ? AVSEEK_FLAG_BACKWARD : 0);
	  }
	  if (state < 0)
	  {
		return;
	  }

	  // Read the next key frame.  It is possible for a seek to find something
	  // other than a key frame.  For example, if an mpeg has timestamps on
	  // packets other than I frames.
	  avcodec_flush_buffers (cc);
	  do
	  {
		av_free_packet (&packet);
		state = av_read_frame (fc, &packet);
		if (state < 0)
		{
		  return;
		}
	  }
	  while (packet.stream_index != stream->index  ||  (stream->parser  &&  ! (packet.flags & PKT_FLAG_KEY)));
	  state = 0;
	  size = packet.size;
	  gotPicture = 0;

	  // Guard against unseekability
	  if (packet.dts == AV_NOPTS_VALUE)
	  {
		// Timing info is not available, so fall back to "linear" mode
		seekLinear = true;
		seekTime (timestamp);
		return;
	  }
	}

	// Sift forward until the current picture contains the requested time.
	do
	{
	  gotPicture = 0;
	  readNext (0);
	  if (! gotPicture) return;
	}
	while (nextPTS <= targetPTS);

	// Adjust skew if necessary
	if (targetPTS < picture.pts)
	{
	  // We overshot.  Need to target further ahead.
	  if (expectedSkew < picture.pts - targetPTS)
	  {
		expectedSkew = picture.pts - targetPTS;
	  }
	  else
	  {
		expectedSkew += framePeriod;
	  }
	}
  }
  gotPicture = 1;  // Hack to reactivate a picture if we already have it in hand.

  // Determine the number of frame that seek obtained
  // Use round() because DTS should be exactly on some frame's timestamp,
  // and we want to compensate for numerical error.
  // Add 1 to be consistent with normal frame_number semantics.  IE: we have
  // already retrieved the picture, so frame_number should point to next
  // picture.
  cc->frame_number = 1 + (int) roundp
  (
    ((double) (picture.pts - startPTS) * stream->time_base.num / stream->time_base.den)
	* stream->r_frame_rate.num / stream->r_frame_rate.den
  );
}

void
VideoInFileFFMPEG::readNext (Image & image)
{
  readNext (&image);
}

void
VideoInFileFFMPEG::readNext (Image * image)
{
  if (state) return;  // Don't attempt to read when we are in an error state.

  while (! gotPicture)
  {
	// Mysteriously, it is necessary to pay attention to whether or not
	// avcodec_decode_video2() fully consumes a packet.  It seems that
	// ffplay does not need to do this, but rather just feeds it packets
	// and then disposes of them.  Either I don't understand the ffplay
	// code, or I'm doing something wrong here.  In any case, it seems to
	// be working OK.
	if (size <= 0)
	{
	  av_free_packet (&packet);
	  state = av_read_frame (fc, &packet);
	  if (state < 0) return;
	  state = 0;
	  if (packet.stream_index != stream->index) continue;
	  size = packet.size;
	}

	int used = avcodec_decode_video2 (cc, &picture, &gotPicture, &packet);
	if (used < 0)
	{
	  state = used;
	  return;
	}
	size -= used;

	if (gotPicture)
	{
	  state = 0;  // to reset EOF condition while emptying out codec's queue.

	  if (picture.pts == AV_NOPTS_VALUE  ||  picture.pts == 0)
	  {
		switch (codec->id)
		{
		  case CODEC_ID_DVVIDEO:
		  case CODEC_ID_RAWVIDEO:
			picture.pts = packet.pts;  // since decoding is immediate
			break;
		  default:
			picture.pts = nextPTS;
		}
	  }
	}
	else if (state)
	{
	  return;
	}

	if (packet.dts != AV_NOPTS_VALUE)
	{
	  switch (codec->id)
	  {
		case CODEC_ID_DVVIDEO:
		case CODEC_ID_RAWVIDEO:
		  nextPTS = picture.pts + (int64_t) roundp ((double) stream->r_frame_rate.den / stream->r_frame_rate.num * stream->time_base.den / stream->time_base.num);  // TODO: use AV rational arithmetic instead
		  break;
		default:
		  nextPTS = packet.dts;
	  }
	}
  }

  if (image) extractImage (*image);
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
VideoInFileFFMPEG::get (const std::string & name, double & value)
{
  if (stream)
  {
	if (name == "duration")
	{
	  if (fc->duration != AV_NOPTS_VALUE)
	  {
		value = (double) fc->duration / AV_TIME_BASE;
	  }
	}
	else if (name == "startTime")
	{
	  value = startTime;
	}
  }
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

  stream = 0;
  for (int i = 0; i < fc->nb_streams; i++)
  {
	fc->streams[i]->discard = AVDISCARD_ALL;
	if (stream == 0  &&  fc->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
	{
	  stream = fc->streams[i];
	  fc->streams[i]->discard = AVDISCARD_DEFAULT;
	}
  }
  if (! stream)
  {
	state = -10;
	return;
  }
  cc = stream->codec;

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

  state = avcodec_open (cc, codec);
  if (state < 0)
  {
	return;
  }
  state = 0;

  seekLinear = false;
  expectedSkew = 0;
  nextPTS = 0;

  startTime = 0;
  if (stream->start_time != AV_NOPTS_VALUE)
  {
	startTime = (double) stream->start_time * stream->time_base.num / stream->time_base.den;
  }
  else if (fc->start_time != AV_NOPTS_VALUE)
  {
	startTime = (double) fc->start_time / AV_TIME_BASE;
  }
  startTime = max (startTime, 0.0);  // TODO: is this still needed?  test against lola vobs
}

void
VideoInFileFFMPEG::close ()
{
  av_free_packet (&packet);
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

void
VideoInFileFFMPEG::extractImage (Image & image)
{
  switch (cc->pix_fmt)
  {
	case PIX_FMT_YUV420P:
	  assert (picture.linesize[1] == picture.linesize[2]);
	  image.format = &YUV420;
	  image.buffer = new PixelBufferPlanar (picture.data[0], picture.data[1], picture.data[2], picture.linesize[0], picture.linesize[1], cc->height, YUV420.ratioH, YUV420.ratioV);
	  image.width = cc->width;
	  image.height = cc->height;
	  break;
	case PIX_FMT_YUV411P:
	  assert (picture.linesize[1] == picture.linesize[2]);
	  image.format = &YUV411;
	  image.buffer = new PixelBufferPlanar (picture.data[0], picture.data[1], picture.data[2], picture.linesize[0], picture.linesize[1], cc->height, YUV411.ratioH, YUV411.ratioV);
	  image.width = cc->width;
	  image.height = cc->height;
	  break;
	case PIX_FMT_YUYV422:
	  image.attach (picture.data[0], cc->width, cc->height, YUYV);
	  break;
	case PIX_FMT_UYVY422:
	  image.attach (picture.data[0], cc->width, cc->height, UYVY);
	  break;
	case PIX_FMT_RGB24:
	  image.attach (picture.data[0], cc->width, cc->height, RGBChar);
	  break;
	case PIX_FMT_BGR24:
	  image.attach (picture.data[0], cc->width, cc->height, BGRChar);
	  break;
	case PIX_FMT_GRAY8:
	  image.attach (picture.data[0], cc->width, cc->height, GrayChar);
	  break;
	default:
	  throw "Unsupported PIX_FMT";
  }

  if (timestampMode)
  {
	image.timestamp = cc->frame_number - 1;
  }
  else
  {
	image.timestamp = (double) picture.pts * stream->time_base.num / stream->time_base.den;
  }
  gotPicture = 0;
}


// class VideoOutFileFFMPEG ---------------------------------------------------

class VideoOutFileFFMPEG : public VideoOutFile
{
public:
  VideoOutFileFFMPEG (const std::string & fileName, const std::string & formatName, const std::string & codecName);
  ~VideoOutFileFFMPEG ();

  void open (const std::string & fileName, const std::string & formatName, const std::string & codecName);
  void close ();
  virtual void writeNext (const Image & image);
  virtual bool good () const;
  virtual void set (const std::string & name, double value);

  AVFormatContext * fc;
  AVStream * stream;
  AVCodec * codec;
  fl::PixelFormat * pixelFormat;  ///< The format in which the codec receives the image.
  bool needHeader;  ///< indicates that file header needs to be written; also that codec needs to be opened
  fl::Pointer videoBuffer;  ///< Working memory for encoder.
  int state;
};

VideoOutFileFFMPEG::VideoOutFileFFMPEG (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  state = 0;
  fc = 0;
  stream = 0;
  codec = 0;
  pixelFormat = 0;
  needHeader = false;

  open (fileName, formatName, codecName);
}

VideoOutFileFFMPEG::~VideoOutFileFFMPEG ()
{
  close ();
}

void
VideoOutFileFFMPEG::open (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  close ();

  fc = avformat_alloc_context ();
  if (! fc)
  {
	state = -10;
	return;
  }

  // Select container format
  const char * formatAddress = formatName.size () ? formatName.c_str () : 0;
  fc->oformat = av_guess_format
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

  // Select codec
  codec = 0;
  if (codecName.size ())
  {
	for (AVCodec * i = av_codec_next (0); i; i = av_codec_next (i))
	{
	  if (i->encode  &&  i->type == CODEC_TYPE_VIDEO  &&  codecName == i->name)
	  {
		codec = i;
		break;
	  }
	}
  }
  if (! codec)
  {
	// Use default codec for container
	codec = avcodec_find_encoder (fc->oformat->video_codec);
  }
  if (! codec)
  {
	state = -12;
	return;
  }

  // Set codec parameters.
  AVCodecContext & cc = *stream->codec;
  cc.codec_type = codec->type;
  cc.codec_id   = codec->id;
  cc.gop_size     = 12; // default = 50; industry standard is 12 or 15
  if (codec->id == CODEC_ID_MPEG2VIDEO)
  {
	cc.max_b_frames = 2;
  }
  if (fc->oformat->flags & AVFMT_GLOBALHEADER)
  {
	cc.flags |= CODEC_FLAG_GLOBAL_HEADER;
  }
  if (codec->supported_framerates)
  {
	const AVRational & fr = codec->supported_framerates[0];
	cc.time_base.num = fr.den;
	cc.time_base.den = fr.num;
  }
  else  // any framerate is ok, so pick our favorite default
  {
	cc.time_base.num = 1;
	cc.time_base.den = 24;
  }

  state = av_set_parameters (fc, 0);
  if (state < 0) return;

  if (fc->oformat->flags & AVFMT_NOFILE)
  {
	cerr << "WARNING: Non-file type video format.  Not sure how this works, but proceeding anyway." << endl;
  }
  else
  {
	state = url_fopen (& fc->pb, fileName.c_str (), URL_WRONLY);
	if (state < 0) return;
  }

  needHeader = true;
  state = 0;
}

void
VideoOutFileFFMPEG::close ()
{
  if (! needHeader  &&  fc  &&  ! state)  // A header was written, and probably some frames as well, so file needs to be closed out properly.
  {
	av_write_trailer (fc);  // Clears private data used by avformat.  Private data is not allocated until av_write_header(), so this is balanced.
  }

  if (codec)
  {
	avcodec_close (stream->codec);
	av_free (stream->codec);
	codec = 0;
  }

  if (stream)
  {
	av_free (stream);
	stream = 0;
  }

  if (fc)
  {
	av_metadata_free (&fc->metadata);
	if (! (fc->oformat->flags & AVFMT_NOFILE)) url_fclose (fc->pb);
	av_free (fc);
	fc = 0;
  }
}

struct PixelFormatMapping
{
  fl::PixelFormat * fl;
  ::PixelFormat     av;
};

static PixelFormatMapping pixelFormatMap[] =
{
  {&YUV420,   PIX_FMT_YUV420P},
  {&YUV411,   PIX_FMT_YUV411P},
  {&YUYV,     PIX_FMT_YUYV422},
  {&UYVY,     PIX_FMT_UYVY422},
  {&RGBChar,  PIX_FMT_RGB24},
  {&BGRChar,  PIX_FMT_BGR24},
  {&GrayChar, PIX_FMT_GRAY8},
  {0}
};

void
VideoOutFileFFMPEG::writeNext (const Image & image)
{
  if (state) return;

  stream->codec->width = image.width;
  stream->codec->height = image.height;
  if (stream->codec->pix_fmt == PIX_FMT_NONE)
  {
	enum ::PixelFormat best = PIX_FMT_YUV420P;  // FFMPEG's default
	if (codec->pix_fmts)  // options are available, so enumerate and find best match for image.format
	{
	  best = codec->pix_fmts[0];

	  // Select PIX_FMT associate with image.formt
	  PixelFormatMapping * m = pixelFormatMap;
	  while (m->fl)
	  {
		if ((const fl::PixelFormat *) image.format == m->fl) break;
		m++;
	  }
	  enum ::PixelFormat target = m->av;
	  if (target < 0  &&  image.format->monochrome)
	  {
		target = PIX_FMT_GRAY8;
	  }

	  // See if PIX_FMT is in supported list
	  if (target >= 0)
	  {
		const enum ::PixelFormat * p = codec->pix_fmts;
		while (*p != -1)
		{
		  if (*p == target)
		  {
			best = *p;
			break;
		  }
		  p++;
		}
	  }
	}
	stream->codec->pix_fmt = best;
  }

  if (needHeader)
  {
	// Must know pixel format before opening codec, and we only know that
	// after receiving the first image, so we open the codec here rather than
	// in open().
	state = avcodec_open (stream->codec, codec);
	if (state < 0) return;
	state = 0;

	state = av_write_header (fc);
	if (state < 0) return;
	state = 0;

	needHeader = false;
  }

  if (! pixelFormat)
  {
	PixelFormatMapping * m = pixelFormatMap;
	while (m->fl)
	{
	  if (m->av == stream->codec->pix_fmt) break;
	  m++;
	}
	if (! m->fl) throw "Unsupported PIX_FMT";
	pixelFormat = m->fl;
  }

  // Get image into a format that FFMPEG understands...
  AVFrame dest;
  avcodec_get_frame_defaults (&dest);
  Image converted = image * *pixelFormat;
  if (PixelBufferPlanar * pb = (PixelBufferPlanar *) converted.buffer)
  {
	dest.data[0] = (unsigned char *) pb->plane0;
	dest.data[1] = (unsigned char *) pb->plane1;
	dest.data[2] = (unsigned char *) pb->plane2;
	dest.linesize[0] = pb->stride0;
	dest.linesize[1] = pb->stride12;  // assumes planes always have depth 1
	dest.linesize[2] = pb->stride12;
  }
  else if (PixelBufferPacked * pb = (PixelBufferPacked *) converted.buffer)
  {
	dest.data[0] = (unsigned char *) pb->memory;
	dest.linesize[0] = pb->stride;
  }
  else throw "Unhandled buffer type";

  AVCodecContext * cc = stream->codec;
  if (image.timestamp < 95443)  // approximately 2^33 / 90kHz, or about 26.5 hours.  Times larger than this are probably coming from the system clock and are not intended to be encoded in the video.
  {
	dest.pts = (int64_t) roundp (image.timestamp * cc->time_base.den / cc->time_base.num);
  }

  // Finally, encode and write the frame
  int size = max (FF_MIN_BUFFER_SIZE, (int) ceil (converted.format->depth * converted.width * converted.height));  // Assumption: encoded image will never be larger than raw image.
  videoBuffer.grow (size);
  size = videoBuffer.size ();
  size = avcodec_encode_video (cc, (uint8_t *) videoBuffer, size, &dest);
  if (size < 0)
  {
	state = size;
  }
  else if (size > 0)
  {
	AVPacket packet;
	av_init_packet (&packet);
	packet.stream_index = stream->index;

	if (fc->oformat->flags & AVFMT_RAWPICTURE)
	{
	  packet.flags |= PKT_FLAG_KEY;
	  packet.data = (uint8_t *) &dest;
	  packet.size = sizeof (AVPicture);
	}
	else
	{
	  if (cc->coded_frame->pts != AV_NOPTS_VALUE)
	  {
		packet.pts = av_rescale_q (cc->coded_frame->pts, cc->time_base, stream->time_base);
	  }
	  if (cc->coded_frame->key_frame)
	  {
		packet.flags |= PKT_FLAG_KEY;
	  }
	  packet.data = (uint8_t *) videoBuffer;
	  packet.size = size;
	}

	state = av_interleaved_write_frame (fc, &packet);
	if (state == 1)
	{
	  state = 0;
	  close ();
	}
  }
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
	  if (codec  &&  codec->supported_framerates)
	  {
		// Restricted set of framerates, so select closest one
		const AVRational * fr = codec->supported_framerates;
		const AVRational * bestRate = fr;
		double bestDistance = INFINITY;
		while (fr->den)
		{
		  double rate = (double) fr->num / fr->den;
		  double distance = fabs (value - rate);
		  if (distance < bestDistance)
		  {
			bestDistance = distance;
			bestRate = fr;
		  }
		  fr++;
		}
		stream->codec->time_base.num = bestRate->den;
		stream->codec->time_base.den = bestRate->num;
	  }
	  else  // arbitrary framerate is acceptable
	  {
		stream->codec->time_base.num = AV_TIME_BASE;
		stream->codec->time_base.den = (int) roundp (value * AV_TIME_BASE);
	  }
	}
	else if (name == "bitrate")
	{
	  stream->codec->bit_rate = (int) roundp (value);
	}
	else if (name == "gop")
	{
	  stream->codec->gop_size = (int) roundp (value);
	}
	else if (name == "bframes")
	{
	  stream->codec->max_b_frames = (int) roundp (value);
	}
  }
}


// class VideoFileFormatFFMPEG ------------------------------------------------

void
VideoFileFormatFFMPEG::use ()
{
  vector<VideoFileFormat *>::iterator i;
  for (i = formats.begin (); i < formats.end (); i++)
  {
	if (typeid (**i) == typeid (VideoFileFormatFFMPEG)) return;
  }
  formats.push_back (new VideoFileFormatFFMPEG);
}

VideoFileFormatFFMPEG::VideoFileFormatFFMPEG ()
{
  av_register_all ();
}

VideoInFile *
VideoFileFormatFFMPEG::openInput (const std::string & fileName) const
{
  return new VideoInFileFFMPEG (fileName);
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
