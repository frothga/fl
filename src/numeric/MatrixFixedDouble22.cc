/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/MatrixFixed.tcc"


namespace fl
{
  template class MatrixFixed<double,2,2>;
  template SHARED void geev (const MatrixFixed<double,2,2> & A, Matrix<double> & eigenvalues, bool destroyA);
  template SHARED void geev (const MatrixFixed<double,2,2> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors, bool destroyA);
}
