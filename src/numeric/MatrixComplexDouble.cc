/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


08/2005 Fred Rothganger -- Compilability fix for GCC 3.4.4
09/2005 Fred Rothganger -- Move stream operators from matrix.h to Matrix.tcc
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/Matrix.tcc"
#include "fl/MatrixComplex.tcc"
#include "fl/factory.h"


using namespace fl;
using namespace std;


template class MatrixAbstract<complex<double> >;
template class Matrix<complex<double> >;
template class MatrixTranspose<complex<double> >;
template class MatrixRegion<complex<double> >;

template class Factory<MatrixAbstract<complex<double> > >;
template <> Factory<MatrixAbstract<complex<double> > >::productMap Factory<MatrixAbstract<complex<double> > >::products;

namespace fl
{
  template std::ostream & operator << (std::ostream & stream, const MatrixAbstract<complex<double> > & A);
  template std::istream & operator >> (std::istream & stream, MatrixAbstract<complex<double> > & A);
  template MatrixAbstract<complex<double> > & operator << (MatrixAbstract<complex<double> > & A, const std::string & source);
}
