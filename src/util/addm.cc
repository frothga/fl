/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3 thru 1.6 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 11:06:58  Fred
Use CVS Log to generate revision history.

Revision 1.7  2005/10/14 17:00:43  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/14 16:50:11  Fred
Add Sandia distribution terms.

Revision 1.5  2005/09/17 16:04:07  Fred
Add revision history and Sandia copyright notice.  Must add license info before
release.

Revision 1.4  2005/08/06 16:24:05  Fred
Better to close input file as well.

Revision 1.3  2005/08/06 16:22:58  Fred
MS compatibility fix: close output file before renaming.

Revision 1.2  2005/04/23 19:40:47  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include <stdlib.h>
#include <stdio.h>


void
addm ()
{
  int character;
  while ((character = getchar ()) != EOF)
  {
	if (character == 10)
	{
	  putchar (13);
	}
	putchar (character);
  }
}

int
main (int argc, char * argv[])
{
  if (argc == 1)
  {
	addm ();
  }
  else
  {
	for (int i = 1; i < argc; i++)
	{
	  char outname[1024];
	  sprintf (outname, "%s_temp", argv[i]);

	  freopen (argv[i], "r", stdin);
	  freopen (outname, "w", stdout);

	  addm ();

	  fclose (stdin);
	  fclose (stdout);

	  char command[1024];
	  sprintf (command, "mv %s %s", outname, argv[i]);
	  system (command);
	}
  }

  return 0;
}
