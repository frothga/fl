/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2 thru 1.5 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/09 03:28:17  Fred
Update revision history and add Sandia copyright notice.

Revision 1.4  2005/05/29 21:55:41  Fred
Add TWOPI, as well as explicit float variants of PI and TWOPI.  Use these
constants to better express the mod2pi operations.

Revision 1.3  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/22 20:46:31  Fred
MSVC compilability fix:  Add cast to maintain float type.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_pi_h
#define fl_pi_h


#include <math.h>

#define PI     3.1415926535897932384626433832795029
#define PIf    3.1415926535897932384626433832795029f
#define TWOPI  6.283185307179586476925286766559
#define TWOPIf 6.283185307179586476925286766559f


inline double
mod2pi (double a)
{
  a = fmod (a, TWOPI);
  if (a < 0)
  {
	a += TWOPI;
  }
  return a;
}

inline float
mod2pi (float a)
{
  a = fmodf (a, TWOPIf);
  if (a < 0)
  {
	a += TWOPIf;
  }
  return a;
}


#endif
