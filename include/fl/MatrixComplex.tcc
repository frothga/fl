#ifndef fl_matrix_complex_tcc
#define fl_matrix_complex_tcc


#include "fl/matrix.h"
#include "fl/complex.h"


namespace fl
{
  complex double
  MatrixAbstract<complex double>::frob (complex double n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  complex double
  Matrix<complex double>::frob (complex double n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  complex float
  MatrixAbstract<complex float>::frob (complex float n) const
  {
	throw "complex frobenius norm unimplemented";
  }

  complex float
  Matrix<complex float>::frob (complex float n) const
  {
	throw "complex frobenius norm unimplemented";
  }
}


#endif
