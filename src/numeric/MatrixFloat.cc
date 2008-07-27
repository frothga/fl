/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/Matrix.tcc"
#include "fl/serialize.h"


using namespace fl;


template class MatrixAbstract<float>;
template class Matrix<float>;
template class MatrixTranspose<float>;
template class MatrixRegion<float>;

template class Factory<MatrixAbstract<float> >;

namespace fl
{
  template std::ostream & operator << (std::ostream & stream, const MatrixAbstract<float> & A);
  template std::istream & operator >> (std::istream & stream, MatrixAbstract<float> & A);
  template MatrixAbstract<float> & operator << (MatrixAbstract<float> & A, const std::string & source);
}
