/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3 and 1.4 Copyright 2005 Sandia Corporation.
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

Revision 1.4  2005/09/17 16:05:30  Fred
Add revision history.

Revision 1.3  2005/08/06 16:24:44  Fred
Get include file from proper (general) location.

Revision 1.2  2005/04/23 19:40:47  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#define BLOCKSIZE 4096
#define BLOCKCOUNT 65536


#include <fl/time.h>

#include <iostream>

using namespace std;
using namespace fl;


int main (int argc, char * argv[])
{
  char block[BLOCKSIZE];
  FILE * file;

  double starttime = getTimestamp ();

  if (argc < 2)
  {
	cerr << "Starting write test" << endl;
	double writetime = getTimestamp ();
	file = fopen ("timing.tmp", "w");
	for (int i = 0; i < BLOCKCOUNT; i++)
	{
	  fwrite (block, BLOCKSIZE, 1, file);
	}
	fsync (fileno (file));
	fclose (file);
	cerr << "Done writing: " << getTimestamp () - writetime << endl;
  }

  cerr << "Starting read test" << endl;
  double readtime = getTimestamp ();
  file = fopen ("timing.tmp", "r");
  for (int i = 0; i < BLOCKCOUNT; i++)
  {
	fread (block, BLOCKSIZE, 1, file);
  }
  fsync (fileno (file));
  fclose (file);
  cerr << "Done reading: " << getTimestamp () - readtime << endl;

  cerr << "Total: " << getTimestamp () - starttime << endl;

  return 0;
}
