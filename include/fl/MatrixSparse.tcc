#ifndef fl_matrix_sparse_tcc
#define fl_matrix_sparse_tcc


#include "fl/matrix.h"


namespace fl
{
  template<class T>
  MatrixSparse<T>::MatrixSparse ()
  {
	rows_ = 0;
	data = new SparseBlock;
	data->refcount = 1;
  }

  template<class T>
  MatrixSparse<T>::MatrixSparse (const int rows, const int columns)
  {
	data = new SparseBlock;
	data->refcount = 1;
	resize (rows, columns);
  }

  template<class T>
  MatrixSparse<T>::MatrixSparse (const MatrixAbstract<T> & that)
  {
	data = new SparseBlock;
	data->refcount = 1;
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
  MatrixSparse<T>::MatrixSparse (const MatrixSparse<T> & that)
  {
	rows_ = that.rows_;
	data = that.data;
	data->refcount++;
  }

  template<class T>
  MatrixSparse<T>::MatrixSparse (std::istream & stream)
  {
  }

  template<class T>
  MatrixSparse<T>::~MatrixSparse ()
  {
	data->refcount--;
	if (data->refcount <= 0)
	{
	  delete data;
	}
  }

  template<class T>
  MatrixSparse<T> &
  MatrixSparse<T>::operator = (const MatrixSparse & that)
  {
	rows_ = that.rows_;
	data = that.data;
	data->refcount++;
  }

  template<class T>
  void
  MatrixSparse<T>::set (const int row, const int column, const T value)
  {
	if (value == (T) 0)
	{
	  if (column < data->columns.size ())
	  {
		data->columns[column].erase (row);
	  }
	}
	else
	{
	  if (row >= rows_)
	  {
		rows_ = row + 1;
	  }
	  if (column >= data->columns.size ())
	  {
		data->columns.resize (column + 1);
	  }
	  data->columns[column][row] = value;
	}
  }

  template<class T>
  T &
  MatrixSparse<T>::operator () (const int row, const int column) const
  {
	if (column < data->columns.size ())
	{
	  std::map<int, T> & c = data->columns[column];
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
	return data->columns.size ();
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixSparse<T>::duplicate () const
  {
	return new MatrixSparse (*this);
  }

  template<class T>
  void
  MatrixSparse<T>::clear ()
  {
	data->columns.clear ();
  }

  template<class T>
  void
  MatrixSparse<T>::resize (const int rows, const int columns)
  {
	rows_ = rows;
	data->columns.resize (columns);
  }

  template<class T>
  void
  MatrixSparse<T>::copyFrom (const MatrixSparse & that)
  {
	// Prepare fresh data block
	if (data->refcount != 1)
	{
	  data->refcount--;
	  if (data->refcount <= 0)
	  {
		delete data;
	  }
	  data = new SparseBlock;
	  data->refcount = 1;
	}

	rows_ = that.rows_;
	data->columns = that.data->columns;  // performs deep copy of STL vector and map objects
  }

  template<class T>
  void
  MatrixSparse<T>::read (std::istream & stream)
  {
	int n;
	stream.read ((char *) &n, sizeof (n));
	data->columns.resize (n);

	for (int i = 0; i < n; i++)
	{
	  std::map<int,T> & C = data->columns[i];

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
	  }
	}
  }

  template<class T>
  void
  MatrixSparse<T>::write (std::ostream & stream, bool withName) const
  {
	MatrixAbstract<T>::write (stream, withName);

	const int n = data->columns.size ();
	stream.write ((char *) &n, sizeof (n));
	for (int i = 0; i < n; i++)
	{
	  std::map<int,T> & C = data->columns[i];

	  const int m = C.size ();
	  stream.write ((char *) &m, sizeof (m));

	  typename std::map<int,T>::iterator j = C.begin ();
	  while (j != C.end ())
	  {
		stream.write ((char *) & j->first, sizeof (j->first));
		stream.write ((char *) & j->second, sizeof (j->second));
	  }
	}
  }

}

#endif
