/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5, 1.6, 1.8 Copyright 2005 Sandia Corporation.
Revisions 1.10 and 1.11 Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.11  2007/03/23 11:38:05  Fred
Correct which revisions are under Sandia copyright.

Revision 1.10  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.9  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.8  2005/09/26 04:21:59  Fred
Add detail to revision history.

Revision 1.7  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.6  2005/01/22 20:39:16  Fred
MSVC compilability fix: Change interface to frob() to use float rather than
templated type.

Revision 1.5  2005/01/12 04:59:59  rothgang
Use std versions of pow, sqrt, and fabs so that choice of type specific version
will be automatic when template is instantiated.  IE: std contains type
overloaded versions of the functions rather than separately named functions.

Revision 1.4  2003/12/30 16:49:37  rothgang
Add frob().

Revision 1.3  2003/08/11 14:17:12  rothgang
Changed interface for clear() so that it can take an arbitrary scalar rather
than just setting everything to zero.

Revision 1.2  2003/07/09 14:59:17  rothgang
Use SmartPointer rather than SparseBlock.  SmartPointer is essentially a
generalization of the idea behind SparseBlock (a structured Pointer).

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_matrix_sparse_tcc
#define fl_matrix_sparse_tcc


#include "fl/matrix.h"


namespace fl
{
  template<class T>
  MatrixSparse<T>::MatrixSparse ()
  {
	rows_ = 0;
	data.initialize ();
  }

  template<class T>
  MatrixSparse<T>::MatrixSparse (const int rows, const int columns)
  {
	data.initialize ();
	resize (rows, columns);
  }

  template<class T>
  MatrixSparse<T>::MatrixSparse (const MatrixAbstract<T> & that)
  {
	data.initialize ();
	int m = that.rows ();
	int n = that.columns ();
	resize (m, n);
	for (int c = 0; c < n; c++)
	{
	  for (int r = 0; r < m; r++)
	  {
		set (r, c, that(r,c));
	  }
	}
  }

  template<class T>
  MatrixSparse<T>::MatrixSparse (std::istream & stream)
  {
	data.initialize ();
	read (stream);
  }

  template<class T>
  MatrixSparse<T>::~MatrixSparse ()
  {
  }

  template<class T>
  void
  MatrixSparse<T>::set (const int row, const int column, const T value)
  {
	if (value == (T) 0)
	{
	  if (column < data->size ())
	  {
		(*data)[column].erase (row);
	  }
	}
	else
	{
	  if (row >= rows_)
	  {
		rows_ = row + 1;
	  }
	  if (column >= data->size ())
	  {
		data->resize (column + 1);
	  }
	  (*data)[column][row] = value;
	}
  }

  template<class T>
  T &
  MatrixSparse<T>::operator () (const int row, const int column) const
  {
	if (column < data->size ())
	{
	  std::map<int, T> & c = (*data)[column];
	  typename std::map<int, T>::iterator i = c.find (row);
	  if (i != c.end ())
	  {
		return i->second;
	  }
	}
	static T zero;
	zero = (T) 0;
	return zero;
  }

  template<class T>
  int
  MatrixSparse<T>::rows () const
  {
	return rows_;
  }

  template<class T>
  int
  MatrixSparse<T>::columns () const
  {
	return data->size ();
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixSparse<T>::duplicate () const
  {
	return new MatrixSparse (*this);
  }

  template<class T>
  void
  MatrixSparse<T>::clear (const T scalar)
  {
	typename std::vector< std::map<int,T> >::iterator i = data->begin ();
	while (i < data->end ())
	{
	  (i++)->clear ();
	}
  }

  template<class T>
  void
  MatrixSparse<T>::resize (const int rows, const int columns)
  {
	rows_ = rows;
	data->resize (columns);
  }

  template<class T>
  void
  MatrixSparse<T>::copyFrom (const MatrixSparse & that)
  {
	rows_ = that.rows_;
	data.copyFrom (that.data);  // performs deep copy of STL vector and map objects
  }

  template<class T>
  T
  MatrixSparse<T>::frob (float n) const
  {
	int w = data->size ();

	if (n == INFINITY)
	{
	  T result = (T) -INFINITY;
	  for (int c = 0; c < w; c++)
	  {
		std::map<int,T> & C = (*data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result = std::max (i->second, result);
		  i++;
		}
	  }
	  return result;
	}
	else if (n == 1.0f)
	{
	  T result = 0;
	  for (int c = 0; c < w; c++)
	  {
		std::map<int,T> & C = (*data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result += i->second;
		  i++;
		}
	  }
	  return result;
	}
	else if (n == 2.0f)
	{
	  T result = 0;
	  for (int c = 0; c < w; c++)
	  {
		std::map<int,T> & C = (*data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result += i->second * i->second;
		  i++;
		}
	  }
	  return (T) std::sqrt (result);
	}
	else
	{
	  T result = 0;
	  for (int c = 0; c < w; c++)
	  {
		std::map<int,T> & C = (*data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result += (T) std::pow (i->second, (T) n);
		  i++;
		}
	  }
	  return (T) std::pow (result, (T) (1.0f / n));
	}
  }

  template<class T>
  MatrixSparse<T>
  MatrixSparse<T>::operator - (const MatrixSparse<T> & B) const
  {
	int n = data->size ();
	MatrixSparse result (1, n);

	for (int c = 0; c < n; c++)
	{
	  std::map<int,T> & CR = (*result.data)[c];
	  std::map<int,T> & CA = (*data)[c];
	  std::map<int,T> & CB = (*B.data)[c];
	  typename std::map<int,T>::iterator ir = CR.begin ();
	  typename std::map<int,T>::iterator ia = CA.begin ();
	  typename std::map<int,T>::iterator ib = CB.begin ();
	  while (ia != CA.end ()  &&  ib != CB.end ())
	  {
		if (ia->first == ib->first)
		{
		  T t = ia->second - ib->second;
		  if (t != 0)
		  {
			ir = CR.insert (ir, std::make_pair (ia->first, t));
			result.rows_ = std::max (result.rows_, ia->first + 1);
		  }
		  ia++;
		  ib++;
		}
		else if (ia->first > ib->first)
		{
		  ir = CR.insert (ir, std::make_pair (ib->first, - ib->second));
		  result.rows_ = std::max (result.rows_, ib->first + 1);
		  ib++;
		}
		else  // ia->first < ib->first
		{
		  ir = CR.insert (ir, std::make_pair (ia->first, ia->second));
		  result.rows_ = std::max (result.rows_, ia->first + 1);
		  ia++;
		}
	  }
	}

	return result;
  }

  template<class T>
  void
  MatrixSparse<T>::read (std::istream & stream)
  {
	int n;
	stream.read ((char *) &n, sizeof (n));
	if (! stream.good ())
	{
	  throw "MatrixSparse: can't finish reading because stream is bad";
	}
	data->resize (n);

	for (int i = 0; i < n; i++)
	{
	  std::map<int,T> & C = (*data)[i];

	  int m;
	  stream.read ((char *) &m, sizeof (m));

	  typename std::map<int,T>::iterator insertionPoint = C.begin ();
	  for (int j = 0; j < m; j++)
	  {
		int first;
		T second;
		stream.read ((char *) &first, sizeof (first));
		stream.read ((char *) &second, sizeof (second));
		insertionPoint = C.insert (insertionPoint, std::make_pair (first, second));
		rows_ = std::max (rows_, first + 1);
	  }
	}
  }

  template<class T>
  void
  MatrixSparse<T>::write (std::ostream & stream) const
  {
	const int n = data->size ();
	stream.write ((char *) &n, sizeof (n));
	for (int i = 0; i < n; i++)
	{
	  std::map<int,T> & C = (*data)[i];

	  const int m = C.size ();
	  stream.write ((char *) &m, sizeof (m));

	  typename std::map<int,T>::iterator j = C.begin ();
	  while (j != C.end ())
	  {
		stream.write ((char *) & j->first, sizeof (j->first));
		stream.write ((char *) & j->second, sizeof (j->second));
		j++;
	  }
	}
  }

}

#endif
