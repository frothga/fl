#ifndef fl_matrix3x3_tcc
#define fl_matrix3x3_tcc


#include "fl/matrix.h"


namespace fl
{
  // class Matrix3x3<T> -------------------------------------------------------

  template <class T>
  Matrix3x3<T>::Matrix3x3 ()
  {
  }

  template <class T>
  Matrix3x3<T>::Matrix3x3 (std::istream & stream)
  {
	read (stream);
  }

  template <class T>
  int
  Matrix3x3<T>::rows () const
  {
	return 3;
  }

  template <class T>
  int
  Matrix3x3<T>::columns () const
  {
	return 3;
  }

  template <class T>
  MatrixAbstract<T> *
  Matrix3x3<T>::duplicate () const
  {
	//return new Matrix3x3<T> (*this);

	// The following is dangerous hack.  It will fail in fairly obscure cases:
	// if you return a view (MatrixRegion or MatrixTranspose) of this matrix
	// from a function or if this matrix is an intermediate result and you
	// operate on a view of it.  These are atypical uses for Matrix3x3, but
	// are in fact the cases duplicate() was designed to solve, and do occur
	// often for the Matrix class.
	return new Matrix<T> (const_cast<T *> (&data[0][0]), 3, 3);
  }

  template<class T>
  void
  Matrix3x3<T>::resize (const int rows, const int columns)
  {
	if (rows != 3  ||  columns != 3)
	{
	  throw "Can't resize: matrix size is fixed at 3x3";
	}
  }

  template <class T>
  Matrix<T>
  Matrix3x3<T>::operator * (const MatrixAbstract<T> & B) const
  {
	int w = B.columns ();
	Matrix<T> result (3, w);
	T * i = (T *) result.data;
	for (int c = 0; c < w; c++)
	{
	  T & row0 = B (0, c);
	  T & row1 = B (1, c);
	  T & row2 = B (2, c);
	  *i++ = data[0][0] * row0 + data[1][0] * row1 + data[2][0] * row2;
	  *i++ = data[0][1] * row0 + data[1][1] * row1 + data[2][1] * row2;
	  *i++ = data[0][2] * row0 + data[1][2] * row1 + data[2][2] * row2;
	}
	return result;
  }

  template <class T>
  void
  Matrix3x3<T>::read (std::istream & stream)
  {
	stream.read ((char *) data, 9 * sizeof (T));
  }

  template <class T>
  void
  Matrix3x3<T>::write (std::ostream & stream, bool withName) const
  {
	if (withName)
	{
	  stream << typeid (*this).name () << std::endl;
	}
	stream.write ((char *) data, 9 * sizeof (T));
  }
}


#endif
