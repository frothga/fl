/*
Author: Fred Rothganger


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include <fl/slideshow.h>


using namespace fl;
using namespace std;


int
main (int argc, char * argv[])
{
  try
  {
	if (argc < 2)
	{
	  cerr << "Usage: " << argv[0] << " {image file names}" << endl;
	  return 1;
	}


	new ImageFileFormatPGM;
	new ImageFileFormatPNG;
	new ImageFileFormatJPEG;
	new ImageFileFormatTIFF;
	new ImageFileFormatMatlab;
	new ImageFileFormatNITF;


	SlideShow window;

	for (int i = 1; i < argc; i++)
	{
	  Image disp (argv[i]);
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
