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
