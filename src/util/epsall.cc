/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
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
