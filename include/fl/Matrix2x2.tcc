/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


09/2005 Fred Rothganger -- Move geev from matrix.h
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix2x2_tcc
#define fl_matrix2x2_tcc


#include "fl/matrix.h"


namespace fl
{
  // class Matrix2x2<T> -------------------------------------------------------

  template <class T>
  Matrix2x2<T>::Matrix2x2 ()
  {
  }

  template <class T>
  Matrix2x2<T>::Matrix2x2 (std::istream & stream)
  {
	read (stream);
  }

  template <class T>
  int
  Matrix2x2<T>::rows () const
  {
	return 2;
  }

  template <class T>
  int
  Matrix2x2<T>::columns () const
  {
	return 2;
  }

  template <class T>
  MatrixAbstract<T> *
  Matrix2x2<T>::duplicate () const
  {
	// return new Matrix2x2<T> (*this);

	// See comments in Matrix3x3.tcc on why this is potentially buggy.
	return new Matrix<T> (const_cast<T *> (&data[0][0]), 2, 2);
  }

  template<class T>
  void
  Matrix2x2<T>::resize (const int rows, const int columns)
  {
	if (rows != 2  ||  columns != 2)
	{
	  throw "Can't resize: matrix size is fixed at 2x2";
	}
  }

  template <class T>
  Matrix2x2<T>
  Matrix2x2<T>::operator ! () const
  {
	Matrix2x2<T> result;
	T q = data[0][0] * data[1][1] - data[0][1] * data[1][0];
	if (q == 0)
	{
	  throw "invert: Matrix is singular!";
	}
	result.data[0][0] = data[1][1] / q;
	result.data[0][1] = data[0][1] / -q;
	result.data[1][0] = data[1][0] / -q;
	result.data[1][1] = data[0][0] / q;
	return result;
  }

  template <class T>
  Matrix2x2<T>
  Matrix2x2<T>::operator ~ () const
  {
	Matrix2x2<T> result;
	result.data[0][0] = data[0][0];
	result.data[0][1] = data[1][0];
	result.data[1][0] = data[0][1];
	result.data[1][1] = data[1][1];
	return result;
  }

  template <class T>
  Matrix<T>
  Matrix2x2<T>::operator * (const MatrixAbstract<T> & B) const
  {
	int h = B.rows ();
	int w = B.columns ();
	Matrix<T> result (2, w);
	T * i = (T *) result.data;
	for (int c = 0; c < w; c++)
	{
	  T & row0 = B (0, c);
	  T & row1 = B (1, c);
	  *i++ = data[0][0] * row0 + data[1][0] * row1;
	  *i++ = data[0][1] * row0 + data[1][1] * row1;
	}
	return result;
  }

  template<class T>
  Matrix2x2<T>
  Matrix2x2<T>::operator * (const Matrix2x2<T> & B) const
  {
	Matrix2x2<T> result;
	result.data[0][0] = data[0][0] * B.data[0][0] + data[1][0] * B.data[0][1];
	result.data[0][1] = data[0][1] * B.data[0][0] + data[1][1] * B.data[0][1];
	result.data[1][0] = data[0][0] * B.data[1][0] + data[1][0] * B.data[1][1];
	result.data[1][1] = data[0][1] * B.data[1][0] + data[1][1] * B.data[1][1];
	return result;
  }

  template<class T>
  Matrix2x2<T>
  Matrix2x2<T>::operator * (const T scalar) const
  {
	Matrix2x2<T> result;
	result.data[0][0] = data[0][0] * scalar;
	result.data[0][1] = data[0][1] * scalar;
	result.data[1][0] = data[1][0] * scalar;
	result.data[1][1] = data[1][1] * scalar;
	return result;
  }

  template<class T>
  Matrix2x2<T>
  Matrix2x2<T>::operator / (const T scalar) const
  {
	Matrix2x2<T> result;
	result.data[0][0] = data[0][0] / scalar;
	result.data[0][1] = data[0][1] / scalar;
	result.data[1][0] = data[1][0] / scalar;
	result.data[1][1] = data[1][1] / scalar;
	return result;
  }

  template <class T>
  Matrix2x2<T> &
  Matrix2x2<T>::operator *= (const Matrix2x2<T> & B)
  {
	T temp00   = data[0][0] * B.data[0][0] + data[1][0] * B.data[0][1];
	T temp01   = data[0][1] * B.data[0][0] + data[1][1] * B.data[0][1];
	data[1][0] = data[0][0] * B.data[1][0] + data[1][0] * B.data[1][1];
	data[1][1] = data[0][1] * B.data[1][0] + data[1][1] * B.data[1][1];
	data[0][0] = temp00;
	data[0][1] = temp01;
	return *this;
  }

  template <class T>
  MatrixAbstract<T> &
  Matrix2x2<T>::operator *= (const T scalar)
  {
	data[0][0] *= scalar;
	data[0][1] *= scalar;
	data[1][0] *= scalar;
	data[1][1] *= scalar;
	return *this;
  }

  template <class T>
  void
  Matrix2x2<T>::read (std::istream & stream)
  {
	stream.read ((char *) data, 4 * sizeof (T));
  }

  template <class T>
  void
  Matrix2x2<T>::write (std::ostream & stream, bool withName) const
  {
	if (withName)
	{
	  stream << typeid (*this).name () << std::endl;
	}
	stream.write ((char *) data, 4 * sizeof (T));
  }

  template<class T>
  void
  geev (const Matrix2x2<T> & A, Matrix<T> & eigenvalues)
  {
	// a = 1  :)
	T b = A.data[0][0] + A.data[1][1];  // trace
	T c = A.data[0][0] * A.data[1][1] - A.data[0][1] * A.data[1][0];  // determinant
	T b4c = b * b - 4 * c;
	if (b4c < 0)
	{
	  throw "eigen: no real eigenvalues!";
	}
	if (b4c > 0)
	{
	  b4c = sqrt (b4c);
	}
	eigenvalues.resize (2, 1);
	eigenvalues (0, 0) = (b - b4c) / 2.0;
	eigenvalues (1, 0) = (b + b4c) / 2.0;
  }

  template<class T>
  void
  geev (const Matrix2x2<T> & A, Matrix<std::complex<T> > & eigenvalues)
  {
	eigenvalues.resize (2, 1);

	// a = 1  :)
	T b = -(A.data[0][0] + A.data[1][1]);  // trace
	T c = A.data[0][0] * A.data[1][1] - A.data[0][1] * A.data[1][0];  // determinant
	T b4c = b * b - 4 * c;
	bool imaginary = b4c < 0;
	if (b4c != 0)
	{
	  b4c = sqrt (fabs (b4c));
	}
	if (imaginary)
	{
	  b /= -2.0;
	  b4c /= 2.0;
	  eigenvalues(0,0) = std::complex<T> (b, b4c);
	  eigenvalues(1,0) = std::complex<T> (b, -b4c);
	}
	else
	{
	  eigenvalues(0,0) = (-b - b4c) / T (2);
	  eigenvalues(1,0) = (-b + b4c) / T (2);
	}
  }
}


#endif
