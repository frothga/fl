/*
Author: Fred Rothganger
Created 2/29/08 to replace Matrix2x2.tcc and Matrix3x3.tcc

Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix_fixed_tcc
#define fl_matrix_fixed_tcc


#include "fl/matrix.h"


namespace fl
{
  // class MatrixFixed<T,R,C> -------------------------------------------------

  template<class T, int R, int C>
  MatrixFixed<T,R,C>::MatrixFixed ()
  {
  }

  template<class T, int R, int C>
  MatrixFixed<T,R,C>::MatrixFixed (std::istream & stream)
  {
	read (stream);
  }

  template<class T, int R, int C>
  int
  MatrixFixed<T,R,C>::rows () const
  {
	return R;
  }

  template<class T, int R, int C>
  int
  MatrixFixed<T,R,C>::columns () const
  {
	return C;
  }

  template<class T, int R, int C>
  MatrixAbstract<T> *
  MatrixFixed<T,R,C>::duplicate () const
  {
	return new MatrixFixed<T,R,C> (*this);
  }

  template<class T, int R, int C>
  void
  MatrixFixed<T,R,C>::resize (const int rows, const int columns)
  {
	assert (rows == R  &&  columns == C);
  }

  template<class T, int R, int C>
  MatrixFixed<T,C,R>
  MatrixFixed<T,R,C>::operator ~ () const
  {
	MatrixFixed<T,C,R> result;
	for (int c = 0; c < C; c++)
	{
	  for (int r = 0; r < R; r++)
	  {
		result.data[r][c] = data[c][r];
	  }
	}
	return result;
  }

  template<class T, int R, int C>
  Matrix<T>
  MatrixFixed<T,R,C>::operator * (const MatrixAbstract<T> & B) const
  {
	int w = std::min (C, B.rows ());
	int bw = B.columns ();
	Matrix<T> result (R, bw);
	T * ri = (T *) result.data;
	for (int c = 0; c < bw; c++)
	{
	  for (int r = 0; r < R; r++)
	  {
		const T * i = &data[0][r];
		register T element = (T) 0;
		for (int j = 0; j < w; j++)
		{
		  element += (*i) * B (j, c);
		  i += R;
		}
		*ri++ = element;
	  }
	}
	return result;
  }

  template<class T, int R, int C>
  MatrixFixed<T,R,C>
  MatrixFixed<T,R,C>::operator * (const MatrixFixed<T,R,C> & B) const
  {
	const int w = std::min (C, R);
	Matrix<T> result (R, C);
	T * ri = result.data;
	for (int c = 0; c < C; c++)
	{
	  for (int r = 0; r < R; r++)
	  {
		const T * i   = &  data[0][r];
		const T * bi  = &B.data[c][0];
		const T * end = bi + w;
		register T element = (T) 0;
		while (bi < end)
		{
		  element += (*i) * (*bi++);
		  i += R;
		}
		*ri++ = element;
	  }
	}
	return result;
  }

  template<class T, int R, int C>
  MatrixFixed<T,R,C>
  MatrixFixed<T,R,C>::operator * (const T scalar) const
  {
	MatrixFixed<T,R,C> result;
	const T * i = (T *) data;
	T * o       = (T *) result.data;
	T * end     = o + R * C;
	while (o < end) *o++ = *i++ * scalar;
	return result;
  }

  template<class T, int R, int C>
  MatrixFixed<T,R,C>
  MatrixFixed<T,R,C>::operator / (const T scalar) const
  {
	MatrixFixed<T,R,C> result;
	const T * i = (T *) data;
	T * o       = (T *) result.data;
	T * end     = o + R * C;
	while (o < end) *o++ = *i++ / scalar;
	return result;
  }

  template<class T, int R, int C>
  MatrixFixed<T,R,C> &
  MatrixFixed<T,R,C>::operator *= (const MatrixFixed<T,R,C> & B)
  {
	const int w = std::min (C, R);
	T temp[C][R];
	T * ri = &temp[0][0];
	for (int c = 0; c < C; c++)
	{
	  for (int r = 0; r < R; r++)
	  {
		T * i         = &  data[0][r];
		const T * bi  = &B.data[c][0];
		const T * end = bi + w;
		register T element = (T) 0;
		while (bi < end)
		{
		  element += (*i) * (*bi++);
		  i += R;
		}
		*ri++ = element;
	  }
	}
	memcpy (data, temp, R * C * sizeof (T));
	return *this;
  }

  template<class T, int R, int C>
  MatrixAbstract<T> &
  MatrixFixed<T,R,C>::operator *= (const T scalar)
  {
	T * i = (T *) data;
	T * end = i + R * C;
	while (i < end) *i++ *= scalar;
	return *this;
  }

  template<class T, int R, int C>
  void
  MatrixFixed<T,R,C>::read (std::istream & stream)
  {
	stream.read ((char *) data, R * C * sizeof (T));
  }

  template<class T, int R, int C>
  void
  MatrixFixed<T,R,C>::write (std::ostream & stream) const
  {
	stream.write ((char *) data, R * C * sizeof (T));
  }
}


#endif
