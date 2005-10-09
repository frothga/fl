/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
08/2005 Fred Rothganger -- Compilability fix for GCC 3.4.4
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_vector_tcc
#define fl_vector_tcc


#include "fl/matrix.h"


namespace fl
{
  // class Vector<T> ----------------------------------------------------------

  template<class T>
  Vector<T>::Vector ()
  {
	this->rows_ = 0;
	this->columns_ = 0;
  }

  template<class T>
  Vector<T>::Vector (const int rows)
  {
	this->rows_ = rows;
	this->columns_ = 1;
	this->data.grow (rows * sizeof (T));
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
	  T * i = (T *) this->data;
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
	this->data = that.data;
	this->rows_ = that.rows_ * that.columns_;
	this->columns_ = 1;
  }

  template<class T>
  Vector<T>::Vector (std::istream & stream)
  {
	this->read (stream);
  }

  template<class T>
  Vector<T>::Vector (T * that, const int rows)
  {
	this->data.attach (that, rows * sizeof (T));
	this->rows_ = rows;
	this->columns_ = 1;
  }

  template<class T>
  Vector<T>::Vector (Pointer & that, const int rows)
  {
	this->data = that;
	this->columns_ = 1;
	if (rows < 0)  // infer number from size of memory block and size of our data type
	{
	  int size = this->data.size ();
	  if (size < 0)
	  {
		// Pointer does not know the size of memory block, so we pretend it is empty.  This is really an error condition.
		this->rows_ = 0;
	  }
	  else
	  {
		this->rows_ = size / sizeof (T);
	  }
	}
	else  // number of rows is given
	{
	  this->rows_ = rows;
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
	  T * i = (T *) this->data;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  *i++ = that(r,c);
		}
	  }
	}

	return *this;
  }

  template<class T>
  Vector<T> &
  Vector<T>::operator = (const Matrix<T> & that)
  {
	this->data = that.data;
	this->rows_ = that.rows_ * that.columns_;
	this->columns_ = 1;
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
