/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix_packed_tcc
#define fl_matrix_packed_tcc


#include "fl/matrix.h"

#include <algorithm>
#include <typeinfo>


namespace fl
{
  // class MatrixPacked<T> ----------------------------------------------------

  template<class T>
  MatrixPacked<T>::MatrixPacked ()
  {
	rows_ = 0;
  }

  template<class T>
  MatrixPacked<T>::MatrixPacked (const int rows)
  {
	rows_ = 0;
	resize (rows, rows);
  }

  template<class T>
  MatrixPacked<T>::MatrixPacked (const MatrixAbstract<T> & that)
  {
	if (that.classID ()  &  MatrixPackedID)
	{
	  *this = (const MatrixPacked &) that;
	}
	else
	{
	  rows_ = 0;
	  copyFrom (that);
	}
  }

  template<class T>
  MatrixPacked<T>::MatrixPacked (std::istream & stream)
  {
	read (stream);
  }

  template<class T>
  uint32_t
  MatrixPacked<T>::classID () const
  {
	return MatrixAbstractID | MatrixPackedID;
  }

  template<class T>
  T &
  MatrixPacked<T>::operator () (const int row, const int column) const
  {
	if (row <= column)
	{
	  return ((T *) data)[row + (column + 1) * (column) / 2];
	}
	else
	{
	  return ((T *) data)[column + (row + 1) * (row) / 2];
	}
  }

  template<class T>
  T &
  MatrixPacked<T>::operator [] (const int row) const
  {
	return ((T *) data)[row];
  }

  template<class T>
  int
  MatrixPacked<T>::rows () const
  {
	return rows_;
  }

  template<class T>
  int
  MatrixPacked<T>::columns () const
  {
	return rows_;
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixPacked<T>::duplicate (bool deep) const
  {
	if (deep)
	{
	  MatrixPacked * result = new MatrixPacked;
	  result->copyFrom (*this);
	  return result;
	}
	return new MatrixPacked<T> (*this);
  }

  template<class T>
  void
  MatrixPacked<T>::clear (const T scalar)
  {
	if (scalar == (T) 0)
	{
	  data.clear ();
	}
	else
	{
	  T * i = (T *) data;
	  T * end = i + (rows_ + 1) * rows_ / 2;
	  while (i < end)
	  {
		*i++ = scalar;
	  }
	}	  
  }

  template<class T>
  void
  MatrixPacked<T>::resize (const int rows, const int columns)
  {
	int newrows = columns > 0 ? std::min (rows, columns) : rows;
	if (rows_ != newrows)
	{
	  rows_ = newrows;
	  data.grow (sizeof (T) * (rows_ + 1) * rows_ / 2);
	}
  }

  template<class T>
  void
  MatrixPacked<T>::copyFrom (const MatrixAbstract<T> & that)
  {
	if (that.classID () & MatrixPackedID)
	{
	  const MatrixPacked & MP = (const MatrixPacked &) that;
	  resize (MP.rows_);
	  data.copyFrom (MP.data);
	}
	else
	{
	  // We only copy the upper triangle
	  resize (that.rows (), that.columns ());
	  T * i = (T *) data;
	  for (int c = 0; c < rows_; c++)
	  {
		for (int r = 0; r <= c; r++)
		{
		  *i++ = that(r,c);
		}
	  }
	}
  }

  template<class T>
  MatrixResult<T>
  MatrixPacked<T>::operator ~ () const
  {
	return new MatrixPacked<T> (*this);
  }

  template<class T>
  void
  MatrixPacked<T>::read (std::istream & stream)
  {
	stream.read ((char *) &rows_, sizeof (rows_));
	if (! stream.good ())
	{
	  throw "Stream bad.  Unable to finish reading matrix.";
	}
	int bytes = sizeof (T) * (rows_ + 1) * rows_ / 2;
	data.grow (bytes);
	stream.read ((char *) data, bytes);
  }

  template<class T>
  void
  MatrixPacked<T>::write (std::ostream & stream) const
  {
	stream.write ((char *) &rows_, sizeof (rows_));
	stream.write ((char *) data, sizeof (T) * (rows_ + 1) * rows_ / 2);
  }
}


#endif
