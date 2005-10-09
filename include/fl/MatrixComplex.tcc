/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
*/


#ifndef fl_matrix_complex_tcc
#define fl_matrix_complex_tcc


#include "fl/matrix.h"


namespace fl
{
  template<>
  std::complex<double>
  MatrixAbstract<std::complex<double> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  template<>
  std::complex<double>
  Matrix<std::complex<double> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  template<>
  std::complex<float>
  MatrixAbstract<std::complex<float> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  template<>
  std::complex<float>
  Matrix<std::complex<float> >::frob (float n) const
  {
	throw "complex frobenius norm unimplemented";
  }
}


#endif
