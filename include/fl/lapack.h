/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved implementations into numeric lib and switched
        to single template declaration for both float and double versions.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
*/


#ifndef fl_lapack_h
#define fl_lapack_h


#include "fl/matrix.h"


namespace fl
{
  /**
	 Compute eigenvalues and eigenvectors for symmetric matrix
  **/
  template<class T>
  void syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors);

  template<class T>
  void syev (const MatrixPacked<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors);

  /**
	 Compute eigenvalues (only) for symmetric matrix.
  **/
  template<class T>
  void syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues);

  /**
	 Symmetric generalized eigenvalue problem.
   **/
  template<class T>
  void sygv (const Matrix<T> & A, Matrix<T> & B, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors);

  /**
	 Compute eigenvalues and eigenvectors for general (non-symmetric) matrix
  **/
  template<class T>
  void geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors);

  template<class T>
  void geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues);

  template<class T>
  void geev (const MatrixAbstract<T> & A, Matrix<std::complex<T> > & eigenvalues, Matrix<T> & eigenvectors);

  /**
	 Solve least squares problem using SVD via QR
  **/
  template<class T>
  int gelss (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & b, const T rcond, Matrix<T> & s);

  template<class T>
  inline int
  gelss (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & b, const T rcond = -1)
  {
	Matrix<T> s;
	return gelss (A, x, b, rcond, s);
  }

  template<class T>
  void gesvd (const MatrixAbstract<T> & A, Matrix<T> & U, Matrix<T> & S, Matrix<T> & VT, char jobu = 'A', char jobvt = 'A');


  // General non LAPACK operations the depend on LAPACK -----------------------

  /**
	 Returns the pseudoinverse of any matrix A
  **/
  template<class T>
  Matrix<T> pinv (const Matrix<T> & A, T tolerance = -1, T epsilon = -1);

  template<class T>
  T det (const Matrix<T> & A);

  template<class T>
  int rank (const Matrix<T> & A, T threshold = -1, T epsilon = -1);
}


#endif
