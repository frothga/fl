/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.15, 1.16, 1.18 thru 1.22 Copyright 2005 Sandia Corporation.
Revisions 1.24 thru 1.34             Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.34  2007/08/13 04:12:14  Fred
Use PixelBufferPacked::stride directly as the number of bytes in a row.

Revision 1.33  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.32  2006/11/12 15:06:18  Fred
Add comment to clarify outer loop of seek() method.  Handle timestamping of
raw video.

Use planar PixelFormats to avoid extra conversions.  Better selection of
PixelFormat.  More efficient allocation of working buffer.

Ability to select named codecs.

Revision 1.31  2006/04/08 15:16:28  Fred
Strip "Char" from all YUV-type formats.  It is ugly, and doesn't add any
information in this case.

Revision 1.30  2006/03/20 05:41:10  Fred
Get rid of hint.  Instead, follow the paradigm of returing an image with no
conversion.  Make use of new planar formats and get rid of conversion code.

Revision 1.29  2006/02/25 22:38:32  Fred
Change image structure by encapsulating storage format in a new PixelBuffer
class.  Must now unpack the PixelBuffer before accessing memory directly. 
ImageOf<> now intercepts any method that may modify the buffer location and
captures the new address.

Revision 1.28  2006/01/29 03:22:02  Fred
Rework seekTime() to use only timestamps, and not depend on an estimate of
frame rate.  Move linear seeking back into seekFrame().

Revision 1.27  2006/01/25 03:37:53  Fred
Set the PTS of images going into a video file based on the timestamp field. 
Set the timestamp field from PTS rather than generating it based on a fixed
framerate assumption when reading video.

Remove hack for telecine.  Instead, fixed FFMPEG.

Framerate selection code had num and den reversed, so was failing.

Revision 1.26  2006/01/16 05:45:01  Fred
Protect against uninitialized packet.

Revision 1.25  2006/01/02 20:30:53  Fred
Fix seek().  Previous changes to keep up with FFMPEG were cursory and poorly
tested.  This checkin is a more thorough pass through the code to make sure
everything is coherent.  It fixes most of the outstanding bugs in seek(),
including the crash in the first GOP.

Revision 1.24  2005/12/17 14:40:36  Fred
Move file close inside a guard against the existence for the format context
that holds the file.

Revision 1.23  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.22  2005/10/09 05:34:42  Fred
Change AVStream::codec to a pointer.

Update revision history and add Sandia copyright notice.

Revision 1.21  2005/08/27 13:20:57  Fred
Compilation fix for MSVC 7.1

Revision 1.20  2005/08/06 15:31:39  Fred
Update to latest FFMPEG: new way of representing time base and frame rate.

Revision 1.19  2005/05/29 15:55:54  Fred
Use standard BGRChar rather than one created locally.

Revision 1.18  2005/05/01 21:16:46  Fred
Adjust to more rigorous naming of PixelFormats.

Revision 1.17  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.16  2005/01/22 21:25:28  Fred
MSVC compilability fix:  Be explicit about namespace of PixelFormat, since
FFMPEG defines an enumeration also called PixelFormat.  Use fl/math.h.

Take into account different streams in a video file.

Revision 1.15  2005/01/12 05:37:22  rothgang
Update to latest ffmpeg: Use av_read_frame() rather than av_read_packet().  Add
flag to end of av_seek_frame(), and use stream's native time base.

Revision 1.14  2004/09/08 17:11:40  rothgang
Add trap for case where video does not have any valid timestamps.  Switches to
linear seeking instead.

Revision 1.13  2004/08/29 16:00:29  rothgang
A hack for seeking in AVIs.  Actually, already fixed this directly in FFMPEG,
so just storing the hack for the record.  The fix in FFMPEG may not be correct
or permanent, so this code may be useful sometime (but hopefully not).

Revision 1.12  2004/08/25 15:30:59  rothgang
Fix handling of last few frames in output.

Revision 1.11  2004/08/10 21:15:43  rothgang
Add ability to get attributes from input stream.  Use this to get duration from
FFMPEG.

Revision 1.10  2004/08/10 14:56:23  rothgang
Tidy up seek code changes.  Added "expectedSkew" to adjust requested timestamp
back a little to ensure we don't overshoot the target frame.

Revision 1.9  2004/08/10 03:35:57  rothgang
(The previous version wasn't quite as correct as claimed.  Hopefully, this one
is.)  Uses PTS rather than DTS to determine the frame of the next retrieved
image.  The skew built into PTS is exactly the correction needed to account for
the frames skipped when picking up in the middle of an mpeg stream.  Need to
add code now to compensate for the skew ahead of time.  Also need code to avoid
doing a search if we are close enough to read forward to the requested frame.

Revision 1.8  2004/08/08 23:13:00  rothgang
Use FFMPEG's seek facility.  This code is correct, and it works for the most
part.  However, the time base information found by FFMPEG is wrong for all
video files tested, so there is drift between the correct frame and the frame
found by this method.  To correct MPEG* files, need to probe the file and find
the duration of one frame.  Not sure how to correct AVI files.  Don't deal with
any other container types at present.

Revision 1.7  2004/08/07 15:58:52  rothgang
Setting B frames causes FFMPEG to crash, so suppress for now.

Revision 1.6  2004/08/03 18:02:27  rothgang
Updates to work with current FFMPEG: New function for allocating format
context.  New interface to av_write_frame().

Add two more user-tunable parameters: GOP size and number of B frames.

Revision 1.5  2004/06/08 19:30:01  rothgang
Minor update to flag handling to keep in sync with ffmpeg.

Revision 1.4  2004/05/08 19:47:09  rothgang
Change semantics of good() slightly by not reading ahead to check for eof.

Fix some warnings from valgrind about imbalanced new[] and delete.

Revision 1.3  2003/12/30 16:17:10  rothgang
Create timestamp mode, which switches between frame numbers and seconds for the
value of Image::timestamp.  Add bitrate setting.  Add more pixel formats. 
Guarantee that memory held by returned image is valid until next call to video
object.

Revision 1.2  2003/07/17 14:48:16  rothgang
Updates for latest snapshot of FFMPEG.  Deal with new approach to initializing
AVFormatContext, and with change in how frame_rate_base is stored.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/video.h"
#include "fl/math.h"


// For debugging only
//#include "fl/slideshow.h"


using namespace std;
using namespace fl;


// class VideoInFileFFMPEG ----------------------------------------------------

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
	}

	if (packet.dts != AV_NOPTS_VALUE)
	{
	  switch (codec->id)
	  {
		case CODEC_ID_DVVIDEO:
		case CODEC_ID_RAWVIDEO:
		  nextPTS = picture.pts + (int64_t) rint ((double) stream->r_frame_rate.den / stream->r_frame_rate.num * stream->time_base.den / stream->time_base.num);  // TODO: use AV rational arithmetic instead
		  break;
		default:
		  nextPTS = packet.dts;
	  }
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
	case PIX_FMT_YUV422:
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

VideoOutFileFFMPEG::VideoOutFileFFMPEG (const std::string & fileName, const std::string & formatName, const std::string & codecName)
{
  stream = 0;
  codec = 0;
  pixelFormat = 0;
  needHeader = true;

  fc = av_alloc_format_context ();
  if (! fc)
  {
	state = -10;
	return;
  }

  // Select container format
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

  // Select codec
  codec = 0;
  if (codecName.size ())
  {
	for (AVCodec * i = first_avcodec; i; i = i->next)
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

struct PixelFormatMapping
{
  fl::PixelFormat * fl;
  ::PixelFormat     av;
};

static PixelFormatMapping pixelFormatMap[] =
{
  {&YUV420,   PIX_FMT_YUV420P},
  {&YUV411,   PIX_FMT_YUV411P},
  {&YUYV,     PIX_FMT_YUV422},
  {&UYVY,     PIX_FMT_UYVY422},
  {&RGBChar,  PIX_FMT_RGB24},
  {&BGRChar,  PIX_FMT_BGR24},
  {&GrayChar, PIX_FMT_GRAY8},
  {0}
};

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

	  // Select PIX_FMT associate with image.formt
	  PixelFormatMapping * m = pixelFormatMap;
	  while (m->fl)
	  {
		if (image.format == m->fl) break;
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

  if (image.timestamp < 95443)  // approximately 2^33 / 90kHz, or about 26.5 hours.  Times larger than this are probably coming from the system clock and are not intended to be encoded in the video.
  {
	AVCodecContext & cc = *stream->codec;
	dest.pts = (int64_t) rint (image.timestamp * cc.time_base.den / cc.time_base.num);
  }

  // Finally, encode and write the frame
  int size = 1024 * 1024;
  videoBuffer.grow (size);
  size = avcodec_encode_video (stream->codec, (unsigned char *) videoBuffer, size, &dest);
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
	packet.data = (unsigned char *) videoBuffer;
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
