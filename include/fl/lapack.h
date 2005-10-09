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
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_lapack_h
#define fl_lapack_h


#include "fl/matrix.h"


namespace fl
{
  /**
	 Compute eigenvalues and eigenvectors for symmetric matrix.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten by the eigenvectors if this is a dense Matrix.
	 \param eigenvectors This matrix gets aliased to A if A is a dense Matrix.
	 \param copy Indicates that A should not be overwritten, even if it
	 is a dense Matrix.  Forces A to be copied, which is less efficient.
  **/
  template<class T>
  void syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool copy = false);

  /**
	 Compute eigenvalues and eigenvectors for symmetric matrix stored in
	 packed format.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten.
  **/
  template<class T>
  void syev (const MatrixPacked<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors);

  /**
	 Compute eigenvalues (only) for symmetric matrix.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten if this is a dense Matrix.
	 \param copy Indicates that A should not be overwritten, even if it
	 is a dense Matrix.  Forces A to be copied, which is less efficient.
  **/
  template<class T>
  void syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, bool copy = false);

  /**
	 Symmetric generalized eigenvalue problem: A*x = lambda*B*x.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten by the eigenvectors if this is a dense Matrix.
	 \param B This parameter is marked as const for convenience, but
	 will in fact be overwritten by the triangular factor U or L from the
	 Cholesky factorization L*~L or ~U*U if this is a dense Matrix.
	 \param eigenvectors This matrix gets aliased to A if A is dense.
	 \param copy Indicates that A and B should not be overwritten, even if
	 they are dense Matrices.  Forces them to be copied, which is less
	 efficient.
   **/
  template<class T>
  void sygv (const MatrixAbstract<T> & A, const MatrixAbstract<T> & B, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool copy = false);

  /**
	 Compute eigenvalues and right eigenvectors for general (non-symmetric)
	 matrix.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten if this is a dense Matrix.
	 \param copy Indicates that A should not be overwritten, even if it
	 is a dense Matrix.  Forces A to be copied, which is less efficient.
  **/
  template<class T>
  void geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool copy = false);

  /**
	 Compute eigenvalues only for general (non-symmetric) matrix.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten if this is a dense Matrix.
	 \param copy Indicates that A should not be overwritten, even if it
	 is a dense Matrix.  Forces A to be copied, which is less efficient.
  **/
  template<class T>
  void geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, bool copy = false);

  /**
	 Compute eigenvalues and right eigenvectors for general (non-symmetric)
	 matrix.  This version returns the imaginary component of any complex
	 eigenvalues, rather than just ignoring it.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten if this is a dense Matrix.
	 \param copy Indicates that A should not be overwritten, even if it
	 is a dense Matrix.  Forces A to be copied, which is less efficient.
  **/
  template<class T>
  void geev (const MatrixAbstract<T> & A, Matrix<std::complex<T> > & eigenvalues, Matrix<T> & eigenvectors, bool copy = false);

  /**
	 Solve least squares problem using SVD via QR.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten by the right singular vectors if this is a
	 dense Matrix.
	 \param x This matrix will be aliased to b if b is dense and if A has
	 at least as many rows as columns.  Otherwise, x will be sized
	 appropriately by a fresh memory allocation and b will be copied (which
	 is less efficient).
	 \param b This parameter is marked as const for convenience, but
	 will in fact be overwritten by the solution x if this is a
	 dense Matrix and if A has at least as many rows as columns (that is,
	 if the problem is overconstrained or exact).
	 \param residual If nonzero, then compute the sum of squared error and
	 assign to the given address.  In zero, don't compute the residual.
	 Computes the residual by summing the squares of all the "extra" elements
	 in b that don't have associated elements in x, which only exist when A
	 has more rows than columns.  If A has equal or fewer rows than columns,
	 then the squared error is zero.
  **/
  template<class T>
  void gelss (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & b, T * residual = 0);

  /**
	 Solve least squares problem using divide and conquer.  LAPACK
	 documentation claims this is a faster implementation than gelss.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten by the right singular vectors if this is a
	 dense Matrix.
	 \param x This matrix will be aliased to b if b is dense and if A has
	 at least as many rows as columns.  Otherwise, x will be sized
	 appropriately by a fresh memory allocation and b will be copied (which
	 is less efficient).
	 \param b This parameter is marked as const for convenience, but
	 will in fact be overwritten by the solution x if this is a
	 dense Matrix and if A has at least as many rows as columns (that is,
	 if the problem is overconstrained or exact).
	 \param residual If nonzero, then compute the sum of squared error and
	 assign to the given address.  In zero, don't compute the residual.
	 Computes the residual by summing the squares of all the "extra" elements
	 in b that don't have associated elements in x, which only exist when A
	 has more rows than columns.  If A has equal or fewer rows than columns,
	 then the squared error is zero.
  **/
  template<class T>
  void gelsd (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & b, T * residual = 0);

  /**
	 Singular value decomposition on a general matrix.
	 \param A This parameter is marked as const for convenience (to allow
	 immediate construction of matrix classes within the call list), but
	 will in fact be overwritten if this is a dense Matrix.  Exactly what
	 overwrites A depends on jobu and jobvt.
	 \param copy Indicates that A should not be overwritten, even if it
	 is a dense Matrix.  Forces A to be copied, which is less efficient.
   **/
  template<class T>
  void gesvd (const MatrixAbstract<T> & A, Matrix<T> & U, Matrix<T> & S, Matrix<T> & VT, char jobu = 'S', char jobvt = 'S', bool copy = false);

  /**
	 Convenience function to avoid specifying job modes for gesvd().
   **/
  template<class T>
  void gesvd (const MatrixAbstract<T> & A, Matrix<T> & U, Matrix<T> & S, Matrix<T> & VT, bool copy)
  {
	gesvd (A, U, S, VT, 'S', 'S', copy);
  }


  // General non LAPACK operations the depend on LAPACK -----------------------

  /**
	 Returns the pseudoinverse of an arbitrary matrix.
  **/
  template<class T>
  Matrix<T> pinv (const MatrixAbstract<T> & A, T tolerance = -1, T epsilon = -1);

  /**
	 Compute the determinant of a square matrix.
   **/
  template<class T>
  T det (const MatrixAbstract<T> & A);

  /**
	 Estimate the rank of an arbitrary matrix using SVD.
  **/
  template<class T>
  int rank (const MatrixAbstract<T> & A, T threshold = -1, T epsilon = -1);
}


#endif
