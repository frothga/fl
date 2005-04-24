/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/Matrix.tcc"
#include "fl/factory.h"


using namespace fl;


template class MatrixAbstract<int>;
template class Matrix<int>;
template class MatrixTranspose<int>;
template class MatrixRegion<int>;

Factory<MatrixAbstract<int> >::productMap Factory<MatrixAbstract<int> >::products;
