#include "fl/video.h"
#include "fl/parms.h"
#include "fl/convolve.h"

#include <stdlib.h>
#include <stdio.h>


using namespace std;
using namespace fl;


int
main (int argc, char * argv[])
{
  Parameters parms (argc, argv);
  float size = parms.getFloat ("size", 64);

  ImageFileFormatJPEG  ::use ();
  VideoFileFormatFFMPEG::use ();

  for (int i = 0; i < parms.fileNames.size (); i++)
  {
	string fileName = parms.fileNames[i];
	string stem = fileName.substr (0, fileName.find_last_of ('.'));
	cerr << fileName << endl;

	VideoIn vin (fileName);
	Image frame;
	vin >> frame;

	double ratio = size / frame.height;
	TransformGauss small (ratio, ratio);
	frame = frame * small;

	frame.write (stem + ".jpg", "jpeg");
  }

  return 0;
}
