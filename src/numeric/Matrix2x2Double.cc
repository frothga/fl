/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


09/2005 Fred Rothganger -- Moved geev() out of matrix.h into Matrix2x2.tcc
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/Matrix2x2.tcc"


namespace fl
{
  template class Matrix2x2<double>;
  template void geev (const Matrix2x2<double> & A, Matrix<double> & eigenvalues);
  template void geev (const Matrix2x2<double> & A, Matrix<std::complex<double> > & eigenvalues);
}
