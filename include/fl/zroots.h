/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.2  2004/05/10 20:12:57  rothgang
Switch from using C99 style complex numbers to STL complex template.

Revision 1.1  2003/12/31 16:08:37  rothgang
Add the zroots function from Numerical Recipes in C.
-------------------------------------------------------------------------------
*/


#ifndef fl_zroots_h
#define fl_zroots_h


#include "fl/matrix.h"


namespace fl
{
  /**
	 The venerable "Numerical Recipes in C" method.
  **/
  void zroots (const Vector<std::complex<double> > & a, Vector<std::complex<double> > & roots, bool polish = true, bool sortroots = true);
  int laguer (const Vector<std::complex<double> > & a, std::complex<double> & x);  ///< Subroutine of zroots()
}


#endif
