/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix_tcc
#define fl_matrix_tcc


#include "fl/matrix.h"
#include "fl/string.h"

#include <algorithm>
#include <typeinfo>


namespace fl
{
  // class MatrixAbstract<T> --------------------------------------------------

  template<class T>
  int MatrixAbstract<T>::displayWidth = 10;

  template<class T>
  int MatrixAbstract<T>::displayPrecision = 6;

  template<class T>
  MatrixAbstract<T>::~MatrixAbstract ()
  {
  }

  template<class T>
  uint32_t
  MatrixAbstract<T>::classID () const
  {
	return MatrixAbstractID;
  }

  template<class T>
  T &
  MatrixAbstract<T>::operator [] (const int row) const
  {
	return (*this) (row / rows (), row % rows ());
  }

  template<class T>
  int
  MatrixAbstract<T>::rows () const
  {
	return 0;
  }

  template<class T>
  int
  MatrixAbstract<T>::columns () const
  {
	return 0;
  }

  template<class T>
  void
  MatrixAbstract<T>::clear (const T scalar)
  {
	int h = rows ();
	int w = columns ();
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		(*this)(r,c) = scalar;
	  }
	}
  }

  template<class T>
  void
  MatrixAbstract<T>::copyFrom (const MatrixAbstract<T> & that)
  {
	int h = that.rows ();
	int w = that.columns ();
	resize (h, w);
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		(*this)(r,c) = that(r,c);
	  }
	}
  }

  template<class T>
  T
  MatrixAbstract<T>::norm (float n) const
  {
	int h = rows ();
	int w = columns ();
	if (n == INFINITY)
	{
	  T result = (T) 0;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  result = std::max (std::abs ((*this)(r,c)), result);
		}
	  }
	  return result;
	}
	else if (n == 0.0f)
	{
	  unsigned int result = 0;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  if (std::abs ((*this)(r,c)) > 0) result++;
		}
	  }
	  return (T) result;
	}
	else if (n == 1.0f)
	{
	  T result = (T) 0;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  result += (*this) (r, c);
		}
	  }
	  return result;
	}
	else if (n == 2.0f)
	{
	  T result = (T) 0;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  T t = (*this) (r, c);
		  result += t * t;
		}
	  }
	  return (T) std::sqrt (result);
	}
	else
	{
	  T result = (T) 0;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  result += (T) std::pow ((*this) (r, c), (T) n);
		}
	  }
	  return (T) std::pow (result, (T) (1.0 / n));
	}
  }

  template<class T>
  T
  MatrixAbstract<T>::sumSquares () const
  {
	int h = rows ();
	int w = columns ();
	T result = 0;
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		T t = (*this) (r, c);
		result += t * t;
	  }
	}
	return result;
  }

  template<class T>
  void
  MatrixAbstract<T>::normalize (const T scalar)
  {
	T length = norm (2);
	if (length != (T) 0)
	{
	  (*this) /= length;
	  // It is less efficient to separate these operations, but more
	  // numerically stable.
	  if (scalar != (T) 1)
	  {
		(*this) *= scalar;
	  }
	}
  }

  template<class T>
  T
  MatrixAbstract<T>::dot (const MatrixAbstract<T> & B) const
  {
	int h = std::min (rows (), B.rows ());
	register T result = (T) 0;
	for (int r = 0; r < h; r++) result += (*this)(r,1) * B(r,1);
	return result;
  }

  template<class T>
  void
  MatrixAbstract<T>::identity (const T scalar)
  {
	clear ();
	int last = std::min (rows (), columns ());
	for (int i = 0; i < last; i++)
	{
	  (*this)(i,i) = scalar;
	}
  }

  template<class T>
  MatrixRegion<T>
  MatrixAbstract<T>::row (const int r) const
  {
	return MatrixRegion<T> (*this, r, 0, r, columns () - 1);
  }

  template<class T>
  MatrixRegion<T>
  MatrixAbstract<T>::column (const int c) const
  {
	return MatrixRegion<T> (*this, 0, c, rows () - 1, c);
  }

  template<class T>
  MatrixRegion<T>
  MatrixAbstract<T>::region (const int firstRow, const int firstColumn, int lastRow, int lastColumn) const
  {
	return MatrixRegion<T> (*this, firstRow, firstColumn, lastRow, lastColumn);
  }

  template<class T>
  const char *
  MatrixAbstract<T>::toString (std::string & buffer) const
  {
	std::ostringstream stream;
	stream << *this;
	buffer = stream.str ();
	return buffer.c_str ();
  }

  template<class T>
  bool
  MatrixAbstract<T>::operator == (const MatrixAbstract<T> & B) const
  {
	int h = rows ();
	int w = columns ();
	if (B.rows () != h  ||  B.columns () != w)
	{
	  return false;
	}
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		if (B(r,c) != (*this)(r,c))
		{
		  return false;
		}
	  }
	}
	return true;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator ~ () const
  {
	return new MatrixTranspose<T> (this->duplicate ());
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator ^ (const MatrixAbstract<T> & B) const
  {
	// This version is only good for 3 element vectors.  Need to choose
	// a cross-product hack for higher dimensions

	Matrix<T> * result = new Matrix<T> (3);
	(*result)[0] = (*this)[1] * B[2] - (*this)[2] * B[1];
	(*result)[1] = (*this)[2] * B[0] - (*this)[0] * B[2];
	(*result)[2] = (*this)[0] * B[1] - (*this)[1] * B[0];

	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator & (const MatrixAbstract<T> & B) const
  {
	int h = rows ();
	int w = columns ();
	int oh = std::min (h, B.rows ());
	int ow = std::min (w, B.columns ());
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < ow; c++)
	{
	  for (int r = 0;  r < oh; r++) (*result)(r,c) = (*this)(r,c) * B(r,c);
	  for (int r = oh; r < h;  r++) (*result)(r,c) = (*this)(r,c);
	}
	for (int c = ow; c < w; c++)
	{
	  for (int r = 0; r < h; r++)   (*result)(r,c) = (*this)(r,c);
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator * (const MatrixAbstract<T> & B) const
  {
	int w = std::min (columns (), B.rows ());
	int h = rows ();
	int bw = B.columns ();
	Matrix<T> * result = new Matrix<T> (h, bw);
	for (int c = 0; c < bw; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		register T element = (T) 0;
		for (int i = 0; i < w; i++)
		{
		  element += (*this)(r,i) * B(i,c);
		}
		(*result)(r,c) = element;
	  }
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator * (const T scalar) const
  {
	int h = rows ();
	int w = columns ();
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		(*result)(r,c) = (*this)(r,c) * scalar;
	  }
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator / (const MatrixAbstract<T> & B) const
  {
	int h = rows ();
	int w = columns ();
	int oh = std::min (h, B.rows ());
	int ow = std::min (w, B.columns ());
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < ow; c++)
	{
	  for (int r = 0;  r < oh; r++) (*result)(r,c) = (*this)(r,c) / B(r,c);
	  for (int r = oh; r < h;  r++) (*result)(r,c) = (*this)(r,c);
	}
	for (int c = ow; c < w; c++)
	{
	  for (int r = 0; r < h; r++)   (*result)(r,c) = (*this)(r,c);
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator / (const T scalar) const
  {
	int h = rows ();
	int w = columns ();
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		(*result)(r,c) = (*this)(r,c) / scalar;
	  }
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator + (const MatrixAbstract<T> & B) const
  {
	int h = rows ();
	int w = columns ();
	int oh = std::min (h, B.rows ());
	int ow = std::min (w, B.columns ());
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < ow; c++)
	{
	  for (int r = 0;  r < oh; r++) (*result)(r,c) = (*this)(r,c) + B(r,c);
	  for (int r = oh; r < h;  r++) (*result)(r,c) = (*this)(r,c);
	}
	for (int c = ow; c < w; c++)
	{
	  for (int r = 0; r < h; r++)   (*result)(r,c) = (*this)(r,c);
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator + (const T scalar) const
  {
	int h = rows ();
	int w = columns ();
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		(*result)(r,c) = (*this)(r,c) + scalar;
	  }
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator - (const MatrixAbstract<T> & B) const
  {
	int h = rows ();
	int w = columns ();
	int oh = std::min (h, B.rows ());
	int ow = std::min (w, B.columns ());
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < ow; c++)
	{
	  for (int r = 0;  r < oh; r++) (*result)(r,c) = (*this)(r,c) - B(r,c);
	  for (int r = oh; r < h;  r++) (*result)(r,c) = (*this)(r,c);
	}
	for (int c = ow; c < w; c++)
	{
	  for (int r = 0; r < h; r++)   (*result)(r,c) = (*this)(r,c);
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixAbstract<T>::operator - (const T scalar) const
  {
	int h = rows ();
	int w = columns ();
	Matrix<T> * result = new Matrix<T> (h, w);
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		(*result)(r,c) = (*this)(r,c) - scalar;
	  }
	}
	return result;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator ^= (const MatrixAbstract<T> & B)
  {
	copyFrom ((*this) ^ B);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator &= (const MatrixAbstract<T> & B)
  {
	copyFrom ((*this) & B);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator *= (const MatrixAbstract<T> & B)
  {
	copyFrom ((*this) * B);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator *= (const T scalar)
  {
	copyFrom ((*this) * scalar);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator /= (const MatrixAbstract<T> & B)
  {
	copyFrom ((*this) / B);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator /= (const T scalar)
  {
	copyFrom ((*this) / scalar);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator += (const MatrixAbstract<T> & B)
  {
	copyFrom ((*this) + B);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator += (const T scalar)
  {
	copyFrom ((*this) + scalar);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator -= (const MatrixAbstract<T> & B)
  {
	copyFrom ((*this) - B);
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixAbstract<T>::operator -= (const T scalar)
  {
	copyFrom ((*this) - scalar);
	return *this;
  }

  template<class T>
  void
  MatrixAbstract<T>::read (std::istream & stream)
  {
  }

  template<class T>
  void
  MatrixAbstract<T>::write (std::ostream & stream) const
  {
  }

  /**
	 Relies on ostream to absorb the variability in the type T.
	 This function should be specialized for char (since ostreams treat
	 chars as characters, not numbers).
  **/
  template<class T>
  std::string elementToString (const T & value)
  {
	std::ostringstream formatted;
	formatted.precision (MatrixAbstract<T>::displayPrecision);
	formatted << value;
	return formatted.str ();
  }

  template<class T>
  T elementFromString (const std::string & value)
  {
	return (T) atof (value.c_str ());
  }

  template<class T>
  std::ostream &
  operator << (std::ostream & stream, const MatrixAbstract<T> & A)
  {
	const int rows = A.rows ();
	const int columns = A.columns ();

	std::string line = columns > 1 ? "[" : "~[";
	int r = 0;
	while (true)
	{
	  int c = 0;
	  while (true)
	  {
		line += elementToString (A(r,c));
		if (++c >= columns) break;
		line += ' ';
		while (line.size () < c * A.displayWidth + 1)  // +1 to allow for opening "[" all the way down
		{
		  line += ' ';
		}
	  }
	  stream << line;

	  if (++r >= rows) break;
	  if (columns > 1)
	  {
		stream << std::endl;
		line = " ";  // adjust for opening "["
	  }
	  else
	  {
		stream << " ";
		line.clear ();
	  }
	}
	stream << "]";

	return stream;
  }

  template<class T>
  std::istream &
  operator >> (std::istream & stream, MatrixAbstract<T> & A)
  {
	std::vector<std::vector<T> > temp;
	int columns = 0;
	bool transpose = false;

	// Scan for opening "["
	char token;
	do
	{
	  stream.get (token);
	  if (token == '~') transpose = true;
	}
	while (token != '['  &&  stream.good ());

	// Read rows until closing "]"
	std::string line;
	bool comment = false;
	bool done = false;
	while (stream.good ()  &&  ! done)
	{
	  stream.get (token);

	  bool processLine = false;
	  switch (token)
	  {
		case '\r':
		  break;  // ignore CR characters
		case '#':
		  comment = true;
		  break;
		case '\n':
		  comment = false;
		case ';':
		  if (! comment) processLine = true;
		  break;
		case ']':
		  if (! comment)
		  {
			done = true;
			processLine = true;
		  }
		  break;
		default:
		  if (! comment) line += token;
	  }

	  if (processLine)
	  {
		std::vector<T> row;
		std::string element;
		trim (line);
		while (line.size ())
		{
		  int position = line.find_first_of (" \t");
		  element = line.substr (0, position);
		  row.push_back (elementFromString<T> (element));
		  if (position == std::string::npos) break;
		  line = line.substr (position);
		  trim (line);
		}
		int c = row.size ();
		if (c)
		{
		  temp.push_back (row);
		  columns = std::max (columns, c);
		}
		line.clear ();
	  }
	}

	// Assign elements to A.
	const int rows = temp.size ();
	if (transpose)
	{
	  A.resize (columns, rows);
	  A.clear ();
	  for (int r = 0; r < rows; r++)
	  {
		std::vector<T> & row = temp[r];
		for (int c = 0; c < row.size (); c++)
		{
		  A(c,r) = row[c];
		}
	  }
	}
	else
	{
	  A.resize (rows, columns);
	  A.clear ();
	  for (int r = 0; r < rows; r++)
	  {
		std::vector<T> & row = temp[r];
		for (int c = 0; c < row.size (); c++)
		{
		  A(r,c) = row[c];
		}
	  }
	}

	return stream;
  }

  template<class T>
  MatrixAbstract<T> &
  operator << (MatrixAbstract<T> & A, const std::string & source)
  {
	std::istringstream stream (source);
	stream >> A;
	return A;
  }


  // class Matrix<T> ----------------------------------------------------------

  template<class T>
  Matrix<T>::Matrix ()
  {
	rows_ = 0;
	columns_ = 0;
	strideC = 0;
  };

  template<class T>
  Matrix<T>::Matrix (const int rows, const int columns)
  {
	resize (rows, columns);
  }

  template<class T>
  Matrix<T>::Matrix (const MatrixAbstract<T> & that)
  {
	if (that.classID () & MatrixID)
	{
	  *this = (const Matrix<T> &) that;
	}
	else
	{
	  // same code as copyFrom()
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
  Matrix<T>::Matrix (std::istream & stream)
  {
	read (stream);
  }

  template<class T>
  Matrix<T>::Matrix (const std::string & source)
  {
	rows_ = 0;
	columns_ = 0;
	strideC = 0;
	*this << source;
  }

  template<class T>
  Matrix<T>::Matrix (T * that, const int rows, const int columns)
  {
	data.attach (that, rows * columns * sizeof (T));
	rows_ = rows;
	columns_ = columns;
	strideC = rows;
  }

  template<class T>
  Matrix<T>::Matrix (Pointer & that, const int rows, const int columns)
  {
	data = that;
	if (rows < 0  ||  columns < 0)  // infer number from size of memory block and size of our data type
	{
	  int size = data.size ();
	  if (size < 0)
	  {
		// Pointer does not know the size of memory block, so we pretend it is empty.  This is really an error condition.
		rows_ = 0;
		columns_ = 0;
	  }
	  else
	  {
		if (rows < 0)
		{
		  rows_ = size / (sizeof (T) * columns);
		  columns_ = columns;
		}
		else
		{
		  rows_ = rows;
		  columns_ = size / (sizeof (T) * rows);
		}
	  }
	}
	else  // number of rows and columns is given
	{
	  rows_ = rows;
	  columns_ = columns;
	}
	strideC = rows_;
  }

  template<class T>
  Matrix<T>::~Matrix ()
  {
	// data automatically destructs its memory.  Cool, eh?
  }

  template<class T>
  void
  Matrix<T>::detach ()
  {
	rows_ = 0;
	columns_ = 0;
	strideC = 0;
	data.detach ();
  }

  template<class T>
  uint32_t
  Matrix<T>::classID () const
  {
	return MatrixAbstractID | MatrixID;
  }

  template<class T>
  int
  Matrix<T>::rows () const
  {
	return rows_;
  }

  template<class T>
  int
  Matrix<T>::columns () const
  {
	return columns_;
  }

  template<class T>
  MatrixAbstract<T> *
  Matrix<T>::duplicate (bool deep) const
  {
	if (deep)
	{
	  Matrix * result = new Matrix;
	  result->copyFrom (*this);
	  return result;
	}
	return new Matrix (*this);
  }

  template<class T>
  void
  Matrix<T>::clear (const T scalar)
  {
	if (scalar == (T) 0)
	{
	  data.clear ();
	}
	else
	{
	  T * i = (T *) data;
	  T * end = i + strideC * columns_;
	  while (i < end)
	  {
		*i++ = scalar;
	  }
	}	  
  }

  template<class T>
  void
  Matrix<T>::resize (const int rows, const int columns)
  {
	data.grow (rows * columns * sizeof (T));
	rows_ = rows;
	columns_ = columns;
	strideC = rows;
  }

  template<class T>
  void
  Matrix<T>::copyFrom (const MatrixAbstract<T> & that)
  {
	if (that.classID ()  &  MatrixID)
	{
	  const Matrix & M = (const Matrix &) (that);
	  resize (M.rows_, M.columns_);
	  const int step = M.strideC - M.rows_;
	  if (step == 0)
	  {
		data.copyFrom (M.data);
	  }
	  else
	  {
		T * i = (T *) data;
		T * j = (T *) M.data;
		T * end = i + rows_ * columns_;
		while (i < end)
		{
		  T * columnEnd = i + rows_;
		  while (i < columnEnd) *i++ = *j++;
		  j += step;
		}
	  }
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

  /**
	 Unwind the elements columnwise and reflow them into a matrix of the
	 given size.  If the new matrix has a larger number of elements, then
	 repeat the element sequence until the new matrix is filled.  If the
	 number of rows is changed (the most likely case) and the source matrix
	 has a number of rows different than its stride, then this function
	 will move data in memory to provide contiguous access to elements.

	 @param avoidCopy If the new size is no larger in each dimension than
	 the old size, you can avoid copying the elements by setting this flag.
	 The consequence is that the values in the new matrix will not follow
	 the columnwise unwind behavior, but will simply be a block out of the
	 source matrix of the specified size.
  **/
  template<class T>
  Matrix<T>
  Matrix<T>::reshape (const int rows, const int columns, bool inPlace) const
  {
	if (inPlace)
	{
	  if (rows <= strideC  &&  columns <= columns_)
	  {
		Matrix result = *this;  // should be shallow copy; strideC will be the same
		result.rows_    = rows;
		result.columns_ = columns;
		return result;
	  }
	}
	else
	{
	  if (rows_ == strideC  &&  rows * columns <= rows_ * columns_)
	  {
		Matrix result = *this;
		result.rows_    = rows;
		result.columns_ = columns;
		result.strideC  = rows;
		return result;
	  }
	}

	// Create new matrix and copy data into a dense block.
	Matrix result (rows, columns);
	const int currentSize = rows_ * columns_;
	const int resultSize  = rows  * columns;
	const int step        = strideC - rows_;
	T * resultData = (T *) result.data;
	T * source     = (T *) data;
	T * dest       = resultData;
	//   Copy an integral number of columns over
	T * end = dest + (std::min (currentSize, resultSize) / rows_) * rows_;
	while (dest < end)
	{
	  T * columnEnd = source + rows_;
	  while (source < columnEnd) *dest++ = *source++;
	  source += step;
	}

	// Finish filling result:
	//   currentSize < resultSize -- duplicate data block in result
	//   currentSize > resultSize -- copy over part of a final column
	end = resultData + resultSize;
	if (currentSize < resultSize) source = resultData;
	while (dest < end) *dest++ = *source++;

	return result;
  }

  // This version of norm() is suitable for float and double.  Other
  // numeric types may need more specialization.
  template<class T>
  T
  Matrix<T>::norm (float n) const
  {
	// Some of this code may have to be modified for complex numbers.
	T * i = (T *) data;
	T * end = i + strideC * columns_;
	const int step = strideC - rows_;
	if (n == INFINITY)
	{
	  T result = (T) 0;
	  while (i < end)
	  {
		T * columnEnd = i + rows_;
		while (i < columnEnd) result = std::max (std::abs (*i++), result);
		i += step;
	  }
	  return result;
	}
	else if (n == 0.0f)
	{
	  unsigned int result = 0;
	  while (i < end)
	  {
		T * columnEnd = i + rows_;
		while (i < columnEnd) if (std::abs (*i++) > 0) result++;
		i += step;
	  }
	  return (T) result;
	}
	else if (n == 1.0f)
	{
	  T result = (T) 0;
	  while (i < end)
	  {
		T * columnEnd = i + rows_;
		while (i < columnEnd) result += *i++;
		i += step;
	  }
	  return result;
	}
	else if (n == 2.0f)
	{
	  T result = (T) 0;
	  while (i < end)
	  {
		T * columnEnd = i + rows_;
		while (i < columnEnd) result += (*i) * (*i++);
		i += step;
	  }
	  return (T) std::sqrt (result);
	}
	else
	{
	  T result = (T) 0;
	  while (i < end)
	  {
		T * columnEnd = i + rows_;
		while (i < columnEnd) result += (T) std::pow (*i++, (T) n);
		i += step;
	  }
	  return (T) std::pow (result, (T) (1.0 / n));
	}
  }

  template<class T>
  T
  Matrix<T>::sumSquares () const
  {
	T * i = (T *) data;
	T * end = i + rows_ * columns_;
	const int step = strideC - rows_;
	T result = 0;
	while (i < end)
	{
	  T * columnEnd = i + rows_;
	  while (i < columnEnd) result += (*i) * (*i++);
	  i += step;
	}
	return result;
  }

  template<class T>
  T
  Matrix<T>::dot (const Matrix<T> & B) const
  {
	register T result = (T) 0;
	T * i   = (T *) data;
	T * j   = (T *) B.data;
	T * end = i + std::min (rows_, B.rows_);
	while (i < end) result += (*i++) * (*j++);
	return result;
  }

  template<class T>
  Matrix<T>
  Matrix<T>::transposeSquare () const
  {
	Matrix result (columns_, columns_);
	for (int i = 0; i < columns_; i++)
	{
	  for (int j = i; j < columns_; j++)
	  {
		T * ki = & (*this)(0,i);
		T * kj = & (*this)(0,j);
		T * end = ki + rows_;
		register T sum = (T) 0;
		while (ki < end)
		{
		  sum += (*ki++) * (*kj++);
		}
		result(i,j) = sum;
	  }
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  Matrix<T>::operator * (const MatrixAbstract<T> & B) const
  {
	const int w  = std::min (columns_, B.rows ());
	const int bw = B.columns ();
	Matrix * result = new Matrix (rows_, bw);
	T * ri = (T *) result->data;

	if (B.classID () & MatrixID)
	{
	  const Matrix & MB = (const Matrix &) B;
	  for (int c = 0; c < bw; c++)
	  {
		for (int r = 0; r < rows_; r++)
		{
		  T * i = ((T *) data) + r;
		  T * j = ((T *) MB.data) + c * MB.strideC;
		  T * end = j + w;
		  register T element = (T) 0;
		  while (j < end)
		  {
			element += (*i) * (*j++);
			i += strideC;
		  }
		  *ri++ = element;
		}
	  }
	}
	else
	{
	  for (int c = 0; c < bw; c++)
	  {
		for (int r = 0; r < rows_; r++)
		{
		  T * i = ((T *) data) + r;
		  register T element = (T) 0;
		  for (int j = 0; j < w; j++)
		  {
			element += (*i) * B (j, c);
			i += strideC;
		  }
		  *ri++ = element;
		}
	  }
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  Matrix<T>::operator * (const T scalar) const
  {
	Matrix * result = new Matrix (rows_, columns_);
	T * i   = (T *) data;
	T * r   = (T *) result->data;
	T * end = r + rows_ * columns_;
	const int step = strideC - rows_;
	while (r < end)
	{
	  T * columnEnd = i + rows_;
	  while (i < columnEnd) *r++ = *i++ * scalar;
	  i += step;
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  Matrix<T>::operator / (const T scalar) const
  {
	Matrix * result = new Matrix (rows_, columns_);
	T * i   = (T *) data;
	T * r   = (T *) result->data;
	T * end = r + rows_ * columns_;
	const int step = strideC - rows_;
	while (r < end)
	{
	  T * columnEnd = i + rows_;
	  while (i < columnEnd) *r++ = *i++ / scalar;
	  i += step;
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  Matrix<T>::operator + (const MatrixAbstract<T> & B) const
  {
	if ((B.classID () & MatrixID) == 0) return MatrixAbstract<T>::operator + (B);

	Matrix * result = new Matrix (rows_, columns_);
	const Matrix & MB = (const Matrix &) B;
	const int oh = std::min (rows_,    MB.rows_);
	const int ow = std::min (columns_, MB.columns_);
	const int stepA =    strideC - rows_;
	const int stepB = MB.strideC - oh;
	T * a = (T *) data;
	T * b = (T *) MB.data;
	T * r = (T *) result->data;
	T * end = r + rows_ * ow;
	while (r < end)
	{
	  T * overlapEnd = r + oh;
	  T * columnEnd = r + rows_;
	  while (r < overlapEnd) *r++ = *a++ + *b++;
	  while (r < columnEnd)  *r++ = *a++;
	  a += stepA;
	  b += stepB;
	}
	end += rows_ * (columns_ - ow);
	while (r < end)
	{
	  T * columnEnd = r + rows_;
	  while (r < columnEnd) *r++ = *a++;
	  a += stepA;
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  Matrix<T>::operator - (const MatrixAbstract<T> & B) const
  {
	if ((B.classID () & MatrixID) == 0) return MatrixAbstract<T>::operator - (B);

	Matrix * result = new Matrix (rows_, columns_);
	const Matrix & MB = (const Matrix &) B;
	const int oh = std::min (rows_,    MB.rows_);
	const int ow = std::min (columns_, MB.columns_);
	const int stepA =    strideC - rows_;
	const int stepB = MB.strideC - oh;
	T * a = (T *) data;
	T * b = (T *) MB.data;
	T * r = (T *) result->data;
	T * end = r + rows_ * ow;
	while (r < end)
	{
	  T * overlapEnd = r + oh;
	  T * columnEnd = r + rows_;
	  while (r < overlapEnd) *r++ = *a++ - *b++;
	  while (r < columnEnd)  *r++ = *a++;
	  a += stepA;
	  b += stepB;
	}
	end += rows_ * (columns_ - ow);
	while (r < end)
	{
	  T * columnEnd = r + rows_;
	  while (r < columnEnd) *r++ = *a++;
	  a += stepA;
	}

	return result;
  }

  template<class T>
  MatrixAbstract<T> &
  Matrix<T>::operator *= (const MatrixAbstract<T> & B)
  {
	if (B.classID () & MatrixID)
	{
	  return *this = (*this) * (const Matrix &) B;
	}
	return MatrixAbstract<T>::operator *= (B);
  }

  template<class T>
  MatrixAbstract<T> &
  Matrix<T>::operator *= (const T scalar)
  {
	T * i = (T *) data;
	T * end = i + strideC * columns_;
	const int step = strideC - rows_;
	while (i < end)
	{
	  T * columnEnd = i + rows_;
	  while (i < columnEnd) *i++ *= scalar;
	  i += step;
	}
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  Matrix<T>::operator /= (const T scalar)
  {
	T * i = (T *) data;
	T * end = i + strideC * columns_;
	const int step = strideC - rows_;
	while (i < end)
	{
	  T * columnEnd = i + rows_;
	  while (i < columnEnd) *i++ /= scalar;
	  i += step;
	}
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  Matrix<T>::operator += (const MatrixAbstract<T> & B)
  {
	if ((B.classID () & MatrixID) == 0) return MatrixAbstract<T>::operator += (B);

	const Matrix & MB = (const Matrix &) B;
	const int oh = std::min (rows_,    MB.rows_);
	const int ow = std::min (columns_, MB.columns_);
	const int stepA =    strideC - oh;
	const int stepB = MB.strideC - oh;
	T * a   = (T *) data;
	T * b   = (T *) MB.data;
	T * end = a + strideC * ow;
	while (a < end)
	{
	  T * columnEnd = a + oh;
	  while (a < columnEnd) *a++ += *b++;
	  a += stepA;
	  b += stepB;
	}

	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  Matrix<T>::operator += (const T scalar)
  {
	T * i   = (T *) data;
	T * end = i + strideC * columns_;
	const int step = strideC - rows_;
	while (i < end)
	{
	  T * columnEnd = i + rows_;
	  while (i < columnEnd) *i++ += scalar;
	  i += step;
	}
	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  Matrix<T>::operator -= (const MatrixAbstract<T> & B)
  {
	if ((B.classID () & MatrixID) == 0) return MatrixAbstract<T>::operator -= (B);

	const Matrix & MB = (const Matrix &) B;
	const int oh = std::min (rows_,    MB.rows_);
	const int ow = std::min (columns_, MB.columns_);
	const int stepA =    strideC - oh;
	const int stepB = MB.strideC - oh;
	T * a   = (T *) data;
	T * b   = (T *) MB.data;
	T * end = a + strideC * ow;
	while (a < end)
	{
	  T * columnEnd = a + oh;
	  while (a < columnEnd) *a++ -= *b++;
	  a += stepA;
	  b += stepB;
	}

	return *this;
  }

  template<class T>
  MatrixAbstract<T> &
  Matrix<T>::operator -= (const T scalar)
  {
	T * i   = (T *) data;
	T * end = i + strideC * columns_;
	const int step = strideC - rows_;
	while (i < end)
	{
	  T * columnEnd = i + rows_;
	  while (i < columnEnd) *i++ -= scalar;
	  i += step;
	}
	return *this;
  }

  template<class T>
  void
  Matrix<T>::read (std::istream & stream)
  {
	stream.read ((char *) &rows_, sizeof (rows_));
	stream.read ((char *) &columns_, sizeof (columns_));
	strideC = rows_;
	if (! stream.good ())
	{
	  throw "Stream bad.  Unable to finish reading Matrix.";
	}
	int bytes = rows_ * columns_ * sizeof (T);
	data.grow (bytes);
	stream.read ((char *) data, bytes);
  }

  template<class T>
  void
  Matrix<T>::write (std::ostream & stream) const
  {
	stream.write ((char *) &rows_, sizeof (rows_));
	stream.write ((char *) &columns_, sizeof (columns_));
	T * i = (T *) data;
	const int bytes = rows_ * sizeof (T);
	int count = bytes * columns_;
	while (count > 0)
	{
	  stream.write ((char *) i, bytes);
	  i += strideC;
	  count -= bytes;
	}
  }


  // class MatrixTranspose<T> -------------------------------------------------

  template<class T>
  MatrixTranspose<T>::MatrixTranspose (MatrixAbstract<T> * that)
  {
	wrapped = that;
  }

  template<class T>
  MatrixTranspose<T>::~MatrixTranspose ()
  {
	delete wrapped;  // We can assume that wrapped != NULL.
  }

  template<class T>
  int
  MatrixTranspose<T>::rows () const
  {
	return wrapped->columns ();
  }

  template<class T>
  int
  MatrixTranspose<T>::columns () const
  {
	return wrapped->rows ();
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixTranspose<T>::duplicate (bool deep) const
  {
	return new MatrixTranspose<T> (wrapped->duplicate (deep));
  }

  template<class T>
  void
  MatrixTranspose<T>::clear (const T scalar)
  {
	wrapped->clear (scalar);
  }

  template<class T>
  void
  MatrixTranspose<T>::resize (const int rows, const int columns)
  {
	wrapped->resize (columns, rows);
  }

  template<class T>
  MatrixResult<T>
  MatrixTranspose<T>::operator * (const MatrixAbstract<T> & B) const
  {
	int w = std::min (wrapped->rows (), B.rows ());
	int h = wrapped->columns ();
	int bw = B.columns ();
	Matrix<T> * result = new Matrix<T> (h, bw);
	for (int c = 0; c < bw; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		register T element = (T) 0;
		for (int i = 0; i < w; i++)
		{
		  element += (*wrapped)(i,r) * B(i,c);
		}
		(*result)(r,c) = element;
	  }
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixTranspose<T>::operator * (const T scalar) const
  {
    int h = wrapped->columns ();
    int w = wrapped->rows ();
	Matrix<T> * result = new Matrix<T> (h, w);
    for (int c = 0; c < w; c++)
    {
      for (int r = 0; r < h; r++)
      {
		(*result)(r,c) = (*wrapped)(c,r) * scalar;
      }
    }
    return result;
  }


  // class MatrixRegion -------------------------------------------------------

  template<class T>
  MatrixRegion<T>::MatrixRegion (const MatrixAbstract<T> & that, const int firstRow, const int firstColumn, int lastRow, int lastColumn)
  {
	wrapped = &that;
	this->firstRow = firstRow;
	this->firstColumn = firstColumn;
	if (lastRow < 0)
	{
	  lastRow = that.rows () - 1;
	}
	if (lastColumn < 0)
	{
	  lastColumn = that.columns () - 1;
	}
	rows_ = lastRow - firstRow + 1;
	columns_ = lastColumn - firstColumn + 1;
  }

  template<class T>
  MatrixRegion<T> &
  MatrixRegion<T>::operator = (const MatrixRegion<T> & that)
  {
	int h = that.rows ();
	int w = that.columns ();
	resize (h, w);
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		(*this)(r,c) = that(r,c);
	  }
	}
	return *this;
  }

  template<class T>
  int
  MatrixRegion<T>::rows () const
  {
	return rows_;
  }

  template<class T>
  int
  MatrixRegion<T>::columns () const
  {
	return columns_;
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixRegion<T>::duplicate (bool deep) const
  {
	if (deep)
	{
	  // Deep copy implies we have permission to disengage from the orginal
	  // matrix, so just realize a dense Matrix.
	  Matrix<T> * result = new Matrix<T> (rows_, columns_);
	  T * i = (T *) result->data;
	  for (int c = firstColumn; c < firstColumn + columns_; c++)
	  {
		for (int r = firstRow; r < firstRow + rows_; r++)
		{
		  *i++ = (*wrapped)(r,c);
		}
	  }
	  return result;
	}
	return new MatrixRegion<T> (*wrapped, firstRow, firstColumn, firstRow + rows_ - 1, firstColumn + columns_ - 1);
  }

  template<class T>
  void
  MatrixRegion<T>::clear (const T scalar)
  {
	for (int r = firstRow + rows_ - 1; r >= firstRow; r--)
	{
	  for (int c = firstColumn + columns_ - 1; c >= firstColumn; c--)
	  {
		(*wrapped)(r,c) = scalar;
	  }
	}
  }

  template<class T>
  void
  MatrixRegion<T>::resize (const int rows, const int columns)
  {
	// We can't resize a region of the wrapped object, but we can change
	// the number of rows or columns in the view.
	rows_ = rows;
	columns_ = columns;
  }

  template<class T>
  MatrixResult<T>
  MatrixRegion<T>::operator * (const MatrixAbstract<T> & B) const
  {
	int w = std::min (columns (), B.rows ());
	int h = rows ();
	int bw = B.columns ();
	Matrix<T> * result = new Matrix<T> (h, bw);
	for (int c = 0; c < bw; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		register T element = (T) 0;
		for (int i = 0; i < w; i++)
		{
		  element += (*this)(r,i) * B(i,c);
		}
		(*result)(r,c) = element;
	  }
	}
	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixRegion<T>::operator * (const T scalar) const
  {
    int h = rows ();
    int w = columns ();
	Matrix<T> * result = new Matrix<T> (h, w);
    for (int c = 0; c < w; c++)
    {
      for (int r = 0; r < h; r++)
      {
		(*result)(r,c) = (*this)(r,c) * scalar;
      }
    }
    return result;
  }
}


#endif
