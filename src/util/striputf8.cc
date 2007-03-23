/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/03/23 11:06:58  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/14 17:00:43  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:40:47  Fred
Add UIUC copyright notice.

Revision 1.1  2004/07/22 15:23:55  rothgang
Like stripm, but works with files in UTF8 format that only contain ASCII
characters.  IE: the file is fluffed with alternating zeros.
-------------------------------------------------------------------------------
*/


#include <stdlib.h>
#include <stdio.h>


void
striputf8 ()
{
  int character;

  // Eat the endian indicator at start of file.  For now, just assume smalle endian.
  getchar ();
  getchar ();

  while ((character = getchar (), getchar ()) != EOF)
  {
	if (character != 13)
	{
	  putchar (character);
	}
  }
}

int
main (int argc, char * argv[])
{
  if (argc == 1)
  {
	striputf8 ();
  }
  else
  {
	for (int i = 1; i < argc; i++)
	{
	  char outname[1024];
	  sprintf (outname, "%s_temp", argv[i]);

	  freopen (argv[i], "r", stdin);
	  freopen (outname, "w", stdout);

	  striputf8 ();

	  char command[1024];
	  sprintf (command, "mv %s %s", outname, argv[i]);
	  system (command);
	}
  }

  return 0;
}
