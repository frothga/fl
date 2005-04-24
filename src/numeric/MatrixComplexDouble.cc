/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
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

Factory<MatrixAbstract<complex<double> > >::productMap Factory<MatrixAbstract<complex<double> > >::products;
