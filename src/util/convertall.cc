/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
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
