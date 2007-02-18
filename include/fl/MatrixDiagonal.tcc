/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.2  2003/08/11 14:17:12  rothgang
Changed interface for clear() so that it can take an arbitrary scalar rather
than just setting everything to zero.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_matrix_diagonal_tcc
#define fl_matrix_diagonal_tcc


#include "fl/matrix.h"

#include <algorithm>


namespace fl
{
  // MatrixDiagonal -----------------------------------------------------------

  template <class T>
  MatrixDiagonal<T>::MatrixDiagonal ()
  {
	rows_ = 0;
	columns_ = 0;
  }

  template <class T>
  MatrixDiagonal<T>::MatrixDiagonal (const int rows, const int columns)
  {
	rows_ = rows;
	if (columns == -1)
	{
	  columns_ = rows_;
	}
	else
	{
	  columns_ = columns;
	}
	data.grow (std::min (rows_, columns_) * sizeof (T));
  }

  template <class T>
  MatrixDiagonal<T>::MatrixDiagonal (const Vector<T> & that, const int rows, const int columns)
  {
	if (rows == -1)
	{
	  rows_ = that.rows ();
	}
	else
	{
	  rows_ = rows;
	}
	if (columns == -1)
	{
	  columns_ = rows_;
	}
	else
	{
	  columns_ = columns;
	}

	data = that.data;
  }

  template <class T>
  T &
  MatrixDiagonal<T>::operator () (const int row, const int column) const
  {
	if (row == column)
	{
	  return ((T *) data)[row];
	}
	else
	{
	  static T zero;
	  zero = (T) 0;
	  return zero;
	}
  }

  template <class T>
  T &
  MatrixDiagonal<T>::operator [] (const int row) const
  {
	return ((T *) data)[row];
  }

  template <class T>
  int
  MatrixDiagonal<T>::rows () const
  {
	return rows_;
  }

  template <class T>
  int
  MatrixDiagonal<T>::columns () const
  {
	return columns_;
  }

  template <class T>
  MatrixAbstract<T> *
  MatrixDiagonal<T>::duplicate () const
  {
	return new MatrixDiagonal (*this);
  }

  template <class T>
  void
  MatrixDiagonal<T>::clear (const T scalar)
  {
	if (scalar == (T) 0)
	{
	  data.clear ();
	}
	else
	{
	  T * i = (T *) data;
	  T * end = i + std::min (rows_, columns_);
	  while (i < end)
	  {
		*i++ = scalar;
	  }
	}	  
  }

  template <class T>
  void
  MatrixDiagonal<T>::resize (const int rows, const int columns)
  {
	rows_ = rows;
	if (columns == -1)
	{
	  columns_ = rows_;
	}
	else
	{
	  columns_ = columns;
	}
	data.grow (std::min (rows_, columns_) * sizeof (T));
  }
}


#endif
