/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3 thru 1.5 Copyright 2005 Sandia Corporation.
Revisions 1.7 and 1.8  Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 11:38:05  Fred
Correct which revisions are under Sandia copyright.

Revision 1.7  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/09/26 04:21:43  Fred
Add detail to revision history.

Revision 1.4  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.3  2005/01/22 20:38:40  Fred
MSVC compilability fix: Use template<> to introduce template specializations. 
Change interface to frob() to use float rather than templated type.

Revision 1.2  2004/05/10 20:12:57  rothgang
Switch from using C99 style complex numbers to STL complex template.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_matrix_complex_tcc
#define fl_matrix_complex_tcc


#include "fl/matrix.h"


namespace fl
{
  template<>
  std::complex<double>
  MatrixAbstract<std::complex<double> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  template<>
  std::complex<double>
  Matrix<std::complex<double> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  template<>
  std::complex<float>
  MatrixAbstract<std::complex<float> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  template<>
  std::complex<float>
  Matrix<std::complex<float> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }
}


#endif
