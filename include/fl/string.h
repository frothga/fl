/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2 and 1.4 Copyright 2005 Sandia Corporation.
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

Revision 1.4  2005/09/26 04:28:46  Fred
Add detail to revision history.

Revision 1.3  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/22 20:48:27  Fred
MSVC compilability fix:  Handle difference in naming of case insensitive
strcmp().

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_string_h
#define fl_string_h


#include <ctype.h>
#include <string>


namespace fl
{
  inline void
  split (const std::string & source, char * delimiter, std::string & first, std::string & second)
  {
	int index = source.find (delimiter);
	if (index == std::string::npos)
	{
	  first = source;
	  second.erase ();
	}
	else
	{
	  std::string temp = source;
	  first = temp.substr (0, index);
	  second = temp.substr (index + strlen (delimiter));
	}
  }

  inline void
  trim (std::string & target)
  {
	int begin = target.find_first_not_of (' ');
	int end = target.find_last_not_of (' ');
	if (begin == std::string::npos)
	{
	  // all spaces
	  target.erase ();
	  return;
	}
	target = target.substr (begin, end - begin + 1);
  }

  inline void
  lowercase (std::string & target)
  {
	std::string::iterator i;
	for (i = target.begin (); i < target.end (); i++)
	{
	  *i = tolower (*i);
	}
  }
}

#ifdef _MSC_VER

namespace std
{
  inline int
  strcasecmp (const char * a, const char * b)
  {
	return _stricmp (a, b);
  }
}

#endif


#endif
