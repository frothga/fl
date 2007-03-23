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

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include <stdlib.h>
#include <iostream>

using namespace std;


int
main (int argc, char * argv[])
{
  if (argc < 3)
  {
	cerr << "Usage: " << argv[0] << " <target suffix> file1 file2 ..." << endl;
	return 1;
  }
  for (int i = 2; i < argc; i++)
  {
	string base = argv[i];
	int j = base.find_last_of ('.');
	base = base.substr (0, j);

	char line[1024];
	sprintf (line, "convert %s %s.%s", argv[i], base.c_str (), argv[1]);
	cerr << line << endl;
	system (line);
  }
}
