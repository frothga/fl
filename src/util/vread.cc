#include "fl/slideshow.h"
#include "fl/video.h"
#include "fl/parms.h"


using namespace std;
using namespace fl;


class VideoShow : public SlideShow
{
public:
  VideoShow (const string fileName)
  : vin (fileName)
  {
	useFrames = true;
	vin.setTimestampMode (useFrames);

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
			waitingForClick = false;
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
		  double timestamp = 0;
		  vin.get ("duration", timestamp);
		  timestamp *= (double) event.xbutton.x / image.width;
		  vin.seekTime (timestamp);
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
		return true;
	  }
      case KeyPress:
	  {
		KeySym keysym = XLookupKeysym ((XKeyEvent *) &event, event.xkey.state);
		switch (keysym)
		{
		  case 'w':
		  {
			pause ();
			char buffer[1024];
			sprintf (buffer, "%s.frame%g.jpg", stem.c_str (), image.timestamp);
			cerr << "writing " << buffer << endl;
			image.write (buffer, "jpeg");
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
				vin.seekFrame ((int) image.timestamp);
			  }
			  else
			  {
				vin.seekTime (image.timestamp);
			  }
			  showFrame ();
			}
			return true;
		  }
		  case 'q':
		  {
			waitingForClick = false;
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
	while (me->vin.good ()  &&  me->playing  &&  me->waitingForClick)
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

	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	new VideoFileFormatFFMPEG;

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
