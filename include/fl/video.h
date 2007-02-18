/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.7 and 1.9    Copyright 2005 Sandia Corporation.
Revisions 1.11 thru 1.15 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.15  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.14  2006/11/12 15:07:39  Fred
Use planar PixelFormats to avoid extra conversions.  More efficient allocation
of working buffer.

Revision 1.13  2006/03/20 05:19:59  Fred
Get rid of hint.  Instead, follow the paradigm that we return an image with
zero conversion.

Revision 1.12  2006/01/29 03:17:20  Fred
Rework seekTime() to use only timestamps, and not depend on an estimate of
frame rate.  Move linear seeking back into seekFrame().

Revision 1.11  2006/01/25 03:46:11  Fred
Set the timestamp field from PTS rather than generating it based on a fixed
framerate assumption when reading video.

Revision 1.10  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.9  2005/09/26 04:29:13  Fred
Add detail to revision history.

Revision 1.8  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.7  2005/01/22 20:58:31  Fred
MSVC compilability fix:  FFMPEG defines a PixelFormat enumeration, which
collides with fl::PixelFormat under VC compiler.

Revision 1.6  2004/09/08 17:10:56  rothgang
Add trap for case where video does not have any valid timestamps.  Switches to
linear seeking instead.

Revision 1.5  2004/08/10 21:16:41  rothgang
Add ability to get attributes from input stream.  Use this to get duration from
FFMPEG.

Add "expectedSkew" member, to compensate for difference between PTS and DTS
when seeking.

Revision 1.4  2004/05/08 19:49:34  rothgang
Change semantics of good() slightly by not reading ahead to check for eof.  Now
good() indicates the certainty that the previous read succeeded, but only the
possibility that the next read will succeed.

Revision 1.3  2003/12/30 16:21:10  rothgang
Create timestamp mode, which switches between frame number and seconds for
value of Image::timestamp.

Revision 1.2  2003/08/11 13:48:05  rothgang
Remove extern C from around include libavformat.h because it is embedded in the
library how.  Updated comments to doxygen format.  Added guarantee that Image
is always valid till next read.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_video_h
#define fl_video_h


#include "fl/image.h"

// This include is needed to declare data members in VideoFileFFMPEG
#include <ffmpeg/avformat.h>


namespace fl
{
  class VideoInFile;
  class VideoOutFile;

  /**
	 Video input stream.
	 Conceives of the video as an array of images.
	 The most general way to view a video file is as a group of independent
	 streams that begin and end at independent points and that contain frames
	 which should be presented to the viewer at prescribed points in time.
	 Frames can be image, audio, or whatever.  To handle that model, we would
	 probably need several more classes.  There should be a VideoStream class
	 which wraps one stream, and a Video class which wraps the group of
	 streams.  Since all the streams are drawn interleaved from one data
	 stream (in the usual OS sense), the Video class would wrap the data source
	 as well.
  **/
  class VideoIn
  {
  public:
	VideoIn (const std::string & fileName);
	~VideoIn ();

	void seekFrame (int frame);  ///< Position stream just before the given frame.  Numbers are zero based.  (Maybe they should be one-based.  Research the convention.)
	void seekTime (double timestamp);  ///< Position stream so that next frame will have the smallest timestamp >= the given timestamp.
	VideoIn & operator >> (Image & image);  ///< Extract next image frame.  image may end up attached to a buffer used internally by the video device or library, so it may be freed unexpectedly.  However, this clss guarantees that the memory will not be freed before the next call to a method of this class.
	bool good () const;  ///< Indicates that the stream is open and the last read (if any) succeeded.
	void setTimestampMode (bool frames = false);  ///< Changes image.timestamp from presentation time to frame number.
	void get (const std::string & name, double & value);  ///< Retrieve values of stream attributes (such as duration in seconds).

	VideoInFile * file;
  };

  /**
	 Video output stream.
  **/
  class VideoOut
  {
  public:
	VideoOut (const std::string & fileName, const std::string & formatName = "", const std::string & codecName = "");
	~VideoOut ();

	VideoOut & operator << (const Image & image);  ///< Insert next image frame.
	bool good () const;  ///< True as long as it is possible to write another frame to the stream.
	void set (const std::string & name, double value);  ///< Assign values to stream attributes (such as framerate).

	VideoOutFile * file;
  };

  /**
	 Interface for accessing a video file.  Video class uses this as a delegate.
  **/
  class VideoInFile
  {
  public:
	virtual ~VideoInFile ();

	virtual void seekFrame (int frame) = 0;  ///< Position stream just before the given frame.  Numbers follow same convention as Video class.
	virtual void seekTime (double timestamp) = 0;  ///< Position stream so that next frame will have the smallest timestamp >= the given timestamp.
	virtual void readNext (Image & image) = 0;  ///< Reads the next frame and stores it in image.  image may end up attached to a buffer used internally by the video device or library, so it may be freed unexpectedly.  However, this clss guarantees that the memory will not be freed before the next call to a method of this class.
	virtual bool good () const = 0;  ///< Indicates that the stream is open and the last read (if any) succeeded.
	virtual void setTimestampMode (bool frames = false) = 0;  ///< Changes image.timestamp from presentation time to frame number.
	virtual void get (const std::string & name, double & value) = 0;  ///< Retrieve values of stream attributes (such as duration in seconds).
  };

  class VideoOutFile
  {
  public:
	virtual ~VideoOutFile ();

	virtual void writeNext (const Image & image) = 0;  ///< Reads the next frame and stores it in image
	virtual bool good () const = 0;  ///< True if another frame can be written
	virtual void set (const std::string & name, double value) = 0;
  };

  class VideoFileFormat
  {
  public:
	VideoFileFormat ();
	virtual ~VideoFileFormat ();

	virtual VideoInFile * openInput (const std::string & fileName) const = 0;  ///< Creates a new VideoInFile attached to the given file and positioned before the first frame.  The caller is responsible to destroy the object.
	virtual VideoOutFile * openOutput (const std::string & fileName, const std::string & formatName, const std::string & codecName) const = 0;
	virtual float isIn (const std::string & fileName) const = 0;  ///< Determines probability [0,1] that this object handles the video format contained in the file.
	virtual float handles (const std::string & formatName, const std::string & codecName) const = 0;  ///< Determines probability [0,1] that this object handles the format with the given human readable name.

	static VideoFileFormat * find (const std::string & fileName);  ///< Determines what format the stream is in.
	static VideoFileFormat * find (const std::string & formatName, const std::string & codecName);  ///< Determines what format to use based on given name.

	static std::vector<VideoFileFormat *> formats;
  };


  // --------------------------------------------------------------------------
  // Support for FFMPEG.  Since this is probably the only library one would
  // ever need, it is the only one supported right now.
  // This could also be broken up into several classes to allow less of the
  // codecs or formats to be imported from the FFMPEG library.

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
	unsigned char * data;
	int gotPicture;  ///< indicates that picture contains a full image
	int state;  ///< state == 0 means good; anything else means we can't read more frames
	bool timestampMode;  ///< Indicates that image.timestamp should be frame number rather than presentation time.
	int64_t expectedSkew;  ///< How much before a given target time to read in order to get a keyframe, in stream units.
	bool seekLinear;  ///< Indicates that the file only supports linear seeking, not random seeking.  Generally due to lack of timestamps in stream.
	int64_t nextPTS;  ///< DTS of most recent packet pushed into decoder.  In MPEG, this will be the PTS of the next picture to come out of decoder, which occurs next time a packet is pushed in.
	double startTime;  ///< Best estimate of timestamp on first image in video.

	std::string fileName;
  };

  class VideoOutFileFFMPEG : public VideoOutFile
  {
  public:
	VideoOutFileFFMPEG (const std::string & fileName, const std::string & formatName, const std::string & codecName);
	~VideoOutFileFFMPEG ();

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

  class VideoFileFormatFFMPEG : public VideoFileFormat
  {
  public:
	VideoFileFormatFFMPEG ();

	virtual VideoInFile * openInput (const std::string & fileName) const;
	virtual VideoOutFile * openOutput (const std::string & fileName, const std::string & formatName, const std::string & codecName) const;
	virtual float isIn (const std::string & fileName) const;
	virtual float handles (const std::string & formatName, const std::string & codecName) const;
  };
}


#endif
