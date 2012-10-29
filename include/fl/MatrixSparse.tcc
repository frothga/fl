/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
  MatrixSparse<T>::~MatrixSparse ()
  {
  }

  template<class T>
  uint32_t
  MatrixSparse<T>::classID () const
  {
	return MatrixAbstractID | MatrixSparseID;
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixSparse<T>::clone (bool deep) const
  {
	if (deep)
	{
	  MatrixSparse * result = new MatrixSparse;
	  result->copyFrom (*this);
	  return result;
	}
	return new MatrixSparse (*this);
  }

  template<class T>
  void
  MatrixSparse<T>::copyFrom (const MatrixAbstract<T> & that, bool deep)
  {
	if (that.classID () & MatrixSparseID)
	{
	  const MatrixSparse & MS = (const MatrixSparse &) (that);
	  if (deep)
	  {
		rows_ = MS.rows_;
		data.copyFrom (MS.data);  // performs deep copy of STL vector and map objects
	  }
	  else
	  {
		this->operator = (MS);
	  }
	}
	else
	{
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
  void
  MatrixSparse<T>::resize (const int rows, const int columns)
  {
	rows_ = rows;
	data->resize (columns);
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
  T
  MatrixSparse<T>::norm (float n) const
  {
	int w = data->size ();

	if (n == INFINITY)
	{
	  T result = (T) 0;
	  for (int c = 0; c < w; c++)
	  {
		std::map<int,T> & C = (*data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result = std::max (std::abs (i->second), result);
		  i++;
		}
	  }
	  return result;
	}
	else if (n == 0.0f)
	{
	  unsigned int result = 0;
	  for (int c = 0; c < w; c++)
	  {
		std::map<int,T> & C = (*data)[c];
		// Theoretically, we don't need to scan elements, since only
		// nonzero values are stored.  However, this isn't an absolute
		// guarantee.
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  if (i->second) result++;
		  i++;
		}
	  }
	  return (T) result;
	}
	else if (n == 1.0f)
	{
	  T result = (T) 0;
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
	  T result = (T) 0;
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
	  T result = (T) 0;
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
  MatrixResult<T>
  MatrixSparse<T>::transposeMultiply (const MatrixAbstract<T> & B) const
  {
	int m = this->data->size ();
	int n = B.columns ();
	Matrix<T> * result = new Matrix<T> (m, n);

	for (int c = 0; c < n; c++)
	{
	  for (int r = 0; r < m; r++)	
	  {
		register T temp = 0;
		std::map<int,T> & C = (*this->data)[r];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  temp += B(i->first,c) * i->second;
		  i++;
		}
		(*result)(r,c) = temp;
	  }
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixSparse<T>::operator * (const MatrixAbstract<T> & B) const
  {
	int w  = std::min ((int) this->data->size (), B.rows ());
	int bw = B.columns ();

	Matrix<T> * result = new Matrix<T> (this->rows_, bw);
	result->clear ();

	for (int c = 0; c < bw; c++)
	{
	  for (int k = 0; k < w; k++)
	  {
		T & b = B(k,c);
		std::map<int,T> & C = (*this->data)[k];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  (*result)(i->first,c) += i->second * b;
		  i++;
		}
	  }
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixSparse<T>::operator - (const MatrixAbstract<T> & B) const
  {
	if (! (B.classID () & MatrixSparseID)) return MatrixAbstract<T>::operator - (B);
	MatrixSparse & SB = (MatrixSparse &) B;

	int n = data->size ();
	MatrixSparse * result = new MatrixSparse (rows_, n);

	for (int c = 0; c < n; c++)
	{
	  std::map<int,T> & CR = (*result->data)[c];
	  std::map<int,T> & CA = (*data)[c];
	  std::map<int,T> & CB = (*SB.data)[c];
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
			result->rows_ = std::max (result->rows_, ia->first + 1);
		  }
		  ia++;
		  ib++;
		}
		else if (ia->first > ib->first)
		{
		  ir = CR.insert (ir, std::make_pair (ib->first, - ib->second));
		  result->rows_ = std::max (result->rows_, ib->first + 1);
		  ib++;
		}
		else  // ia->first < ib->first
		{
		  ir = CR.insert (ir, std::make_pair (ia->first, ia->second));
		  result->rows_ = std::max (result->rows_, ia->first + 1);
		  ia++;
		}
	  }
	}

	return result;
  }

  template<class T>
  void
  MatrixSparse<T>::serialize (Archive & archive, uint32_t version)
  {
	int n = data->size ();
	archive & n;

	if (archive.in)
	{
	  if (! archive.in->good ()) throw "MatrixSparse: can't finish reading because stream is bad";
	  data->resize (n);
	}

	for (int i = 0; i < n; i++)
	{
	  std::map<int,T> & C = (*data)[i];

	  int m = C.size ();
	  archive & m;

	  typename std::map<int,T>::iterator it = C.begin ();
	  if (archive.in)
	  {
		for (int j = 0; j < m; j++)
		{
		  int first;
		  T second;
		  archive.in->read ((char *) &first,  sizeof (first ));
		  archive.in->read ((char *) &second, sizeof (second));
		  it = C.insert (it, std::make_pair (first, second));
		  rows_ = std::max (rows_, first + 1);
		}
	  }
	  else
	  {
		while (it != C.end ())
		{
		  archive.out->write ((char *) & it->first,  sizeof (it->first ));
		  archive.out->write ((char *) & it->second, sizeof (it->second));
		  it++;
		}
	  }
	}
  }
}

#endif
