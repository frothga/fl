/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#ifndef fl_matrix_identity_tcc
#define fl_matrix_identity_tcc


#include "fl/matrix.h"

#include <algorithm>


namespace fl
{
  // MatrixIdentity -----------------------------------------------------------

  template <class T>
  MatrixIdentity<T>::MatrixIdentity ()
  {
	size = 0;
	value = 1;
  }

  template <class T>
  MatrixIdentity<T>::MatrixIdentity (int size, T value)
  {
	this->size = size;
	this->value = value;
  }

  template <class T>
  T &
  MatrixIdentity<T>::operator () (const int row, const int column) const
  {
	if (row == column)
	{
	  return const_cast<T &> (value);
	}
	else
	{
	  static T zero;
	  zero = (T) 0;
	  return zero;
	}
  }

  template <class T>
  int
  MatrixIdentity<T>::rows () const
  {
	return size;
  }

  template <class T>
  int
  MatrixIdentity<T>::columns () const
  {
	return size;
  }

  template <class T>
  MatrixAbstract<T> *
  MatrixIdentity<T>::duplicate () const
  {
	return new MatrixIdentity (size, value);
  }

  template <class T>
  void
  MatrixIdentity<T>::clear (const T scalar)
  {
	value = scalar;
  }

  template <class T>
  void
  MatrixIdentity<T>::resize (const int rows, const int columns)
  {
	size = std::max (rows, columns);
  }
}


#endif
