/*
Author: Fred Rothganger


Rebisions 1.1 and 1.2 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.3  2007/03/23 11:06:58  Fred
Use CVS Log to generate revision history.

Revision 1.2  2005/10/14 16:54:11  Fred
Add Sandia distribution terms.

Revision 1.1  2005/08/28 21:14:03  Fred
Utility to measure difference between two video files.
-------------------------------------------------------------------------------
*/


#include "fl/video.h"
#include "fl/parms.h"
#include "fl/math.h"


using namespace std;
using namespace fl;


int
main (int argc, char * argv[])
{
  try
  {
	Parameters parms (argc, argv);
	int limit = parms.getInt ("limit", 0);

	if (parms.fileNames.size () != 2)
	{
	  cerr << "Usage: " << argv[0] << " [parameters] {video file 1} {video file 2}" << endl;
	  cerr << "  limit = {max numbers of frames to check} (default = all)" << endl;
	  return 1;
	}

	new VideoFileFormatFFMPEG;

	VideoIn vin1 (parms.fileNames[0]);
	VideoIn vin2 (parms.fileNames[1]);

	double sse = 0;
	double count = 0;

	Image image1;
	Image image2;
	int i = 0;
	while (vin1.good ()  &&  vin2.good ())
	{
	  vin1 >> image1;
	  vin2 >> image2;
	  if (! vin1.good ()  ||  ! vin2.good ()) break;
	  if (image1.width != image2.width  ||  image1.height != image2.height)
	  {
		throw "Images not same size";
	  }

	  ImageOf<float> gray1 = image1 * GrayFloat;
	  ImageOf<float> gray2 = image2 * GrayFloat;

	  float * f1 = & gray1(0,0);
	  float * f2 = & gray2(0,0);
	  int pixels = gray1.width * gray1.height;
	  float * e = f1 + pixels;
	  count += pixels;
	  while (f1 < e)
	  {
		float d = *f1++ - *f2++;
		sse += d * d;
	  }

	  cerr << ".";

	  i++;
	  if (limit > 0  &&  i > limit) break;
	}
	cerr << endl;
	double rms = sqrt (sse / count);

	cerr << "psnr = " << 20 * log (1.0 / rms) / log (10.0) << endl;
  }
  catch (const char * error)
  {
	cerr << "Exception: " << error << endl;
	return 1;
  }

  return 0;
}
