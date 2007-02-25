/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/02/25 14:48:28  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/09 04:05:47  Fred
Put UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:38:49  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/random.h"

#include <math.h>


using namespace fl;


static bool haveNextGaussian = false;
static float nextGaussian;

float
fl::randGaussian ()
{
  if (haveNextGaussian)
  {
	haveNextGaussian = false;
	return nextGaussian;
  }
  else
  {
	float v1, v2, s;
	do
	{ 
	  v1 = randfb ();   // between -1.0 and 1.0
	  v2 = randfb ();
	  s = v1 * v1 + v2 * v2;
	}
	while (s >= 1 || s == 0);
	float multiplier = sqrt (- 2 * log (s) / s);
	nextGaussian = v2 * multiplier;
	haveNextGaussian = true;
	return v1 * multiplier;
  }
}
