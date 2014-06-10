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


#define flNumeric_MS_EVIL
#include "fl/MatrixFloat.tcc"


using namespace fl;


template class MatrixAbstract<float>;
template class MatrixStrided<float>;
template class Matrix<float>;
template class MatrixTranspose<float>;
template class MatrixRegion<float>;

namespace fl
{
  template SHARED std::ostream & operator << (std::ostream & stream, const MatrixAbstract<float> & A);
  template SHARED std::istream & operator >> (std::istream & stream, MatrixAbstract<float> & A);
  template SHARED MatrixAbstract<float> & operator << (MatrixAbstract<float> & A, const std::string & source);
}
