/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 and 1.6 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 10:57:29  Fred
Use CVS Log to generate revision history.

Revision 1.7  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/13 03:30:54  Fred
Add Sandia distribution terms.

Revision 1.5  2005/09/12 03:22:54  Fred
Fix Factory template instantiation for GCC 3.4.4.

Add Sandia copyright notice.  Must add license information before release.

Revision 1.4  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/04/19 17:09:09  rothgang
Remove factory method (use Factory template instead).  Provide default
implementations for read() and write().

Revision 1.2  2003/12/31 16:35:20  rothgang
Convert to fl namespace and add to library.

Revision 1.1  2003/12/23 18:57:25  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/12/23 18:57:25  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/cluster.h"
#include "fl/factory.h"


using namespace fl;
using namespace std;


// class ClusterMethod --------------------------------------------------------

template class Factory<ClusterMethod>;
template <> Factory<ClusterMethod>::productMap Factory<ClusterMethod>::products;

void
ClusterMethod::read (istream & stream)
{
}

void
ClusterMethod::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
