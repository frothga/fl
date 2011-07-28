/*
Author: Fred Rothganger
Created 9/10/2005 to complement the float version of the same routine.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/lapack.tcc"


namespace fl
{
  template SHARED void sygv (const MatrixAbstract<double> & A, const MatrixAbstract<double> & B, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors, bool destroyA, bool destroyB);
}
