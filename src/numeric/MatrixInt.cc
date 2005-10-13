/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
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
#include "fl/factory.h"


using namespace fl;


template class MatrixAbstract<int>;
template class Matrix<int>;
template class MatrixTranspose<int>;
template class MatrixRegion<int>;

template class Factory<MatrixAbstract<int> >;
template <> Factory<MatrixAbstract<int> >::productMap Factory<MatrixAbstract<int> >::products;

namespace fl
{
  template std::ostream & operator << (std::ostream & stream, const MatrixAbstract<int> & A);
  template std::istream & operator >> (std::istream & stream, MatrixAbstract<int> & A);
  template MatrixAbstract<int> & operator << (MatrixAbstract<int> & A, const std::string & source);
}
