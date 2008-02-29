/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.3 Copyright 2005 Sandia Corporation.
Revisions Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 10:57:27  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/10/13 03:38:13  Fred
Move geev() out of matrix.h into Matrix2x2.tcc.  Add Sandia copyright notice.

Revision 1.2  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/MatrixFixed22.tcc"


namespace fl
{
  template class MatrixFixed<float,2,2>;
  template MatrixFixed<float,2,2> operator ! (const MatrixFixed<float,2,2> & A);
  template void geev (const MatrixFixed<float,2,2> & A, Matrix<float> & eigenvalues);
  template void geev (const MatrixFixed<float,2,2> & A, Matrix<std::complex<float> > & eigenvalues);
}
