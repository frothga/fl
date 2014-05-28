/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009 Sandia Corporation.
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
	 \param eigenvectors Will be aliased to A if destroyA is true.
	 \param destroyA Indicates that A may be overwritten by the eigenvectors
	 if A is a dense Matrix.  The default is for A to be copied, which is
	 less efficient but allows A to be reused in other calculations.
  **/
  template<class T>
  SHARED void syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA = false);

  /**
	 Compute eigenvalues and eigenvectors for symmetric matrix stored in
	 packed format.
	 \param destroyA Indicates that A may be overwritten.  The default is
	 for A to be copied, which is less efficient but allows A to be reused
	 in other calculations.
  **/
  template<class T>
  SHARED void syev (const MatrixPacked<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA = false);

  /**
	 Compute eigenvalues (only) for symmetric matrix.
	 \param destroyA Indicates that A may be overwritten if A is a dense
	 Matrix.  The default is for A to be copied, which is less efficient
	 but allows A to be reused in other calculations.
  **/
  template<class T>
  SHARED void syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, bool destroyA = false);

  /**
	 Symmetric generalized eigenvalue problem: A*x = lambda*B*x.
	 \param destroyA Indicates that A may be overwritten if A is a dense
	 Matrix.  The default is for A to be copied, which is less efficient
	 but allows A to be reused in other calculations.
	 \param destroyB Indicates that B may be overwritten by the triangular
	 factor U or L from the Cholesky factorization L*~L or ~U*U if B is a
	 dense Matrix.  The default is for B to be copied.
	 \param eigenvectors This matrix gets aliased to A if destroyA is true
	 and A is dense.
   **/
  template<class T>
  SHARED void sygv (const MatrixAbstract<T> & A, const MatrixAbstract<T> & B, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA = false, bool destroyB = false);

  /**
	 Compute eigenvalues and right eigenvectors for general (non-symmetric)
	 matrix.  This version returns the imaginary component of any complex
	 eigenvalues, rather than just ignoring it.
	 \param destroyA Indicates that A may be overwritten if A is a dense
	 Matrix.  The default is for A to be copied, which is less efficient
	 but allows A to be reused in other calculations.
  **/
  template<class T>
  SHARED void geev (const MatrixAbstract<T> & A, Matrix<std::complex<T> > & eigenvalues, Matrix<T> & eigenvectors, bool destroyA = false);

  /**
	 Compute eigenvalues and right eigenvectors for general (non-symmetric)
	 matrix.
	 \param destroyA Indicates that A may be overwritten if A is a dense
	 Matrix.  The default is for A to be copied, which is less efficient
	 but allows A to be reused in other calculations.
  **/
  template<class T>
  SHARED void geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA = false);

  /**
	 Compute eigenvalues only for general (non-symmetric) matrix.
	 \param destroyA Indicates that A may be overwritten if A is a dense
	 Matrix.  The default is for A to be copied, which is less efficient
	 but allows A to be reused in other calculations.
  **/
  template<class T>
  SHARED void geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, bool destroyA = false);

  /**
	 Solve least squares problem using SVD via QR.
	 \param A Will be overwritten by the right singular vectors if destroyA
	 is true and this is a dense Matrix.
	 \param x This matrix will be aliased to B if destroyB is true AND B is
	 a dense Matrix AND A has at least as many rows as columns.  Otherwise,
	 x will be sized appropriately by a fresh memory allocation and B will
	 be copied (which is less efficient).
	 \param B Will be overwritten by the solution x under certain conditions.
	 See the parameters x and destroyB for details.
	 \param residual If nonzero, then compute the sum of squared error and
	 assign to the given address.  In zero, don't compute the residual.
	 Computes the residual by summing the squares of all the "extra" elements
	 in b that don't have associated elements in x, which only exist when A
	 has more rows than columns.  If A has equal or fewer rows than columns,
	 then the squared error is zero.
	 \param destroyA Indicates that A may be overwritten by the right singular
	 vectors if A is a dense Matrix.  The default is for A to be copied,
	 which is less efficient but allows A to be reused in other calculations.
	 \param destroyB Indicates that B may be overwritten by the solution x if
	 B is a dense Matrix.  The default is for B to be copied.
  **/
  template<class T>
  SHARED void gelss (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & B, T * residual = 0, bool destroyA = false, bool destroyB = false);

  /**
	 Solve least squares problem using divide and conquer.  LAPACK
	 documentation claims this is a faster implementation than gelss.
	 \param A Will be overwritten by the right singular vectors if destroyA
	 is true and this is a dense Matrix.
	 \param x This matrix will be aliased to B if destroyB is true AND B is
	 a dense Matrix AND A has at least as many rows as columns.  Otherwise,
	 x will be sized appropriately by a fresh memory allocation and B will
	 be copied (which is less efficient).
	 \param B Will be overwritten by the solution x under certain conditions.
	 See the parameters x and destroyB for details.
	 \param residual If nonzero, then compute the sum of squared error and
	 assign to the given address.  In zero, don't compute the residual.
	 Computes the residual by summing the squares of all the "extra" elements
	 in b that don't have associated elements in x, which only exist when A
	 has more rows than columns.  If A has equal or fewer rows than columns,
	 then the squared error is zero.
	 \param destroyA Indicates that A may be overwritten by the right singular
	 vectors if A is a dense Matrix.  The default is for A to be copied,
	 which is less efficient but allows A to be reused in other calculations.
	 \param destroyB Indicates that B may be overwritten by the solution x if
	 B is a dense Matrix.  The default is for B to be copied.
  **/
  template<class T>
  SHARED void gelsd (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & b, T * residual = 0, bool destroyA = false, bool destroyB = false);

  /**
	 Singular value decomposition on a general matrix.
	 \param A Will be overwritten if destroyA is true and this is a dense
	 Matrix.  Exactly what overwrites A depends on jobu and jobvt.
	 \param destroyA Indicates that A may be overwritten if A is a dense
	 Matrix.  The default is for A to be copied, which is less efficient
	 but allows A to be reused in other calculations.
   **/
  template<class T>
  SHARED void gesvd (const MatrixAbstract<T> & A, Matrix<T> & U, Matrix<T> & S, Matrix<T> & VT, char jobu = 'S', char jobvt = 'S', bool destroyA = false);

  /**
	 Convenience function to avoid specifying job modes for gesvd().
   **/
  template<class T>
  void gesvd (const MatrixAbstract<T> & A, Matrix<T> & U, Matrix<T> & S, Matrix<T> & VT, bool destroyA)
  {
	gesvd (A, U, S, VT, 'S', 'S', destroyA);
  }

  /// @todo This class should probably be in a separate header.
  template<class T>
  class SHARED Factorization
  {
  public:
	virtual ~Factorization () {}

	virtual void            factorize (const MatrixAbstract<T> & A, bool destroyA = false) = 0;
	virtual MatrixResult<T> solve     (const MatrixAbstract<T> & B, bool destroyB = false) = 0;
	virtual MatrixResult<T> invert    () = 0;  ///< Leaves internal structures in an undefined state. User should make another call to factorize() to continue using this object.
  };

  /// Triangle factorization of a general square matrix
  template<class T>
  class SHARED FactorizationGeneral : public Factorization<T>
  {
  public:
	virtual void            factorize (const MatrixAbstract<T> & A, bool destroyA = false);
	virtual MatrixResult<T> solve     (const MatrixAbstract<T> & B, bool destroyB = false);
	virtual MatrixResult<T> invert    ();

	Matrix<T> A;
	Vector<int> pivots;
  };

  /// Factor a symmetric indefinite matrix using Bunch-Kaufman
  template<class T>
  class SHARED FactorizationSymmetric : public Factorization<T>
  {
  public:
	virtual void            factorize (const MatrixAbstract<T> & A, bool destroyA = false);
	virtual MatrixResult<T> solve     (const MatrixAbstract<T> & B, bool destroyB = false);
	virtual MatrixResult<T> invert    ();

	Matrix<T> A;
	Vector<int> pivots;
  };


  // General non-LAPACK operations the depend on LAPACK -----------------------

  /**
	 Returns the pseudoinverse of an arbitrary matrix.
  **/
  template<class T>
  SHARED MatrixResult<T> pinv (const MatrixAbstract<T> & A, T tolerance = (T) -1, T epsilon = (T) -1);

  /**
	 Compute the determinant of a square matrix.
   **/
  template<class T>
  SHARED T det (const MatrixAbstract<T> & A);

  /**
	 Estimate the rank of an arbitrary matrix using SVD.
  **/
  template<class T>
  SHARED int rank (const MatrixAbstract<T> & A, T threshold = (T) -1, T epsilon = (T) -1);
}


#endif
