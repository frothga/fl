/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/slideshow.h"
#include "fl/video.h"
#include "fl/parms.h"


using namespace std;
using namespace fl;


class EventPredicateMotion3 : public EventPredicate
{
public:
  EventPredicateMotion3 (XEvent & pattern) : pattern (pattern) {}

  virtual bool value (XEvent & event)
  {
	return    event.type         == MotionNotify
	       && event.xany.display == pattern.xany.display
	       && event.xany.window  == pattern.xany.window
	       && (event.xmotion.state & Button3Mask);
  }

  XEvent & pattern;
};


class VideoShow : public SlideShow
{
public:
  VideoShow (const string fileName)
  : vin (fileName)
  {
	useFrames = true;
	vin.setTimestampMode (useFrames);

	startTime = 0;
	vin.get ("startTime", startTime);
	duration = 0;
	vin.get ("duration", duration);

	playing = false;
	sizeSet = false;

	stem = fileName;
	stem = stem.substr (0, stem.find_last_of ('.'));
  }

  void showFrame ()
  {
	vin >> image;
	if (vin.good ())
	{
	  cerr << image.timestamp << endl;
	  show (image);
	}
  }

  virtual bool processEvent (XEvent & event)
  {
	switch (event.type)
	{
      case ClientMessage:
	  {
		if (event.xclient.message_type == WM_PROTOCOLS)
		{
		  if (event.xclient.data.l[0] == WM_DELETE_WINDOW)
		  {
			pause ();
			stopWaiting ();
			return true;
		  }
		}
		break;
	  }
      case ButtonPress:
	  {
		if (event.xbutton.button == Button1)
		{
		  if (playing)
		  {
			pause ();
		  }
		  else
		  {
			play ();
		  }
		}
		else if (event.xbutton.button == Button3)
		{
		  pause ();
		  double t = duration * event.xbutton.x / image.width;
		  t = max (0.0, min (duration, t));
		  vin.seekTime (startTime + t);
		  showFrame ();
		}
		else if (event.xbutton.button == Button4)
		{
		  pause ();
		  if (useFrames)
		  {
			vin.seekFrame (max (0, (int) image.timestamp - 1));
		  }
		  else
		  {
			vin.seekTime (image.timestamp - 1e-3);  // 1ms back in time is sufficient to catch previous frame
		  }
		  showFrame ();
		}
		else if (event.xbutton.button == Button5)
		{
		  pause ();
		  showFrame ();
		}
		return true;
	  }
	  case ButtonRelease:
	  {
		return true;  // Prevent SlideShow from getting this event and clearing "waitingForClick".
	  }
      case MotionNotify:
	  {
		if (event.xmotion.state & Button3Mask)
		{
		  EventPredicateMotion3 predicate (event);
		  bool found = false;
		  while (checkIfEvent (event, predicate)) found = true;
		  if (! found)
		  {
			pause ();
			double t = duration * event.xbutton.x / image.width;
			t = max (0.0, min (duration, t));
			vin.seekTime (startTime + t);
			showFrame ();
		  }
		}
		return true;
	  }
      case KeyPress:
	  {
		KeySym keysym = XLookupKeysym ((XKeyEvent *) &event, event.xkey.state);
		switch (keysym)
		{
		  case 'j':
		  {
			pause ();
			char buffer[1024];
			sprintf (buffer, "%s.frame%g.jpg", stem.c_str (), image.timestamp);
			cerr << "writing " << buffer << endl;
			image.write (buffer, "jpeg");
			return true;
		  }
		  case 'p':
		  {
			pause ();
			char buffer[1024];
			sprintf (buffer, "%s.frame%g.ppm", stem.c_str (), image.timestamp);
			cerr << "writing " << buffer << endl;
			image.write (buffer, "ppm");
			return true;
		  }
		  case 'f':
		  {
			useFrames = ! useFrames;
			vin.setTimestampMode (useFrames);
			if (! playing)
			{
			  if (useFrames)
			  {
				vin.seekTime (image.timestamp);
			  }
			  else
			  {
				vin.seekFrame ((int) image.timestamp);
			  }
			  showFrame ();
			}
			return true;
		  }
		  case 'q':
		  {
			pause ();
			stopWaiting ();
			return true;
		  }
		}
		return true;
	  }
	}

	return SlideShow::processEvent (event);
  }

  static void * playThread (void * data)
  {
	VideoShow * me = (VideoShow *) data;
	while (me->vin.good ()  &&  me->playing)
	{
	  me->vin >> me->image;
	  if (! me->vin.good ()) break;
	  cerr << me->image.timestamp << endl;
	  if (! me->sizeSet)
	  {
		cerr << "size = " << me->image.width << " " << me->image.height << endl;
		me->resize (me->image.width, me->image.height);
		me->width = me->image.width;
		me->height = me->image.height;
		me->sizeSet = true;
	  }
	  me->show (me->image);
	}
	if (! me->vin.good ()  &&  me->playing)
	{
	  // Figure a nice way to reopen file
	  VideoInFileFFMPEG * vin = (VideoInFileFFMPEG *) me->vin.file;
	  vin->state = 0;
	  vin->seekFrame (0);
	}
	me->playing = false;
  }

  void play ()
  {
	playing = true;
	if (pthread_create (&pidPlayThread, NULL, playThread, this) != 0)
	{
	  throw "Failed to start play thread.";
	}
  }

  void pause ()
  {
	if (playing)
	{
	  playing = false;
	  void * result;
	  pthread_join (pidPlayThread, &result);
	}
  }

  VideoIn vin;
  Image image;  // hides SlideShow::image
  bool useFrames;
  double startTime;
  double duration;
  string stem;
  pthread_t pidPlayThread;
  bool playing;
  bool sizeSet;
};

int
main (int argc, char * argv[])
{
  try
  {
	Parameters parms (argc, argv);
	int frame  = parms.getInt ("frame", 0);
	bool pause = parms.getInt ("pause", 0);

	if (parms.fileNames.size () < 1)
	{
	  cerr << "Usage: " << argv[0] << " {video filename} [parameters]" << endl;
	  cerr << "parameters:" << endl;
	  cerr << "  frame={frame number} (default = 0)" << endl;
	  cerr << "  pause={1 to freeze first frame, 0 otherwise} (default = 0)" << endl;
	}

	ImageFileFormatPGM   ::use ();
	ImageFileFormatJPEG  ::use ();
	VideoFileFormatFFMPEG::use ();

	VideoShow window (parms.fileNames[0]);
	if (frame > 0)
	{
	  window.vin.seekFrame (frame);
	}
	if (pause)
	{
	  window.showFrame ();
	}
	else
	{
	  window.play ();
	}
	window.waitForClick ();  // really "wait for close", since we override SlideShow's usage
  }
  catch (const char * error)
  {
	cerr << "Exception: " << error << endl;
	return 1;
  }

  return 0;
}
