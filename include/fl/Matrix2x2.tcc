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
}


#endif
