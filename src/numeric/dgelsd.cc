/*
Author: Fred Rothganger
Created 10/4/2005 to take advantage of a faster implementation of gels in
LAPACK.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/lapack.tcc"


namespace fl
{
  template SHARED void gelsd (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & B, double * residual, bool destroyA, bool destroyB);
}
