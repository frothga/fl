/*
Author: Fred Rothganger


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include <fl/slideshow.h>
#include <fl/parms.h>
#include <fl/convolve.h>


using namespace fl;
using namespace std;


int
main (int argc, char * argv[])
{
  try
  {
	Parameters parms (argc, argv);
	int x = parms.getInt ("x", -1);
	int y = parms.getInt ("y", -1);
	int w = parms.getInt ("w", -1);
	int h = parms.getInt ("h", -1);

	if (parms.fileNames.size () < 1)
	{
	  cerr << "Usage: " << argv[0] << " {options} {image file names}" << endl;
	  cerr << "Options:" << endl;
	  cerr << "  x={horizontal start of sub-image} (default = 0)" << endl;
	  cerr << "  y={vertical start of sub-image} (default = 0)" << endl;
	  cerr << "  w={width of sub-image} (default = full width)" << endl;
	  cerr << "  h={height of sub-image} (default = full height)" << endl;
	  return 1;
	}

	// Internal formats
	new ImageFileFormatBMP;
	new ImageFileFormatPGM;
	new ImageFileFormatMatlab;
	new ImageFileFormatNITF;
	// External formats
#   ifdef HAVE_PNG
	new ImageFileFormatPNG;
#   endif
#   ifdef HAVE_JPEG
	new ImageFileFormatJPEG;
#   endif
#   ifdef HAVE_TIFF
	new ImageFileFormatTIFF;
#   endif


	SlideShow window;

	for (int i = 0; i < parms.fileNames.size (); i++)
	{
	  string fileName = parms.fileNames[i];
	  cerr << fileName << endl;
	  ImageFile f (fileName);
	  Image disp;
	  f.read (disp, x, y, w, h);
	  disp *= Rescale (disp);
	  window.show (disp);
	  window.waitForClick ();
	}
  }
  catch (const char * message)
  {
	cerr << "Exception: " << message << endl;
	return 1;
  }

  return 0;
}
