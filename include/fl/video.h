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


#ifndef fl_video_h
#define fl_video_h


#include "fl/image.h"

#undef SHARED
#ifdef _MSC_VER
#  ifdef flImage_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


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
  class SHARED VideoIn : public Metadata
  {
  public:
	VideoIn (const std::string & fileName);
	~VideoIn ();

	void open (const std::string & fileName);
	void close ();
	void pause ();  ///< If this is a network stream, then temporarily suspend streaming. The next call to operator>>() will restart streaming.
	void seekFrame (int frame);  ///< Position stream just before the given frame.  Numbers are zero based.  (Maybe they should be one-based.  Research the convention.)
	void seekTime (double timestamp);  ///< Position stream so that next frame will have the smallest timestamp >= the given timestamp.
	VideoIn & operator >> (Image & image);  ///< Extract next image frame.  image may end up attached to a buffer used internally by the video device or library, so it may be freed unexpectedly.  However, this clss guarantees that the memory will not be freed before the next call to a method of this class.
	bool good () const;  ///< Indicates that the stream is open and the last read (if any) succeeded.
	void setTimestampMode (bool frames = false);  ///< Changes image.timestamp from presentation time to frame number.

	virtual void get (const std::string & name,       std::string & value);
	virtual void set (const std::string & name, const std::string & value);
	using Metadata::get;
	using Metadata::set;

	VideoInFile * file;
  };

  /**
	 Video output stream.
  **/
  class SHARED VideoOut : public Metadata
  {
  public:
	VideoOut (const std::string & fileName, const std::string & formatName = "", const std::string & codecName = "");
	~VideoOut ();

	VideoOut & operator << (const Image & image);  ///< Insert next image frame.
	bool good () const;  ///< True as long as it is possible to write another frame to the stream.

	virtual void get (const std::string & name,       std::string & value);
	virtual void set (const std::string & name, const std::string & value);
	using Metadata::get;
	using Metadata::set;

	VideoOutFile * file;
  };

  /**
	 Interface for accessing a video file.  Video class uses this as a delegate.
  **/
  class SHARED VideoInFile
  {
  public:
	virtual ~VideoInFile ();

	virtual void pause () = 0;
	virtual void seekFrame (int frame) = 0;
	virtual void seekTime (double timestamp) = 0;
	virtual void readNext (Image & image) = 0;
	virtual bool good () const = 0;
	virtual void setTimestampMode (bool frames = false) = 0;
	virtual void get (const std::string & name,       std::string & value) = 0;
	virtual void set (const std::string & name, const std::string & value) = 0;
  };

  class SHARED VideoOutFile
  {
  public:
	virtual ~VideoOutFile ();

	virtual void writeNext (const Image & image) = 0;  ///< Reads the next frame and stores it in image
	virtual bool good () const = 0;  ///< True if another frame can be written
	virtual void get (const std::string & name,       std::string & value) = 0;
	virtual void set (const std::string & name, const std::string & value) = 0;
  };

  class SHARED VideoFileFormat
  {
  public:
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

  class SHARED VideoFileFormatFFMPEG : public VideoFileFormat
  {
  public:
	VideoFileFormatFFMPEG ();
	static void use ();

	virtual VideoInFile * openInput (const std::string & fileName) const;
	virtual VideoOutFile * openOutput (const std::string & fileName, const std::string & formatName, const std::string & codecName) const;
	virtual float isIn (const std::string & fileName) const;
	virtual float handles (const std::string & formatName, const std::string & codecName) const;
  };
}


#endif
