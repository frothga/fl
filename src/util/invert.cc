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

Revision 1.4  2005/10/14 16:53:45  Fred
Add Sandia distribution terms.

Remove lapack.h as it is no longer neede to get operator !.  Include matrix.h
rather than Matrix.tcc since we are linking to the numeric library anyway.

Revision 1.3  2005/09/17 16:13:41  Fred
Link invert to FL_LIB, thanks to new arrangement of lapack in numeric code. 
Add Sandia copyright notice.  Must add license before release.

Revision 1.2  2005/04/23 19:40:47  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include <fl/matrix.h>


using namespace std;
using namespace fl;


int
main (int argc, char * argv[])
{
  Matrix<double> A;

  cin >> A;
  cout << !A << endl;

  return 0;
}
