/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fixes for MSVC and Cygwin
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
  MatrixSparse<T>::write (std::ostream & stream, bool withName) const
  {
	MatrixAbstract<T>::write (stream, withName);

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
