/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
05/2005 Fred Rothganger -- Use new PixelFormat names.
08/2005 Fred Rothganger -- Update to new time base representation in FFMPEG.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


12/2005 Fred Rothganger -- Fix problems with seek.
01/2006 Fred Rothganger -- Protect against uninitialized packet.  Read/write
        timestamps in video file.  Pure seek on timestamp.
02/2006 Fred Rothganger -- Change Image structure.
*/


#include "fl/video.h"
#include "fl/math.h"


// For debugging only
//#include "fl/slideshow.h"


using namespace std;
using namespace fl;


// class VideoInFileFFMPEG ----------------------------------------------------

VideoInFileFFMPEG::VideoInFileFFMPEG (const std::string & fileName, const fl::PixelFormat & hint)
{
  fc = 0;
  cc = 0;
  packet.size = 0;
  packet.data = 0;
  av_init_packet (&packet);
  timestampMode = false;
  seekLinear = false;

  this->hint = &hint;
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
	  data = 0;
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
  int64_t startPTS   = (int64_t) rint (startTime * stream->time_base.den / stream->time_base.num);
  int64_t horizonPTS = targetPTS - (int64_t) rint ((double) stream->time_base.den / stream->time_base.num);  // willing to sift forward up to 1 second before using seek
  int64_t framePeriod = (int64_t) rint ((double) stream->r_frame_rate.den / stream->r_frame_rate.num * stream->time_base.den / stream->time_base.num);

  while (targetPTS < picture.pts  ||  nextPTS <= targetPTS)
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
	  data = packet.data;
	  gotPicture = false;

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
  // Use rint() because DTS should be exactly on some frame's timestamp,
  // and we want to compensate for numerical error.
  // Add 1 to be consistent with normal frame_number semantics.  IE: we have
  // already retrieved the picture, so frame_number should point to next
  // picture.
  cc->frame_number = 1 + (int) rint
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
  if (state)
  {
	return;  // Don't attempt to read when we are in an error state.
  }

  while (! gotPicture)
  {
	if (size <= 0)
	{
	  av_free_packet (&packet);  // sets packet.size to zero
	  state = av_read_frame (fc, &packet);
	  if (state < 0)
	  {
		size = 0;
		data = 0;
	  }
	  else
	  {
		state = 0;
		if (packet.stream_index != stream->index) continue;
		size = packet.size;
		data = packet.data;
	  }
	}

	while ((size > 0  ||  state)  &&  ! gotPicture)
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
		state = 0;  // to reset EOF condition while emptying out codec's queue.

		if (picture.pts == AV_NOPTS_VALUE  ||  picture.pts == 0)
		{
		  switch (codec->id)
		  {
			case CODEC_ID_DVVIDEO:
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
	}

	if (packet.dts != AV_NOPTS_VALUE)
	{
	  // May need to condition this on codec, since it is true
	  // specifically for MPEG.
	  nextPTS = packet.dts;
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
	if (fc->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
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
  startTime = max (startTime, 0.0);
}

void
VideoInFileFFMPEG::close ()
{
  av_free_packet (&packet);  // sets size field to zero
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
  if (cc->pix_fmt == PIX_FMT_YUV420P)
  {
	if (hint->monochrome  ||  cc->flags & CODEC_FLAG_GRAY)
	{
	  image.format = &GrayChar;
	  image.resize (cc->width, cc->height);
	  ImageOf<unsigned char> that (image);
	  for (int y = 0; y < cc->height; y++)
	  {
		memcpy (&that(0,y), picture.data[0] + y * picture.linesize[0], cc->width);
	  }
	}
	else
	{
	  image.format = &UYVYChar;
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
	  ImageOf<unsigned char> that (image);
	  for (int y = 0; y < cc->height; y++)
	  {
		memcpy (&that(0,y), picture.data[0] + y * picture.linesize[0], cc->width);
	  }
	}
	else
	{
	  image.format = &UYVYChar;
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
	image.attach (picture.data[0], cc->width, cc->height, YUYVChar);
  }
  else if (cc->pix_fmt == PIX_FMT_UYVY422)
  {
	image.attach (picture.data[0], cc->width, cc->height, UYVYChar);
  }
  else if (cc->pix_fmt == PIX_FMT_RGB24)
  {
	image.attach (picture.data[0], cc->width, cc->height, RGBChar);
  }
  else if (cc->pix_fmt == PIX_FMT_BGR24)
  {
	image.attach (picture.data[0], cc->width, cc->height, BGRChar);
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
	image.timestamp = (double) picture.pts * stream->time_base.num / stream->time_base.den;
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
  AVCodecContext & cc = *stream->codec;
  cc.codec_type = codec->type;
  cc.codec_id   = codec->id;
  cc.gop_size     = 12; // default = 50; industry standard is 12 or 15
  if (codec->id == CODEC_ID_MPEG2VIDEO)
  {
	cc.max_b_frames = 2;
  }
  if (   ! strcmp (fc->oformat->name, "mp4")
	  || ! strcmp (fc->oformat->name, "mov")
	  || ! strcmp (fc->oformat->name, "3gp"))
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
	  // Flush codec (ie: push out any B frames).
	  const int bufferSize = 1024 * 1024;
	  unsigned char * videoBuffer = new unsigned char[bufferSize];
	  while (true)
	  {
		int size = avcodec_encode_video (stream->codec, videoBuffer, bufferSize, 0);
		if (size > 0)
		{
		  AVPacket packet;
		  av_init_packet (&packet);
		  packet.pts = stream->codec->coded_frame->pts;
		  if (stream->codec->coded_frame->key_frame)
		  {
			packet.flags |= PKT_FLAG_KEY;
		  }
		  packet.stream_index = stream->index;
		  packet.data = videoBuffer;
		  packet.size = size;
		  state = av_write_frame (fc, &packet);
		}
		else
		{
		  break;
		}
	  }
	  delete [] videoBuffer;

	  avcodec_close (stream->codec);
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
	if (stream->codec->stats_in)
	{
	  av_free (stream->codec->stats_in);
	}

	av_free (stream);
	stream = 0;
  }

  if (fc)
  {
	url_fclose (& fc->pb);
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

  stream->codec->width = image.width;
  stream->codec->height = image.height;
  if (stream->codec->pix_fmt == PIX_FMT_NONE)
  {
	enum ::PixelFormat best = PIX_FMT_YUV420P;  // FFMPEG's default
	if (codec->pix_fmts)  // options are available, so enumerate and find best match for image.format
	{
	  best = codec->pix_fmts[0];
	  const enum ::PixelFormat * p = codec->pix_fmts;
	  while (*p != -1)
	  {
		switch (*p)
		{
		  case PIX_FMT_RGB24:
			if (*image.format == RGBChar) best = *p;
			break;
		  case PIX_FMT_BGR24:
			if (*image.format == BGRChar) best = *p;
			break;
		  case PIX_FMT_YUV422:
			if (*image.format == YUYVChar) best = *p;
			break;
		  case PIX_FMT_UYVY422:
			if (*image.format == UYVYChar) best = *p;
			break;
		  case PIX_FMT_GRAY8:
			if (*image.format == GrayChar) best = *p;
			break;
		}
		p++;
	  }
	}
	stream->codec->pix_fmt = best;
  }

  if (needHeader)
  {
	needHeader = false;

	state = avcodec_open (stream->codec, codec);
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
  if (*image.format == RGBChar)
  {
	sourceFormat = PIX_FMT_RGB24;
	sourceImage = image;
  }
  else if (*image.format == UYVYChar)
  {
	sourceFormat = PIX_FMT_UYVY422;
	sourceImage = image;
  }
  else if (*image.format == YUYVChar)
  {
	sourceFormat = PIX_FMT_YUV422;
	sourceImage = image;
  }
  else if (image.format->monochrome)
  {
	sourceFormat = PIX_FMT_GRAY8;
	sourceImage = image * GrayChar;
  }
  else
  {
	sourceFormat = PIX_FMT_RGB24;
	sourceImage = image * RGBChar;
  }

  // ...then let FFMPEG convert it.
  int destFormat = stream->codec->pix_fmt;
  AVPicture source;
  source.data[0] = (unsigned char *) ((PixelBufferPacked *) sourceImage.buffer)->memory;
  source.linesize[0] = sourceImage.width * sourceImage.format->depth;

  AVFrame dest;
  avcodec_get_frame_defaults (&dest);

  int size = avpicture_get_size (destFormat, image.width, image.height);
  unsigned char * destBuffer = new unsigned char[size];
  avpicture_fill ((AVPicture *) &dest, destBuffer, destFormat, image.width, image.height);
  state = img_convert ((AVPicture *) &dest, destFormat,
					   &source, sourceFormat,
					   image.width, image.height);
  if (state >= 0)
  {
	state = 0;

	if (image.timestamp < 95443)  // approximately 2^33 / 90kHz, or about 26.5 hours.  Times larger than this are probably coming from the system clock and are not intended to be encoded in the video.
	{
	  AVCodecContext & cc = *stream->codec;
	  dest.pts = (int64_t) rint (image.timestamp * cc.time_base.den / cc.time_base.num);
	}

	// Finally, encode and write the frame
	size = 1024 * 1024;
	unsigned char * videoBuffer = new unsigned char[size];
	size = avcodec_encode_video (stream->codec, videoBuffer, size, &dest);
	if (size < 0)  // TODO: Not sure if this case ever happens.
	{
	  state = size;
	}
	else if (size > 0)
	{
	  AVPacket packet;
	  av_init_packet (&packet);
	  packet.pts = av_rescale_q (stream->codec->coded_frame->pts, stream->codec->time_base, stream->time_base);
	  if (stream->codec->coded_frame->key_frame)
	  {
		packet.flags |= PKT_FLAG_KEY;
	  }
	  packet.stream_index = stream->index;
	  packet.data = videoBuffer;
	  packet.size = size;
	  state = av_write_frame (fc, &packet);
	  if (state == 1)
	  {
		// According to doc-comments on av_write_frame(), this means
		// "end of stream wanted".  Not sure what action to take here, or
		// if the doc-comment is even valid.
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
		stream->codec->time_base.den = (int) rint (value * AV_TIME_BASE);
	  }
	}
	else if (name == "bitrate")
	{
	  stream->codec->bit_rate = (int) rint (value);
	}
	else if (name == "gop")
	{
	  stream->codec->gop_size = (int) rint (value);
	}
	else if (name == "bframes")
	{
	  stream->codec->max_b_frames = (int) rint (value);
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
VideoFileFormatFFMPEG::openInput (const std::string & fileName, const fl::PixelFormat & hint) const
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
