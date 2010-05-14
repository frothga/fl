/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/MatrixFixed22.tcc"


namespace fl
{
  template class MatrixFixed<double,2,2>;
  template MatrixFixed<double,2,2> operator ! (const MatrixFixed<double,2,2> & A);
  template void geev (const MatrixFixed<double,2,2> & A, Matrix<double> & eigenvalues);
  template void geev (const MatrixFixed<double,2,2> & A, Matrix<std::complex<double> > & eigenvalues);
}
