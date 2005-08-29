/*
Author: Fred Rothganger
Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
*/


#include <fl/slideshow.h>


using namespace fl;
using namespace std;


int
main (int argc, char * argv[])
{
  if (argc < 2)
  {
	cerr << "Usage: " << argv[0] << " {image file names}" << endl;
	return 1;
  }


  new ImageFileFormatPGM;
  new ImageFileFormatJPEG;
  new ImageFileFormatTIFF;


  SlideShow window;

  for (int i = 1; i < argc; i++)
  {
	Image disp (argv[i]);
	window.show (disp);
	window.waitForClick ();
  }

  return 0;
}
