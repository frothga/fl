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
  template class MatrixFixed<float,2,2>;
  template SHARED MatrixFixed<float,2,2> operator ! (const MatrixFixed<float,2,2> & A);
  template SHARED void geev (const MatrixFixed<float,2,2> & A, Matrix<float> & eigenvalues);
  template SHARED void geev (const MatrixFixed<float,2,2> & A, Matrix<std::complex<float> > & eigenvalues);
}
