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

Revision 1.4  2005/09/17 16:05:40  Fred
Add revision history.

Revision 1.3  2005/08/06 16:25:53  Fred
cygwin compilability fix: get crypt function from right place.

Revision 1.2  2005/04/23 19:40:47  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifdef __CYGWIN__
#include <crypt.h>
#else
#define _XOPEN_SOURCE
#include <unistd.h>
#endif

#include <time.h>
#include <stdio.h>


int
main (int argc, char * argv[])
{
  const char codes[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  int seed = time (NULL) % 4096;
  char salt[3];
  salt[0] = codes[seed & 0x3F];
  salt[1] = codes[seed >> 6];
  salt[2] = 0;

  printf ("%s\n", crypt (argv[1], salt));

  return 0;
}
