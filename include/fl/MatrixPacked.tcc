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
  MatrixPacked<T>::write (std::ostream & stream, bool withName) const
  {
	if (withName)
	{
	  stream << typeid (*this).name () << std::endl;
	}
	stream.write ((char *) &rows_, sizeof (rows_));
	stream.write ((char *) data, sizeof (T) * (rows_ + 1) * rows_ / 2);
  }
}


#endif
