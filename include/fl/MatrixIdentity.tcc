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
