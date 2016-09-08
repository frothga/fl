/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix_block_tcc
#define fl_matrix_block_tcc


#include "fl/matrix.h"


namespace fl
{
  template<class T>
  MatrixBlock<T>::MatrixBlock ()
  {
	startRows.push_back (0);
	startColumns.push_back (0);
	blockStride = 0;
  }

  template<class T>
  MatrixBlock<T>::MatrixBlock (const int blockRows, const int blockColumns)
  {
	startRows.push_back (0);
	startColumns.push_back (0);
	blockStride = 0;
	blockResize (blockRows, blockColumns);
  }

  template<class T>
  MatrixBlock<T>::MatrixBlock (const MatrixAbstract<T> & that)
  {
	startRows.push_back (0);
	startColumns.push_back (0);
	blockStride = 0;

	if (that.classID () & MatrixBlockID)
	{
	  this->operator = ((const MatrixBlock<T> &) that);
	}
	else
	{
	  blockSet (0, 0, that);
	}
  }

  template<class T>
  MatrixBlock<T>::~MatrixBlock ()
  {
	detach ();
  }

  template<class T>
  void
  MatrixBlock<T>::detach ()
  {
	startRows   .clear ();
	startColumns.clear ();
	startRows   .push_back (0);
	startColumns.push_back (0);

	blockStride = 0;

	if (data.size () <= 0) return;
	MatrixAbstract<T> ** p   = (MatrixAbstract<T> **) data;
	MatrixAbstract<T> ** end = p + data.size () / sizeof (MatrixAbstract<T> *);
	while (p < end)
	{
	  if (*p) delete *p;
	  p++;
	}
	data.detach ();
  }

  template<class T>
  uint32_t
  MatrixBlock<T>::classID () const
  {
	return MatrixAbstractID | MatrixBlockID;
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixBlock<T>::clone (bool deep) const
  {
	MatrixBlock * result = new MatrixBlock;
	result->copyFrom (*this, deep);
	return result;
  }

  template<class T>
  void
  MatrixBlock<T>::copyFrom (const MatrixAbstract<T> & that, bool deep)
  {
	uint32_t thatID = that.classID ();
	if (thatID & MatrixBlockID)
	{
	  const MatrixBlock & MB = (const MatrixBlock &) (that);
	  if (this != &MB) detach ();
	  if (MB.data.size () <= 0) return;
	  startRows    = MB.startRows;
	  startColumns = MB.startColumns;
	  blockStride  = MB.blockStride;
	  Pointer oldData = MB.data;
	  data.copyFrom (MB.data);
	  MatrixAbstract<T> ** p   = (MatrixAbstract<T> **) data;
	  MatrixAbstract<T> ** end = p + data.size () / sizeof (MatrixAbstract<T> *);
	  while (p < end)
	  {
		if (*p) *p = (*p)->clone (deep);
		p++;
	  }
	  if (this == &MB  &&  oldData.size () > 0)
	  {
		MatrixAbstract<T> ** p   = (MatrixAbstract<T> **) oldData;
		MatrixAbstract<T> ** end = p + oldData.size () / sizeof (MatrixAbstract<T> *);
		while (p < end)
		{
		  if (*p) delete *p;
		  p++;
		}
	  }
	}
	else
	{
	  if (thatID & MatrixResultID)
	  {
		MatrixAbstract<T> * result = ((const MatrixResult<T> &) that).result;
		if (result->classID () & MatrixBlockID)
		{
		  copyFrom (*result, deep);
		  return;
		}
	  }
	  blockResize (1, 1);  // shrink if necessary
	  blockSet (0, 0, that.clone (deep));
	}
  }

  template<class T>
  MatrixAbstract<T> &
  MatrixBlock<T>::operator = (const MatrixBlock<T> & that)
  {
	assert (this != &that);

	// delete any data we currently own
	detach ();

	// steal data from that
	startRows    = that.startRows;
	startColumns = that.startColumns;
	blockStride  = that.blockStride;
	data         = that.data;

	MatrixBlock & MB = const_cast<MatrixBlock &> (that);
	MB.startRows.clear ();
	MB.startRows.push_back (0);
	MB.startColumns.clear ();
	MB.startColumns.push_back (0);
	MB.blockStride = 0;
	MB.data.detach ();

	return *this;
  }

  template<class T>
  void
  MatrixBlock<T>::blockSet (const int blockRow, const int blockColumn, MatrixAbstract<T> * A)
  {
	int blockRows    = startRows   .size () - 1;
	int blockColumns = startColumns.size () - 1;
	if (blockRow >= blockRows  ||  blockColumn >= blockColumns)
	{
	  int newRows    = std::max (blockRow    + 1, blockRows);
	  int newColumns = std::max (blockColumn + 1, blockColumns);
	  blockResize (newRows, newColumns);
	}
	MatrixAbstract<T> ** p = ((MatrixAbstract<T> **) data) + blockColumn * blockStride + blockRow;
	if (*p) delete *p;
	*p = A;
	blockUpdate (blockRow, blockColumn);
  }

  template<class T>
  void
  MatrixBlock<T>::blockSet (const int blockRow, const int blockColumn, const MatrixAbstract<T> & A)
  {
	blockSet (blockRow, blockColumn, A.clone ());
  }

  template<class T>
  MatrixAbstract<T> *
  MatrixBlock<T>::blockGet (const int blockRow, const int blockColumn) const
  {
	return ((MatrixAbstract<T> **) data)[blockColumn * blockStride + blockRow];
  }

  template<class T>
  void
  MatrixBlock<T>::blockUpdate (const int blockRow, const int blockColumn)
  {
	int blockRows    = startRows   .size () - 1;
	int blockColumns = startColumns.size () - 1;
	assert (blockRow < blockRows  &&  blockColumn < blockColumns);

	MatrixAbstract<T> ** blocks = (MatrixAbstract<T> **) data;
	MatrixAbstract<T> *  a      = blocks[blockColumn * blockStride + blockRow];
	if (a == 0) return;

	int oldRows    = startRows   [blockRow   +1] - startRows   [blockRow];
	int oldColumns = startColumns[blockColumn+1] - startColumns[blockColumn];

	int maximum = a->rows ();
	if (maximum < oldRows)
	{
	  MatrixAbstract<T> ** p   = & blocks[blockRow];
	  MatrixAbstract<T> ** end = p + blockColumns * blockStride;
	  while (p < end)
	  {
		if (*p) maximum = std::max (maximum, (*p)->rows ());
		p += blockStride;
	  }
	}
	int delta = maximum - oldRows;
	if (delta) for (int r = blockRow + 1; r <= blockRows; r++) startRows[r] += delta;

	maximum = a->columns ();
	if (maximum < oldColumns)
	{
	  MatrixAbstract<T> ** p   = blocks + blockColumn * blockStride;
	  MatrixAbstract<T> ** end = p + blockStride;
	  while (p < end)
	  {
		if (*p) maximum = std::max (maximum, (*p)->columns ());
		p++;
	  }
	}
	delta = maximum - oldColumns;
	if (delta) for (int c = blockColumn + 1; c <= blockColumns; c++) startColumns[c] += delta;
  }

  template<class T>
  void
  MatrixBlock<T>::blockUpdateAll ()
  {
	int blockRows    = startRows   .size () - 1;
	int blockColumns = startColumns.size () - 1;

	for (int r = 0; r <= blockRows;    r++) startRows   [r] = 0;
	for (int c = 0; c <= blockColumns; c++) startColumns[c] = 0;

	MatrixAbstract<T> ** p = (MatrixAbstract<T> **) data;
	int step = blockStride - blockRows;
	for (int blockColumn = 0; blockColumn < blockColumns; blockColumn++)
	{
	  int & maxColumns = startColumns[blockColumn];
	  for (int blockRow = 0; blockRow < blockRows; blockRow++)
	  {
		int & maxRows = startRows[blockRow];
		if (*p)
		{
		  maxRows    = std::max ((*p)->rows    (), maxRows);
		  maxColumns = std::max ((*p)->columns (), maxColumns);
		}
		p++;
	  }
	  p += step;
	}

	// Convert counts into start indices
	int total = 0;
	for (int r = 0; r <= blockRows; r++)
	{
	  int & s = startRows[r];
	  int t = s;
	  s = total;
	  total += t;
	}
	total = 0;
	for (int c = 0; c <= blockColumns; c++)
	{
	  int & s = startColumns[c];
	  int t = s;
	  s = total;
	  total += t;
	}
  }

  template<class T>
  int
  MatrixBlock<T>::blockRows () const
  {
	return startRows.size () - 1;
  }

  template<class T>
  int
  MatrixBlock<T>::blockColumns () const
  {
	return startColumns.size () - 1;
  }

  template<class T>
  void
  MatrixBlock<T>::blockResize (const int blockRows, const int blockColumns)
  {
	int oldRows    = startRows   .size () - 1;
	int oldColumns = startColumns.size () - 1;
	startRows   .resize (blockRows    + 1);
	startColumns.resize (blockColumns + 1);

	// Expand if needed: replicate row and column counts
	for (int r = oldRows    + 1; r <= blockRows;    r++) startRows   [r] = startRows   [oldRows   ];
	for (int c = oldColumns + 1; c <= blockColumns; c++) startColumns[c] = startColumns[oldColumns];

	// Shrink if needed: delete any matrics that are now out of bounds
	MatrixAbstract<T> ** d = (MatrixAbstract<T> **) data;
	for (int r = blockRows; r < oldRows; r++)
	{
	  for (int c = 0; c < oldColumns; c++)
	  {
		MatrixAbstract<T> ** p = d + c * blockStride + r;
		if (*p)
		{
		  delete *p;
		  *p = 0;
		}
	  }
	}

	for (int c = blockColumns; c < oldColumns; c++)
	{
	  for (int r = 0; r < blockRows; r++)
	  {
		MatrixAbstract<T> ** p = d + c * blockStride + r;
		if (*p)
		{
		  delete *p;
		  *p = 0;
		}
	  }
	}

	int width = blockStride <= 0 ? 0 : (data.size () / (blockStride * sizeof (MatrixAbstract<T> *)));
	if (blockRows > blockStride  ||  blockColumns > width)
	{
	  Pointer newData (blockRows * blockColumns * sizeof (MatrixAbstract<T> *));
	  newData.clear ();
	  MatrixAbstract<T> ** from = (MatrixAbstract<T> **) data;
	  MatrixAbstract<T> ** to   = (MatrixAbstract<T> **) newData;
	  int copyRows    = std::min (blockRows,    oldRows);
	  int copyColumns = std::min (blockColumns, oldColumns);
	  for (int c = 0; c < copyColumns; c++)
	  {
		for (int r = 0; r < copyRows; r++)
		{
		  to[c * blockRows + r] = from[c * blockStride + r];
		}
	  }
	  blockStride = blockRows;
	  data = newData;
	}
  }

  template<class T>
  void
  MatrixBlock<T>::blockDump () const
  {
	std::cerr << "--------------------------------------------- begin " << this << std::endl;
	std::cerr << "rows: ";
	for (int i = 0; i < startRows.size (); i++) std::cerr << startRows[i] << " ";
	std::cerr << std::endl;
	std::cerr << "cols: ";
	for (int i = 0; i < startColumns.size (); i++) std::cerr << startColumns[i] << " ";
	std::cerr << std::endl;

	MatrixAbstract<T> ** p   = (MatrixAbstract<T> **) data;
	MatrixAbstract<T> ** end = p + (data.size () <= 0 ? 0 : data.size () / sizeof (MatrixAbstract<T> *));
	std::cerr << "data elements " << end - p << std::endl;

	while (p < end)
	{
	  std::cerr << "  " << p << " " << *p;
	  if (*p) std::cerr << std::endl << **p;
	  std::cerr << std::endl;
	  p++;
	}
	std::cerr << "--------------------------------------------- end " << this << std::endl;
  }

  static inline int
  binarySearch (const std::vector<int> & data, const int & target)
  {
	int lo = 0;
	int hi = data.size () - 1;
	if (data[hi] <= target) return -1;  // indicates target is out of bounds
	while (true)
	{
	  int mid = (lo + hi) / 2;
	  if (target < data[mid])
	  {
		hi = mid;
	  }
	  else // data[mid] <= target
	  {
		if (lo == mid) return mid;
		lo = mid;
	  }
	}
  }

  template<class T>
  T &
  MatrixBlock<T>::operator () (const int row, const int column) const
  {
	int blockRow = binarySearch (startRows, row);
	if (blockRow >= 0)
	{
	  int blockColumn = binarySearch (startColumns, column);
	  if (blockColumn >= 0)
	  {
		MatrixAbstract<T> * p = ((MatrixAbstract<T> **) data)[blockColumn * blockStride + blockRow];
		if (p)
		{
		  int r = row - startRows[blockRow];
		  if (r < p->rows ())
		  {
			int c = column - startColumns[blockColumn];
			if (c < p->columns ()) return (*p)(r,c);
		  }
		}
	  }
	}

	static T zero;
	zero = (T) 0;
	return zero;
  }

  template<class T>
  int
  MatrixBlock<T>::rows () const
  {
	return startRows.back ();
  }

  template<class T>
  int
  MatrixBlock<T>::columns () const
  {
	return startColumns.back ();
  }

  template<class T>
  void
  MatrixBlock<T>::resize (const int rows, const int columns)
  {
	detach ();
	blockResize (1, 1);
	startRows   [1] = rows;
	startColumns[1] = columns;
  }

  template<class T>
  void
  MatrixBlock<T>::clear (const T scalar)
  {
	if (data.size () <= 0) return;
	MatrixAbstract<T> ** p   = (MatrixAbstract<T> **) data;
	MatrixAbstract<T> ** end = p + data.size () / sizeof (MatrixAbstract<T> *);
	while (p < end)
	{
	  if (*p) (*p)->clear (scalar);
	  p++;
	}
  }

  template<class T>
  double
  MatrixBlock<T>::norm (double n) const
  {
	if (data.size () <= 0) return 0;
	MatrixAbstract<T> ** p   = (MatrixAbstract<T> **) data;
	MatrixAbstract<T> ** end = p + data.size () / sizeof (MatrixAbstract<T> *);

	if (n == INFINITY)
	{
	  double result = 0;
	  while (p < end)
	  {
		if (*p) result = std::max ((*p)->norm (n), result);
		p++;
	  }
	  return result;
	}
	else if (n == 0)
	{
	  unsigned int result = 0;
	  while (p < end)
	  {
		if (*p) result += (*p)->norm (n);
		p++;
	  }
	  return result;
	}
	else if (n == 1)
	{
	  double result = 0;
	  while (p < end)
	  {
		if (*p) result += (*p)->norm (n);
		p++;
	  }
	  return result;
	}
	else if (n == 2)
	{
	  double result = 0;
	  while (p < end)
	  {
		if (*p)
		{
		  T temp = (*p)->norm (n);
		  result += temp * temp;
		}
		p++;
	  }
	  return std::sqrt (result);
	}
	else
	{
	  double result = 0;
	  while (p < end)
	  {
		if (*p) result += std::pow ((*p)->norm (n), n);
		p++;
	  }
	  return std::pow (result, 1 / n);
	}
  }

  template<class T>
  MatrixResult<T>
  MatrixBlock<T>::transposeSquare () const
  {
	int AR = startRows   .size () - 1;
	int AC = startColumns.size () - 1;
	MatrixBlock * result = new MatrixBlock (AC, AC);
	if (AC < 1  ||  AR < 1) return result;

	MatrixAbstract<T> ** Ablocks = (MatrixAbstract<T> **)         data;
	MatrixAbstract<T> ** Cblocks = (MatrixAbstract<T> **) result->data;

	for (int blockColumn = 0; blockColumn < AC; blockColumn++)
	{
	  // Elements above the diagonal
	  for (int blockRow = 0; blockRow < blockColumn; blockRow++)
	  {
		MatrixAbstract<T> ** a   = Ablocks + blockRow    * blockStride;
		MatrixAbstract<T> ** b   = Ablocks + blockColumn * blockStride;
		MatrixAbstract<T> ** c   = Cblocks + blockColumn * AC + blockRow;
		MatrixAbstract<T> ** end = b + AR;
		while (b < end)
		{
		  if (*a  &&  *b)
		  {
			if (*c) (**c) += (*a)->transposeTimes (**b);
			else     (*c)  = (*a)->transposeTimes (**b).relinquish ();
		  }
		  a++;
		  b++;
		}
	  }

	  // Element on the diagonal
	  MatrixAbstract<T> ** a   = Ablocks + blockColumn * blockStride;
	  MatrixAbstract<T> ** c   = Cblocks + blockColumn * AC + blockColumn;
	  MatrixAbstract<T> ** end = a + AR;
	  while (a < end)
	  {
		if (*a)
		{
		  if (*c) (**c) += (*a)->transposeSquare ();
		  else     (*c)  = (*a)->transposeSquare ().relinquish ();
		}
		a++;
	  }
	}

	result->blockUpdateAll ();

	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixBlock<T>::transposeTimes (const MatrixAbstract<T> & B) const
  {
	int AR = startRows   .size () - 1;
	int AC = startColumns.size () - 1;

	if (B.classID () & MatrixBlockID)
	{
	  const MatrixBlock & BB = (MatrixBlock &) B;

	  int BR = BB.startRows   .size () - 1;
	  int BC = BB.startColumns.size () - 1;

	  const int w = std::min (AR, BR);
	  MatrixBlock * result = new MatrixBlock (AC, BC);

	  MatrixAbstract<T> ** Ablocks = (MatrixAbstract<T> **)         data;
	  MatrixAbstract<T> ** Bblocks = (MatrixAbstract<T> **) BB.     data;
	  MatrixAbstract<T> ** c       = (MatrixAbstract<T> **) result->data;

	  for (int blockColumn = 0; blockColumn < BC; blockColumn++)
	  {
		for (int blockRow = 0; blockRow < AC; blockRow++)
		{
		  MatrixAbstract<T> ** a   = Ablocks + blockRow    *    blockStride;
		  MatrixAbstract<T> ** b   = Bblocks + blockColumn * BB.blockStride;
		  MatrixAbstract<T> ** end = b + w;
		  while (b < end)
		  {
			if (*a  &&  *b)
			{
			  if (*c) (**c) += (*a)->transposeTimes (**b);
			  else     (*c)  = (*a)->transposeTimes (**b).relinquish ();
			}
			a++;
			b++;
		  }
		  c++;
		}
	  }

	  result->blockUpdateAll ();

	  return result;
	}
	else
	{
	  int bh = B.rows ();
	  MatrixBlock * result = new MatrixBlock (AC);
	  result->startRows = startColumns;
	  result->startColumns[1] = B.columns ();

	  MatrixAbstract<T> ** Ablocks = (MatrixAbstract<T> **)         data;
	  MatrixAbstract<T> ** Cblocks = (MatrixAbstract<T> **) result->data;

	  for (int blockRow = 0; blockRow < AR; blockRow++)
	  {
		int r0 = startRows[blockRow];
		if (r0 >= bh) break;
		int r1 = std::min (bh, startRows[blockRow+1]) - 1;

		MatrixAbstract<T> ** a   = Ablocks + blockRow;
		MatrixResult<T>      b   = B.region (r0, 0, r1);
		MatrixAbstract<T> ** c   = Cblocks;
		MatrixAbstract<T> ** end = Cblocks + AC;

		for (int blockColumn = 0; blockColumn < AC; blockColumn++)
		{
		  if (*a)
		  {
			if (*c) (**c) += (*a)->transposeTimes (b);
			else      *c   = (*a)->transposeTimes (b).relinquish ();
		  }
		  a += blockStride;
		  c++;
		}
	  }

	  return result;
	}
  }

  template<class T>
  MatrixResult<T>
  MatrixBlock<T>::row (const int r) const
  {
	MatrixBlock * result = new MatrixBlock;

	int blockRow = binarySearch (startRows, r);
	if (blockRow >= 0)
	{
	  int r0 = r - startRows[blockRow];
	  int blockColumns = startColumns.size () - 1;
	  result->blockResize (1, blockColumns);
	  MatrixAbstract<T> ** from = ((MatrixAbstract<T> **)         data) + blockRow;
	  MatrixAbstract<T> ** to   =  (MatrixAbstract<T> **) result->data;
	  MatrixAbstract<T> ** end  = to + blockColumns;
	  while (to < end)
	  {
		if (*from) *to = (*from)->row (r0).relinquish ();
		to++;
		from += blockStride;
	  }
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixBlock<T>::column (const int c) const
  {
	MatrixBlock * result = new MatrixBlock;

	int blockColumn = binarySearch (startColumns, c);
	if (blockColumn >= 0)
	{
	  int c0 = c - startColumns[blockColumn];
	  int blockRows = startRows.size () - 1;
	  result->blockResize (blockRows, 1);
	  MatrixAbstract<T> ** from = ((MatrixAbstract<T> **)         data) + blockColumn * blockStride;
	  MatrixAbstract<T> ** to   =  (MatrixAbstract<T> **) result->data;
	  MatrixAbstract<T> ** end  = to + blockRows;
	  while (to < end)
	  {
		if (*from) *to = (*from)->column (c0).relinquish ();
		to++;
		from++;
	  }
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixBlock<T>::operator * (const MatrixAbstract<T> & B) const
  {
	int AR = startRows   .size () - 1;
	int AC = startColumns.size () - 1;

	if (B.classID () & MatrixBlockID)
	{
	  const MatrixBlock & BB = (MatrixBlock &) B;

	  int BR = BB.startRows   .size () - 1;
	  int BC = BB.startColumns.size () - 1;

	  const int w = std::min (AC, BR);
	  MatrixBlock * result = new MatrixBlock (AR, BC);

	  MatrixAbstract<T> ** Ablocks = (MatrixAbstract<T> **)         data;
	  MatrixAbstract<T> ** Bblocks = (MatrixAbstract<T> **) BB.     data;
	  MatrixAbstract<T> ** c       = (MatrixAbstract<T> **) result->data;

	  for (int blockColumn = 0; blockColumn < BC; blockColumn++)
	  {
		for (int blockRow = 0; blockRow < AR; blockRow++)
		{
		  MatrixAbstract<T> ** a = &Ablocks[blockRow];
		  MatrixAbstract<T> ** b = &Bblocks[blockColumn * BB.blockStride];
		  MatrixAbstract<T> ** end = b + w;
		  while (b < end)
		  {
			if (*a  &&  *b)
			{
			  if (*c) (**c) +=  (**a) * (**b);
			  else     (*c)  = ((**a) * (**b)).relinquish ();
			}
			a += blockStride;
			b++;
		  }
		  c++;
		}
	  }

	  result->blockUpdateAll ();

	  return result;
	}
	else
	{
	  int bh = B.rows ();
	  MatrixBlock * result = new MatrixBlock (AR);
	  result->startRows = startRows; // force rows to match A, even if B is defficient
	  result->startColumns[1] = B.columns ();

	  MatrixAbstract<T> ** a       = (MatrixAbstract<T> **)         data;
	  MatrixAbstract<T> ** Cblocks = (MatrixAbstract<T> **) result->data;

	  for (int blockColumn = 0; blockColumn < AC; blockColumn++)
	  {
		int r0 = startColumns[blockColumn];
		if (r0 >= bh) break;
		int r1 = std::min (bh, startColumns[blockColumn+1]) - 1;

		MatrixResult<T>      b = B.region (r0, 0, r1);
		MatrixAbstract<T> ** c = Cblocks;

		for (int blockRow = 0; blockRow < AR; blockRow++)
		{
		  if (*a)
		  {
			if (*c) (**c) +=  (**a) * b;
			else      *c   = ((**a) * b).relinquish ();
		  }
		  a++;
		  c++;
		}
	  }

	  return result;
	}
  }

  template<class T>
  MatrixResult<T>
  MatrixBlock<T>::operator / (const T scalar) const
  {
	int AR = startRows   .size () - 1;
	int AC = startColumns.size () - 1;
	MatrixBlock * result = new MatrixBlock (AR, AC);
	result->startRows    = startRows;
	result->startColumns = startColumns;

	MatrixAbstract<T> ** a   = (MatrixAbstract<T> **)         data;
	MatrixAbstract<T> ** c   = (MatrixAbstract<T> **) result->data;
	MatrixAbstract<T> ** end = c + AR * AC;
	int stepA = blockStride - AR;
	while (c < end)
	{
	  MatrixAbstract<T> ** columnEnd = c + AR;
	  while (c < columnEnd)
	  {
		if (*a) *c = ((**a) / scalar).relinquish ();
		a++;
		c++;
	  }
	  a += stepA;
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  MatrixBlock<T>::operator - (const MatrixAbstract<T> & B) const
  {
	int AR = startRows   .size () - 1;
	int AC = startColumns.size () - 1;
	MatrixBlock * result = new MatrixBlock (AR, AC);
	result->startRows    = startRows;
	result->startColumns = startColumns;

	if (B.classID () & MatrixBlockID)
	{
	  const MatrixBlock & BB = (MatrixBlock &) B;

	  int BR = BB.startRows   .size () - 1;
	  int BC = BB.startColumns.size () - 1;

	  const int oh    = std::min (AR, BR);
	  const int ow    = std::min (AC, BC);
	  const int stepA =    blockStride - AR;
	  const int stepB = BB.blockStride - oh;

	  MatrixAbstract<T> ** a   = (MatrixAbstract<T> **)         data;
	  MatrixAbstract<T> ** b   = (MatrixAbstract<T> **) BB.     data;
	  MatrixAbstract<T> ** c   = (MatrixAbstract<T> **) result->data;
	  MatrixAbstract<T> ** end = c + AR * ow;
	  while (c < end)
	  {
		MatrixAbstract<T> ** overlapEnd = c + oh;
		MatrixAbstract<T> ** columnEnd  = c + AR;
		while (c < overlapEnd)
		{
		  if (*a)
		  {
			if (*b) *c = (**a - **b).relinquish ();
			else    *c = (*a)->clone ();
		  }
		  else
		  {
			if (*b) *c = (**b * -1).relinquish ();
			// else both entries are null, so the output should also be null
		  }
		  a++;
		  b++;
		  c++;
		}
		while (c < columnEnd)
		{
		  if (*a) *c = (*a)->clone (true);  // don't alias, because this is the result of a computation
		  a++;
		  c++;
		}
		a += stepA;
		b += stepB;
	  }
	  end += AR * (AC - ow);  // plan is to fill out the remaining columns with values from A
	  while (c < end)
	  {
		MatrixAbstract<T> ** columnEnd = c + AR;
		while (c < columnEnd)
		{
		  if (*a) *c = (*a)->clone (true);
		  a++;
		  c++;
		}
		a += stepA;
	  }
	}
	else
	{
	  const int oh    = std::min (startRows   .back (), B.rows ());
	  const int ow    = std::min (startColumns.back (), B.columns ());
	  const int stepA = blockStride - AR;

	  MatrixAbstract<T> ** a   = (MatrixAbstract<T> **)         data;
	  MatrixAbstract<T> ** c   = (MatrixAbstract<T> **) result->data;
	  MatrixAbstract<T> ** end = c + AR * AC;
	  int blockColumn = 0;
	  while (c < end)
	  {
		int c0 = startColumns[blockColumn];
		if (c0 >= ow) break;
		int c1 = std::min (ow, startColumns[blockColumn+1]) - 1;

		MatrixAbstract<T> ** columnEnd = c + AR;
		int blockRow = 0;
		while (c < columnEnd)
		{
		  int r0 = startRows[blockRow];
		  if (r0 >= oh) break;
		  int r1 = std::min (oh, startRows[blockRow+1]) - 1;

		  MatrixResult<T> b = B.region (r0, c0, r1, c1);
		  if (*a)
		  {
			*c = (**a - b).relinquish ();
		  }
		  else
		  {
			if (b.norm (INFINITY)) *c = (b * -1).relinquish ();
		  }
		  a++;
		  c++;
		  blockRow++;
		}
		while (c < columnEnd)
		{
		  if (*a) *c = (*a)->clone (true);
		  a++;
		  c++;
		}

		a += stepA;
		blockColumn++;
	  }
	  while (c < end)
	  {
		MatrixAbstract<T> ** columnEnd = c + AR;
		while (c < columnEnd)
		{
		  if (*a) *c = (*a)->clone (true);
		  a++;
		  c++;
		}
		a += stepA;
	  }
	}

	return result;
  }

  template<class T>
  void
  MatrixBlock<T>::serialize (Archive & archive, uint32_t version)
  {
	archive & startRows;
	archive & startColumns;

	int blockRows    = startRows   .size () - 1;
	int blockColumns = startColumns.size () - 1;
	if (archive.in)
	{
	  if (! archive.in->good ()) throw "MatrixBlock: can't finish reading because stream is bad";
	  std::vector<int> tempRows = startRows;
	  std::vector<int> tempCols = startColumns;
	  detach ();
	  blockResize (blockRows, blockColumns);
	  startRows    = tempRows;
	  startColumns = tempCols;
	}

	MatrixAbstract<T> ** blocks = (MatrixAbstract<T> **) data;
	for (int c = 0; c < blockColumns; c++)
	{
	  for (int r = 0; r < blockRows; r++)
	  {
		archive & blocks[c * blockStride + r];
	  }
	}
  }
}

#endif
