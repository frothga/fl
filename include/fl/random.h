/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.2 thru 1.4 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/09/26 04:27:45  Fred
Add detail to revision history.

Revision 1.3  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/22 20:47:30  Fred
MSVC compilability fix:  Explicitly mark float constants.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef random_h
#define random_h


#include <stdlib.h>


namespace fl
{
  // Random number in [0, 1.0]
  inline float
  randf ()
  {
	return (float) rand () / RAND_MAX;
  }

  // Random number in [-1.0, 1.0]
  // "b" stands for "biased".
  inline float
  randfb ()
  {
	return 2.0f * rand () / RAND_MAX - 1.0f;
  }

  // Random number with a Gaussian distribution, mean of zero, and standard
  // deviation of 1.  Note that the range of this function is [-inf, inf].
  float randGaussian ();
}


#endif
