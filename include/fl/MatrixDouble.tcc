/*
Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix_double_tcc
#define fl_matrix_double_tcc


#include "fl/Matrix.tcc"
#include "fl/lapack.h"


namespace fl
{
# ifdef HAVE_LAPACK
  template<>
  MatrixResult<double>
  MatrixAbstract<double>::operator ! () const
  {
	if (rows () != columns ()) return pinv (*this);
	FactorizationGeneral<double> f;
	f.factorize (*this);
	return f.invert ();
  }
# endif
}


#endif
