/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.4 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/23 11:06:58  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/14 17:00:43  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/09/17 16:06:03  Fred
Add detail to revision history.

Revision 1.3  2005/04/23 19:37:40  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/12 05:24:40  rothgang
Set dpi.  Causes jpeg2ps to create bounding box proportional to number of
pixels in image.

Revision 1.1  2004/07/22 15:24:55  rothgang
Converts multiple eps files into jpeg.  My substitute for shell-scripting.
-------------------------------------------------------------------------------
*/


#include <stdlib.h>
#include <iostream>

using namespace std;


int
main (int argc, char * argv[])
{
  for (int i = 1; i < argc; i++)
  {
	string base = argv[i];
	int j = base.find_last_of ('.');
	base = base.substr (0, j);

	char line[1024];
	sprintf (line, "jpeg2ps -r 300 %s > %s.eps", argv[i], base.c_str ());
	cerr << line << endl;
	system (line);
  }
}
