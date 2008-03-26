/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.3  2003/09/07 22:20:20  rothgang
Add real read and write methods to MatrixPacked.

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


#ifndef fl_matrix_packed_tcc
#define fl_matrix_packed_tcc


#include "fl/matrix.h"

#include <algorithm>


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
	if (typeid (that) == typeid (*this))
	{
	  *this = (MatrixPacked<T> &) (that);
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
  MatrixPacked<T>::duplicate () const
  {
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
	// We only copy the upper triangle
	resize (that.rows (), that.columns ());
	T * i = (T *) data;
	for (int c = 0; c < rows_; c++)
	{
	  for (int r = 0; r <= c; r++)
	  {
		*i++ = that (r, c);
	  }
	}
  }

  template<class T>
  void
  MatrixPacked<T>::copyFrom (const MatrixPacked<T> & that)
  {
	resize (that.rows ());
	//memcpy ((void *) data, (void *) that.data, sizeof (T) * (rows_ + 1) * rows_ / 2);
	data.copyFrom (that.data);
  }

  template<class T>
  MatrixPacked<T>
  MatrixPacked<T>::operator ~ () const
  {
	return (MatrixPacked<T>) *this;
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
