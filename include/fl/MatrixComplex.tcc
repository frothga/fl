#ifndef fl_matrix_complex_tcc
#define fl_matrix_complex_tcc


#include "fl/matrix.h"


namespace fl
{
  std::complex<double>
  MatrixAbstract<std::complex<double> >::frob (std::complex<double> n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  std::complex<double>
  Matrix<std::complex<double> >::frob (std::complex<double> n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  std::complex<float>
  MatrixAbstract<std::complex<float> >::frob (std::complex<float> n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  std::complex<float>
  Matrix<std::complex<float> >::frob (std::complex<float> n) const
  {
	throw "complex frobenius norm unimplemented";
  }
}


#endif
