#include "fl/slideshow.h"
#include "fl/video.h"
#include "fl/parms.h"


using namespace std;
using namespace fl;


int
main (int argc, char * argv[])
{
  try
  {
	Parameters parms (argc, argv);

	float time     = parms.getFloat ("time",      -1);
	int frame      = parms.getInt   ("frame",     -1);
	string write   = parms.getChar  ("write",     "");
	bool show      = parms.getInt   ("show",      1);
	bool pause     = parms.getInt   ("pause",     0);
	bool useFrames = parms.getInt   ("useFrames", frame >= 0);

	if (parms.fileNames.size () < 1)
	{
	  cerr << "Usage: " << argv[0] << " {filename} [parameters]" << endl;
	  cerr << "  This program is a crude video player." << endl;
	  cerr << "  You can seek a position in the video file with one of these options:" << endl;
	  cerr << "    time={seconds}" << endl;
	  cerr << "    frame={number}" << endl;
	  cerr << "  You can dump a specified frame to disc with:" << endl;
	  cerr << "    write={g | any other character}" << endl;
	  cerr << "      g means write a b/w image to {filename}.frame.pgm" << endl;
	  cerr << "      any other character means write color image to {filename}.frame.jpg" << endl;
	  cerr << "  You can turn off the window with:" << endl;
	  cerr << "    show=0" << endl;
	  cerr << "  To single-step (by clicking or pressing a key) in show mode:" << endl;
	  cerr << "    pause=1" << endl;
	}

	SlideShow * window = 0;
	if (show)
	{
	  window = new SlideShow;
	}

	new ImageFileFormatPGM;
	new ImageFileFormatJPEG;
	new VideoFileFormatFFMPEG;

	VideoIn vin (parms.fileNames[0]);
	vin.setTimestampMode (useFrames);
	//VideoInFileFFMPEG * ff = (VideoInFileFFMPEG *) vin.file;
	//cerr << "state = " << ff->state << endl;
	if (time > 0)
	{
	  vin.seekTime (time);
	}
	else if (frame > 0)
	{
	  vin.seekFrame (frame);
	}

	Image image;
	while (vin.good ())
	{
	  vin >> image;
	  cerr << image.timestamp << " " << image.width << " " << image.height << endl;

	  if (write.size ())
	  {
		string stem = parms.fileNames[0];
		stem = stem.substr (0, stem.find_last_of ('.'));

		if (write == "g")
		{
		  (image * GrayChar).write (stem + ".frame.pgm", "pgm");
		}
		else
		{
		  image.write (stem + ".frame.jpg", "jpeg");
		}
		exit (0);
	  }

	  if (window)
	  {
		window->show (image);
		if (pause)
		{
		  window->waitForClick ();
		}
	  }
	}

	if (window)
	{
	  delete window;
	}
  }
  catch (const char * error)
  {
	cerr << "Exception: " << error << endl;
	return 1;
  }

  return 0;
}
