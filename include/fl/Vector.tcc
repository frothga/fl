#ifndef fl_vector_tcc
#define fl_vector_tcc


#include "fl/matrix.h"


namespace fl
{
  // class Vector<T> ----------------------------------------------------------

  template<class T>
  Vector<T>::Vector ()
  {
	rows_ = 0;
	columns_ = 0;
  }

  template<class T>
  Vector<T>::Vector (const int rows)
  {
	rows_ = rows;
	columns_ = 1;
	data.grow (rows * sizeof (T));
  }

  template<class T>
  Vector<T>::Vector (const MatrixAbstract<T> & that)
  {
	// Same code as assignment from MatrixAbstract

	if (   typeid (that) == typeid (*this)
	    || typeid (that) == typeid (Matrix<T>))
	{
	  *this = (const Matrix<T> &) that;
	}
	else
	{
	  int h = that.rows ();
	  int w = that.columns ();
	  resize (h, w);
	  T * i = (T *) data;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  *i++ = that(r,c);
		}
	  }
	}
	// Implicitly, a MatrixPacked will convert into a Vector with rows * rows
	// elements.  If that is the wrong behaviour, add another case here.
  }

  template<class T>
  Vector<T>::Vector (const Matrix<T> & that)
  {
	data = that.data;
	rows_ = that.rows_ * that.columns_;
	columns_ = 1;
  }

  template<class T>
  Vector<T>::Vector (std::istream & stream)
  {
	read (stream);
  }

  template<class T>
  Vector<T>::Vector (T * that, const int rows)
  {
	data.attach (that, rows * sizeof (T));
	rows_ = rows;
	columns_ = 1;
  }

  template<class T>
  Vector<T>::Vector (Pointer & that, const int rows)
  {
	data = that;
	columns_ = 1;
	if (rows < 0)  // infer number from size of memory block and size of our data type
	{
	  int size = data.size ();
	  if (size < 0)
	  {
		// Pointer does not know the size of memory block, so we pretend it is empty.  This is really an error condition.
		rows_ = 0;
	  }
	  else
	  {
		rows_ = size / sizeof (T);
	  }
	}
	else  // number of rows is given
	{
	  rows_ = rows;
	}
  }

  template<class T>
  Vector<T> &
  Vector<T>::operator = (const MatrixAbstract<T> & that)
  {
	// Same code as constructor taking MatrixAbstract

	if (   typeid (that) == typeid (*this)
	    || typeid (that) == typeid (Matrix<T>))
	{
	  *this = (Matrix<T> &) that;
	}
	else
	{
	  int h = that.rows ();
	  int w = that.columns ();
	  resize (h, w);
	  T * i = (T *) data;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  *i++ = that(r,c);
		}
	  }
	}
  }

  template<class T>
  Vector<T> &
  Vector<T>::operator = (const Matrix<T> & that)
  {
	data = that.data;
	rows_ = that.rows_ * that.columns_;
	columns_ = 1;
	return *this;
  }

  template<class T>
  MatrixAbstract<T> *
  Vector<T>::duplicate () const
  {
	return new Vector<T> (*this);
  }

  template<class T>
  void
  Vector<T>::resize (const int rows, const int columns)
  {
	Matrix<T>::resize (rows * columns, 1);
  }
}


#endif
