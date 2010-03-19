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


#ifndef fl_levenberg_marquardt_sparse_bk_tcc
#define fl_levenberg_marquardt_sparse_bk_tcc


#include "fl/search.h"
#include "fl/math.h"

#include <float.h>
#include <algorithm>

#undef small  // Somehow, this gets defined (as char) under MSVC x64


namespace fl
{
  // class SparseBK -----------------------------------------------------------

  template<class T>
  class SparseBK : public MatrixSparse<T>
  {
  public:
	SparseBK () : MatrixSparse<T> () {}
	SparseBK (int rows, int columns) : MatrixSparse<T> (rows, columns) {}

	// Determines largest off-diagonal value in given column
	// Does not clear return values before search, so this can update results
	// of a previous search.
	void colmax (const int column, int & row, T & value) const
	{
	  std::map<int,T> & C = (*this->data)[column];
	  typename std::map<int,T>::iterator i;
	  for (i = C.begin (); i != C.end ()  &&  i->first < column; i++)
	  {
		T temp = std::fabs (i->second);
		if (temp > value)
		{
		  row = i->first;
		  value = temp;
		}
	  }
	}

	void swap (const int row1, const int column1, const int row2, const int column2)
	{
	  std::map<int,T> & C1 = (*this->data)[column1];
	  std::map<int,T> & C2 = (*this->data)[column2];
	  std::pair< typename std::map<int,T>::iterator, bool > r1 = C1.insert (std::make_pair (row1, (T) 0));
	  std::pair< typename std::map<int,T>::iterator, bool > r2 = C2.insert (std::make_pair (row2, (T) 0));
	  if (r1.second)  // this(row1,column1) == 0
	  {
		if (r2.second)  // this(row2,column2) == 0
		{
		  C1.erase (r1.first);
		  C2.erase (r2.first);
		}
		else  // this(row2,column2) != 0
		{
		  r1.first->second = r2.first->second;
		  C2.erase (r2.first);
		}
	  }
	  else  // this(row1,column1) != 0
	  {
		if (r2.second)  // this(row2,column2) == 0
		{
		  r2.first->second = r1.first->second;
		  C1.erase (r1.first);
		}
		else  // this(row2,column2) != 0
		{
		  std::swap (r1.first->second, r2.first->second);
		}
	  }
	}

	void swap (const int column1, const int column2, const int lastRow)
	{
	  std::map<int,T> & C1 = (*this->data)[column1];
	  std::map<int,T> & C2 = (*this->data)[column2];
	  typename std::map<int,T>::iterator i1 = C1.begin ();
	  typename std::map<int,T>::iterator i2 = C2.begin ();
	  // Assume that there is always another element in each column beyond lastRow.
	  while (i1->first <= lastRow  ||  i2->first <= lastRow)
	  {
		if (i1->first == i2->first)
		{
		  std::swap (i1->second, i2->second);
		  i1++;
		  i2++;
		}
		else if (i1->first > i2->first)
		{
		  // Transfer element from C2 to C1
		  C1.insert (i1, *i2);
		  typename std::map<int,T>::iterator t = i2;
		  i2++;
		  C2.erase (t);
		}
		else  // i1->first < i2->first
		{
		  // Transfer element from C1 to C2
		  C2.insert (i2, *i1);
		  typename std::map<int,T>::iterator t = i1;
		  i1++;
		  C1.erase (t);
		}
	  }
	}

	void updateRank1 (const int column)
	{
	  std::map<int,T> & Ck = (*this->data)[column];
	  typename std::map<int,T>::reverse_iterator j = Ck.rbegin ();
	  typename std::map<int,T>::reverse_iterator CkEnd = Ck.rend ();
	  if (j == CkEnd  ||  j->first != column)
	  {
		throw "SparseBK::updateRank1: diagonal element is zero";
	  }
	  T alpha = j->second;
	  j++;
	  while (j != CkEnd)
	  {
		T temp = - j->second / alpha;

		std::map<int,T> & Cj = (*this->data)[j->first];
		typename std::map<int,T>::reverse_iterator ij = Cj.rbegin ();
		typename std::map<int,T>::reverse_iterator ik = j;
		typename std::map<int,T>::reverse_iterator CjEnd = Cj.rend ();
		while (ij != CjEnd  ||  ik != CkEnd)
		{
		  int ijRow = ij == CjEnd ? -1 : ij->first;
		  int ikRow = ik == CkEnd ? -1 : ik->first;
		  if (ijRow == ikRow)
		  {
			ij->second += ik->second * temp;
			ij++;
			ik++;
		  }
		  else if (ijRow < ikRow)
		  {
			// A(i,k) != 0, so must create element in A(i,j)
			Cj.insert (ij.base (), *ik)->second *= temp;
			if (ij != CjEnd)
			{
			  ij++;
			}
			ik++;
		  }
		  else  // ijRow > ikRow
		  {
			// A(i,k) == 0, so no action
			ij++;
		  }
		}

		j->second = -temp;
		j++;
	  }
	}

	void updateRank2 (const int column)
	{
	  std::map<int,T> & Ck  = (*this->data)[column];
	  std::map<int,T> & Ck1 = (*this->data)[column - 1];
	  typename std::map<int,T>::reverse_iterator jk  = Ck.rbegin ();
	  typename std::map<int,T>::reverse_iterator jk1 = Ck1.rbegin ();
	  typename std::map<int,T>::reverse_iterator CkEnd  = Ck.rend ();
	  typename std::map<int,T>::reverse_iterator Ck1End = Ck1.rend ();

	  if (jk == CkEnd  ||  jk1 == Ck1End  ||  jk->first != column  ||  jk1->first != column - 1)
	  {
		throw "SparseBK::updateRank2: diagonal element is zero";
	  }

	  T d11 = (jk++)->second;

	  T d12 = (T) 0;
	  if (jk->first == column - 1)
	  {
		d12 = jk->second;
		jk++;
	  }

	  T d22 = (jk1++)->second;

	  T temp = d12;
	  d12 = d11 * d22 / d12 - d12;
	  d22 /= temp;
	  d11 /= temp;

	  while (jk != CkEnd  ||  jk1 != Ck1End)
	  {
		int jkRow  = jk  == CkEnd  ? -1 : jk->first;
		int jk1Row = jk1 == Ck1End ? -1 : jk1->first;
		int j;
		T Ajk;
		T Ajk1;
		if (jkRow == jk1Row)
		{
		  j = jkRow;  // or jk1Row
		  Ajk  = jk ->second;
		  Ajk1 = jk1->second;
		}
		else if (jkRow < jk1Row)
		{
		  j = jk1Row;
		  Ajk  = (T) 0;
		  Ajk1 = jk1->second;
		}
		else  // jkRow > jk1Row
		{
		  j = jkRow;
		  Ajk  = jk ->second;
		  Ajk1 = (T) 0;
		}
		T wk1 = (d11 * Ajk1 - Ajk)  / d12;
		T wk  = (d22 * Ajk  - Ajk1) / d12;

		std::map<int,T> & Cj = (*this->data)[j];
		typename std::map<int,T>::reverse_iterator ij = Cj.rbegin ();
		typename std::map<int,T>::reverse_iterator ik = jk;
		typename std::map<int,T>::reverse_iterator ik1 = jk1;
		typename std::map<int,T>::reverse_iterator CjEnd = Cj.rend ();
		while (ij != CjEnd  ||  ik != CkEnd  ||  ik1 != Ck1End)
		{
		  int ikRow  = ik  == CkEnd  ? -1 : ik->first;
		  int ik1Row = ik1 == Ck1End ? -1 : ik1->first;
		  int i;
		  T Aik;
		  T Aik1;
		  if (ikRow == ik1Row)
		  {
			i = ikRow;
			Aik  = ik ->second;
			Aik1 = ik1->second;
			ik++;
			ik1++;
		  }
		  else if (ikRow < ik1Row)
		  {
			i = ik1Row;
			Aik  = (T) 0;
			Aik1 = ik1->second;
			ik1++;
		  }
		  else  // ikRow > ik1Row
		  {
			i = ikRow;
			Aik  = ik ->second;
			Aik1 = (T) 0;
			ik++;
		  }
		  temp = Aik * wk + Aik1 * wk1;

		  while (ij != CjEnd  &&  ij->first > i)
		  {
			ij++;
		  }
		  if (ij != CjEnd  &&  ij->first == i)
		  {
			ij->second -= temp;
			ij++;
		  }
		  else
		  {
			if (temp != (T) 0)
			{
			  // A(i,k) != 0, so must create element in A(i,j)
			  Cj.insert (ij.base (), std::make_pair (i, -temp));
			  if (ij != CjEnd)
			  {
				ij++;
			  }
			}
		  }
		}

		if (jkRow == jk1Row)
		{
		  jk->second = wk;
		  jk1->second = wk1;
		  jk++;
		  jk1++;
		}
		else if (jkRow < jk1Row)
		{
		  if (wk != (T) 0)
		  {
			Ck.insert (jk.base (), std::make_pair (j, wk));
			if (jk != CkEnd)
			{
			  jk++;
			}
		  }
		  jk1->second = wk1;
		  jk1++;
		}
		else  // jkRow > jk1Row
		{
		  jk->second = wk;
		  if (wk1 != (T) 0)
		  {
			Ck1.insert (jk1.base (), std::make_pair (j, wk1));
			if (jk1 != Ck1End)
			{
			  jk1++;
			}
		  }
		  jk++;
		}
	  }
	}

	void minus (const int column, const int lastRow, Vector<T> & x) const
	{
	  T alpha = x[column];
	  if (alpha == (T) 0)
	  {
		return;
	  }
	  std::map<int,T> & C = (*this->data)[column];
	  typename std::map<int,T>::iterator i = C.begin ();
	  while (i->first <= lastRow  &&  i != C.end ())
	  {
		x[i->first] -= i->second * alpha;
		i++;
	  }
	}

	T dot (const int column, const int lastRow, const Vector<T> & x) const
	{
	  T result = (T) 0;
	  std::map<int,T> & C = (*this->data)[column];
	  typename std::map<int,T>::iterator i = C.begin ();
	  while (i != C.end ()  &&  i->first <= lastRow)
	  {
		result += x[i->first] * i->second;
		i++;
	  }
	  return result;
	}

	// Return the upper triangular part of ~this * this in a new matrix.
	SparseBK transposeSquare () const
	{
	  int n = this->data->size ();
	  SparseBK result (n, n);

	  for (int c = 0; c < n; c++)
	  {
		std::map<int,T> & C = (*result.data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		for (int r = 0; r <= c; r++)
		{
		  T t = (T) 0;
		  std::map<int,T> & C1 = (*this->data)[r];
		  std::map<int,T> & C2 = (*this->data)[c];
		  typename std::map<int,T>::iterator i1 = C1.begin ();
		  typename std::map<int,T>::iterator i2 = C2.begin ();
		  while (i1 != C1.end ()  &&  i2 != C2.end ())
		  {
			if (i1->first == i2->first)
			{
			  t += i1->second * i2->second;
			  i1++;
			  i2++;
			}
			else if (i1->first > i2->first)
			{
			  i2++;
			}
			else  // i1->first < i2->first
			{
			  i1++;
			}
		  }
		  if (t != (T) 0)
		  {
			i = C.insert (i, std::make_pair (r, t));
		  }
		}
	  }

	  return result;
	}

	Vector<T> transposeMult (const Vector<T> & x) const
	{
	  int n = this->data->size ();
	  Vector<T> result (n);

	  for (int c = 0; c < n; c++)
	  {
		result[c] = 0;
		std::map<int,T> & C = (*this->data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result[c] += x[i->first] * i->second;
		  i++;
		}
	  }

	  return result;
	}

	Vector<T> operator * (const Vector<T> & x) const
	{
	  int n = this->data->size ();

	  Vector<T> result (this->rows_);
	  result.clear ();

	  for (int c = 0; c < n; c++)
	  {
		std::map<int,T> & C = (*this->data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result[i->first] += i->second * x[c];
		  i++;
		}
	  }

	  return result;
	}

	/*  For testing only.  Multiplies x by full matrix, even though stored only as upper triangle.
	Vector<T> trimult (const Vector<T> & x) const
	{
	  const int n = data->size ();

	  Vector<T> result (rows_);
	  result.clear ();

	  for (int c = 0; c < n; c++)
	  {
		std::map<int,T> & C = (*data)[c];
		typename std::map<int,T>::iterator i = C.begin ();
		while (i != C.end ())
		{
		  result[i->first] += i->second * x[c];
		  if (i->first < c)
		  {
			result[c] += i->second * x[i->first];
		  }
		  i++;
		}
	  }

	  return result;
	}
	*/

	void addDiagonal (const T alpha, const Vector<T> & x)
	{
	  int n = this->data->size ();

	  for (int j = 0; j < n; j++)
	  {
		T value = alpha * x[j] * x[j];

		std::map<int,T> & C = (*this->data)[j];
		typename std::map<int,T>::reverse_iterator i = C.rbegin ();
		while (i->first > j)  // This should never happen.
		{
		  i++;
		}
		if (i->first == j)
		{
		  i->second += value;
		}
		else  // This also should never happen.
		{
		  C.insert (i.base (), std::make_pair (j, value));
		}
	  }
	}

	T norm2 (const int column)
	{
	  T result = (T) 0;
	  std::map<int,T> & C = (*this->data)[column];
	  typename std::map<int,T>::iterator i = C.begin ();
	  while (i != C.end ())
	  {
		result += i->second * i->second;
		i++;
	  }
	  return std::sqrt (result);
	}
  };


  // Support functions --------------------------------------------------------

  /*  For testing only.  Recreates original matrix from factored form.
  template<class T>
  void
  reconstruct (MatrixAbstract<T> & A, Vector<int> & ipiv, Matrix<T> & B)
  {
	int n = A.rows ();
	Matrix<T> U (n, n);
	U.identity ();
	Matrix<T> D (n, n);
	D.clear ();
	int k = n - 1;
	while (k >= 0)
	{
	  Matrix<T> PUk (n, n);
	  PUk.identity ();
	  if (ipiv[k] > 0)
	  {
		int j = ipiv[k] - 1;
		MatrixRegion<T> (PUk, 0, k, k-1, k) <<= MatrixRegion<T> (A, 0, k, k-1, k);
		if (j != k)
		{
		  Matrix<T> temp;
		  temp <<= PUk.row (k);
		  PUk.row (k) <<= PUk.row (j);
		  PUk.row (j) <<= temp;
		}
		U *= PUk;
		D(k,k) = A(k,k);
		k -= 1;
	  }
	  else  // ipiv[k] < 0 and ipiv[k-1] < 0
	  {
		int j = -ipiv[k] - 1;
		MatrixRegion<T> (PUk, 0, k-1, k-2, k) <<= MatrixRegion<T> (A, 0, k-1, k-2, k);
		if (j != k-1)
		{
		  Matrix<T> temp;
		  temp <<= PUk.row (k-1);
		  PUk.row (k-1) <<= PUk.row (j);
		  PUk.row (j) <<= temp;
		}
		U *= PUk;
		D(k,k) = A(k,k);
		D(k-1,k-1) = A(k-1,k-1);
		D(k-1,k) = A(k-1,k);
		D(k,k-1) = D(k-1,k);
		k -= 2;
	  }
	}
	//cerr << (B - U * D * ~U) << endl << endl;
	cerr << (B - U * D * ~U).norm (2) << endl;
  }
  */


  template<class T>
  static void
  factorize (const int maxPivot, SparseBK<T> & A, Vector<int> & pivots)
  {
	// Factorize A as U*D*U' using the upper triangle of A

	const T alpha = (1.0 + std::sqrt (17.0)) / 8.0;
	int n = A.columns ();

	pivots.resize (n);

	// K is the main loop index, decreasing from N to 1 in steps of
	// 1 or 2
	int k = n - 1;
	while (k >= 0)
	{
	  // Determine rows and columns to be interchanged and whether
	  // a 1-by-1 or 2-by-2 pivot block will be used

	  int kstep = 1;

	  T absakk = std::fabs (A(k,k));

	  // IMAX is the row-index of the largest off-diagonal element in
	  // column K, and COLMAX is its absolute value
	  int imax = 0;
	  T colmax = 0;
	  A.colmax (k, imax, colmax);

	  int kp;
	  if (! (std::max (absakk, colmax) > 0))
	  {
		throw -k;
		// kp = k;
	  }
	  else
	  {
		if ((k - imax) > maxPivot  ||  absakk >= alpha * colmax)
		{
		  // no interchange, use 1-by-1 pivot block
		  kp = k;
		}
		else
		{
		  // JMAX is the column-index of the largest off-diagonal
		  // element in row IMAX, and ROWMAX is its absolute value
		  int jmax = 0;
		  T rowmax = 0;
		  A.colmax (imax, jmax, rowmax);
		  for (int j = imax + 1; j <= k; j++)
		  {
			// no need to update jmax, because it is not used below
			rowmax = std::max (rowmax, std::fabs (A(imax,j)));
		  }

		  if (absakk >= alpha * colmax * colmax / rowmax)
		  {
			// no interchange, use 1-by-1 pivot block
			kp = k;
		  }
		  else if (std::fabs (A(imax,imax)) >= alpha * rowmax)
		  {
			// interchange rows and columns K and IMAX, use 1-by-1 pivot block
			kp = imax;
		  }
		  else
		  {
			// interchange rows and columns K-1 and IMAX, use 2-by-2 pivot block
			kp = imax;
			kstep = 2;
		  }
		}

		int kk = k - kstep + 1;
		if (kp != kk)  // then kp < kk
		{
		  // Interchange rows and columns KK and KP in the leading
		  // submatrix A(1:k,1:k)
		  A.swap (kk, kp, kp - 1);  // exchange columns kk and kp up to row kp-1
		  for (int j = kp + 1; j < kk; j++)
		  {
			A.swap (j, kk, kp, j);  // exchange elements (j,kk) and (kp,j)
		  }
		  A.swap (kk, kk, kp, kp);
		  if (kstep == 2)
		  {
			A.swap (k-1, k, kp, k);
		  }
		}

		// Update the leading submatrix
		if (kstep == 1)
		{
		  // 1-by-1 pivot block D(k): column k now holds W(k) = U(k)*D(k)
		  // where U(k) is the k-th column of U

		  // Perform a rank-1 update of A(1:k-1,1:k-1) as
		  // A := A - U(k)*D(k)*U(k)' = A - W(k)*1/D(k)*W(k)'
		  // and store U(k) in column k
		  A.updateRank1 (k);
		}
		else
		{
		  // 2-by-2 pivot block D(k): columns k and k-1 now hold
		  // ( W(k-1) W(k) ) = ( U(k-1) U(k) )*D(k)
		  // where U(k) and U(k-1) are the k-th and (k-1)-th columns of U

		  // Perform a rank-2 update of A(1:k-2,1:k-2) as
		  // A := A - ( U(k-1) U(k) )*D(k)*( U(k-1) U(k) )'
		  //    = A - ( W(k-1) W(k) )*inv(D(k))*( W(k-1) W(k) )'
		  A.updateRank2 (k);
		}
	  }

	  // Store details of the interchanges in IPIV
	  // Pivot values must be one-based so that negation can work.  The output
	  // of this routine is therefore compatible with dsytf2, etc.
	  kp++;
	  if (kstep == 1)
	  {
		pivots[k] = kp;
	  }
	  else
	  {
		pivots[k] = -kp;
		pivots[k-1] = -kp;
	  }

	  // Decrease K
	  k -= kstep;
	}
  }

  template<class T>
  static void
  solve (const SparseBK<T> & A, const Vector<int> & pivots, Vector<T> & x, const Vector<T> & b)
  {
	// Solve A*X = B, where A = U*D*U'.

	int n = A.columns ();

	x.copyFrom (b);

	// First solve U*D*X = B
	// K is the main loop index, decreasing from N-1 to 0 in steps of
	// 1 or 2, depending on the size of the diagonal blocks.
	int k = n - 1;
	while (k >= 0)
	{
	  if (pivots[k] > 0)
	  {
		// 1 x 1 diagonal block

		// Interchange rows K and IPIV(K).
		int kp = pivots[k] - 1;
		if (kp != k)
		{
		  std::swap (x[k], x[kp]);
		}

		// Multiply by inv(U(K)), where U(K) is the transformation
		// stored in column K of A.
		A.minus (k, k - 1, x);

		// Multiply by the inverse of the diagonal block.
		x[k] /= A(k,k);

		k--;
	  }
	  else
	  {
		// 2 x 2 diagonal block

		// Interchange rows K-1 and -IPIV(K).
		int kp = - pivots[k] - 1;
		if (kp != k - 1)
		{
		  std::swap (x[k-1], x[kp]);
		}

		// Multiply by inv(U(K)), where U(K) is the transformation
		// stored in columns K-1 and K of A.
		A.minus (k,     k - 2, x);
		A.minus (k - 1, k - 2, x);

		// Multiply by the inverse of the diagonal block.
		T akm1k = A(k-1,k);
		T akm1  = A(k-1,k-1) / akm1k;
		T ak    = A(k,k)     / akm1k;
		T denom = akm1 * ak - 1;
		T bkm1  = x[k-1] / akm1k;
		T bk    = x[k]   / akm1k;
		x[k-1] = (ak   * bkm1 - bk)   / denom;
		x[k]   = (akm1 * bk   - bkm1) / denom;

		k -= 2;
	  }
	}

	// Next solve U'*X = B
	// K is the main loop index, increasing from 0 to N-1 in steps of
	// 1 or 2, depending on the size of the diagonal blocks.
	k = 0;
	while (k < n)
	{
	  if (pivots[k] > 0)
	  {
		// 1 x 1 diagonal block

		// Multiply by inv(U'(K)), where U(K) is the transformation
		// stored in column K of A.
		x[k] -= A.dot (k, k - 1, x);

		// Interchange rows K and IPIV(K)
		int kp = pivots[k] - 1;
		if (kp != k)
		{
		  std::swap (x[k], x[kp]);
		}

		k++;
	  }
	  else
	  {
		// 2 x 2 diagonal block

		// Multiply by inv(U'(K+1)), where U(K+1) is the transformation
		// stored in columns K and K+1 of A.
		x[k]   -= A.dot (k,     k - 1, x);
		x[k+1] -= A.dot (k + 1, k - 1, x);

		// Interchange rows K and -IPIV(K).
		int kp = - pivots[k] - 1;
		if (kp != k)
		{
		  std::swap (x[k], x[kp]);
		}

		k += 2;
	  }
	}
  }

  /**
	 Calculate Euclidean norm.  Secial method from MINPACK to handle wide
	 range of magnitudes.
  **/
  template<class T>
  static T
  enorm (Vector<T> & x)
  {
	const T rdwarf = (T) 3.834e-20;  // rdwarf^2 must not underflow in float type T
	const T rgiant = (T) 1.304e19;  // rgiant^2 must not overflow in float type T
	const T agiant = rgiant / x.rows ();

	T large = 0;
	T intermediate = 0;
	T small = 0;
	T largeMax = 0;
	T smallMax = 0;

	for (int i = 0; i < x.rows (); i++)
	{
	  T xabs = std::fabs (x[i]);
	  if (xabs <= rdwarf)
	  {
		// sum for small components.
		if (xabs > smallMax)
		{
		  T t = smallMax / xabs;
		  small = (T) 1.0 + small * t * t;
		  smallMax = xabs;
		}
		else if (xabs != (T) 0)
		{
		  T t = xabs / smallMax;
		  small += t * t;
		}
	  }
	  else if (xabs < agiant)
	  {
		// sum for intermediate components.
		intermediate += xabs * xabs;
	  }
	  else
	  {
		// sum for large components.
		if (xabs > largeMax)
		{
		  T t = largeMax / xabs;
		  large = (T) 1.0 + large * t * t;
		  largeMax = xabs;
		}
		else
		{
		  T t = xabs / largeMax;
		  large += t * t;
		}
	  }
	}

	// calculation of norm.
	if (large != (T) 0)
	{
	  return largeMax * std::sqrt (large + (intermediate / largeMax) / largeMax);
	}
	else if (intermediate != (T) 0)
	{
	  if (intermediate >= smallMax)
	  {
		return std::sqrt (intermediate * ((T) 1.0 + (smallMax / intermediate) * (smallMax * small)));
	  }
	  else
	  {
		return std::sqrt (smallMax * ((intermediate / smallMax) + (smallMax * small)));
	  }
	}
	else
	{
	  return smallMax * std::sqrt (small);
	}
  }

  template<class T>
  static void
  lmpar (const SparseBK<T> & fjac, const Vector<T> & diag, const Vector<T> & fvec, const int maxPivot, T delta, T & par, Vector<T> & x)
  {
	int n = fjac.columns ();

	// Compute and store in x the gauss-newton direction.
	// ~fjac * fjac * x = ~fjac * fvec
	Vector<T> Jf = fjac.transposeMult (fvec);
	SparseBK<T> JJ = fjac.transposeSquare ();
	SparseBK<T> factoredJJ;
	factoredJJ.copyFrom (JJ);
	Vector<int> ipvt;
	factorize (maxPivot, factoredJJ, ipvt);
	solve (factoredJJ, ipvt, x, Jf);

	// Evaluate the function at the origin, and test
	// for acceptance of the gauss-newton direction.
	Vector<T> dx (n);
	for (int j = 0; j < n; j++)
	{
	  dx[j] = diag[j] * x[j];
	}
	T dxnorm = enorm (dx);
	T fp = dxnorm - delta;
std::cerr << "fp=" << fp << " " << dxnorm << " " << delta << std::endl;
	if (fp <= (T) 0.1 * delta)
	{
	  par = 0;
	  return;
	}

	// The jacobian is required to have full rank, so the newton
	// step provides a lower bound, parl, for the zero of
	// the function.
	Vector<T> wa1 (n);
	for (int j = 0; j < n; j++)
	{
	  wa1[j] = diag[j] * dx[j] / dxnorm;
	}
	Vector<T> wa2;
	solve (factoredJJ, ipvt, wa2, wa1);
	T parl = std::max ((T) 0, fp / (delta * wa1.dot (wa2)));

	// Calculate an upper bound, paru, for the zero of the function.
	for (int j = 0; j < n; j++)
	{
	  wa1[j] = Jf[j] / diag[j];
	}
	T gnorm = wa1.norm (2);
	T paru = gnorm / delta;
	if (paru == (T) 0)
	{
	  paru = LevenbergMarquardtSparseBK<T>::minimum / std::min (delta, (T) 0.1);
	}

	// If the input par lies outside of the interval (parl,paru),
	// set par to the closer endpoint.
	par = std::max (par, parl);
	par = std::min (par, paru);
	if (par == (T) 0)
	{
	  par = gnorm / dxnorm;
	}

	int iter = 0;
	while (true)
	{
	  iter++;

	  // Evaluate the function at the current value of par.
	  if (par == (T) 0)
	  {
		par = std::max (LevenbergMarquardtSparseBK<T>::minimum, (T) 0.001 * paru);
	  }
	  factoredJJ.copyFrom (JJ);
	  factoredJJ.addDiagonal (par, diag);
	  factorize (maxPivot, factoredJJ, ipvt);
	  solve (factoredJJ, ipvt, x, Jf);

	  for (int j = 0; j < n; j++)
	  {
		dx[j] = diag[j] * x[j];
	  }
	  dxnorm = dx.norm (2);
	  T oldFp = fp;
	  fp = dxnorm - delta;

std::cerr << "par=" << par << " " << parl << " " << paru << " " << fp << " " << delta << std::endl;
	  // If the function is small enough, accept the current value
	  // of par.  Also test for the exceptional cases where parl
	  // is zero or the number of iterations has reached 10.
	  if (   std::fabs (fp) <= (T) 0.1 * delta
		  || (parl == (T) 0  &&  fp <= oldFp  &&  oldFp < (T) 0)
		  || iter >= 10)
	  {
		return;
	  }

	  // Compute the newton correction.
	  for (int j = 0; j < n; j++)
	  {
		wa1[j] = diag[j] * dx[j] / dxnorm;
	  }
	  solve (factoredJJ, ipvt, wa2, wa1);
	  T parc = fp / (delta * wa1.dot (wa2));

	  // Depending on the sign of the function, update parl or paru.
	  if (fp > (T) 0)
	  {
		parl = std::max (parl, par);
	  }
	  if (fp < (T) 0)
	  {
		paru = std::min (paru, par);
	  }
	  // Compute an improved estimate for par.
	  par = std::max (parl, par + parc);
	}
  }


  // class LevenbergMarquardtSparseBK -----------------------------------------

  template<class T>
  LevenbergMarquardtSparseBK<T>::LevenbergMarquardtSparseBK (T toleranceF, T toleranceX, int maxIterations, int maxPivot)
  {
	if (toleranceF < (T) 0)
	{
	  toleranceF = std::sqrt (epsilon);
	}
	this->toleranceF = toleranceF;

	if (toleranceX < (T) 0)
	{
	  toleranceX = std::sqrt (epsilon);
	}
	this->toleranceX = toleranceX;

	this->maxIterations = maxIterations;
	this->maxPivot = maxPivot;
  }

  /**
	 A loose paraphrase the MINPACK function lmdif
  **/
  template<class T>
  void
  LevenbergMarquardtSparseBK<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	const T toleranceG = (T) 0;

	// Evaluate the function at the starting point and calculate its norm.
	Vector<T> fvec;
	searchable.value (point, fvec);

	int m = fvec.rows ();
	int n = point.rows ();

	SparseBK<T> fjac (m, n);
	Vector<T> diag (n);  // scales
	T par = (T) 0;  // levenberg-marquardt parameter
	T fnorm = fvec.norm (2);
	T xnorm;
	T delta;

	// outer loop
	int iter = 0;
	while (true)
	{
	  iter++;

	  // calculate the jacobian matrix
	  searchable.jacobian (point, fjac, &fvec);

	  // compute the qr factorization of the jacobian.
	  Vector<T> jacobianNorms (n);
	  for (int j = 0; j < n; j++)
	  {
		jacobianNorms[j] = fjac.norm2 (j);
	  }

	  // On the first iteration ...
	  if (iter == 1)
	  {
		// Scale according to the norms of the columns of the initial jacobian.
		for (int j = 0; j < n; j++)
		{
		  diag[j] = jacobianNorms[j];
		  if (diag[j] == (T) 0)
		  {
			diag[j] = (T) 1;
		  }
		}

		// Calculate the norm of the scaled x and initialize the step bound delta.
		xnorm = (T) 0;
		for (int j = 0; j < n; j++)
		{
		  T temp = diag[j] * point[j];
		  xnorm += temp * temp;
		}
		xnorm = std::sqrt (xnorm);

		const T factor = (T) 1;
		delta = factor * xnorm;
		if (delta == (T) 0)
		{
		  delta = factor;
		}
	  }

	  // compute the norm of the scaled gradient
	  T gnorm = (T) 0;
	  if (fnorm != (T) 0)
	  {
		for (int j = 0; j < n; j++)
		{
		  if (jacobianNorms[j] != (T) 0)
		  {
			T value = fjac.dot (j, m - 1, fvec);
			gnorm = std::max (gnorm, std::fabs (value / (fnorm * jacobianNorms[j])));
		  }
		}
	  }

	  // test for convergence of the gradient norm
	  if (gnorm <= toleranceG)
	  {
		//info = 4;
		return;
	  }

	  // rescale if necessary
	  for (int j = 0; j < n; j++)
	  {
		diag[j] = std::max (diag[j], jacobianNorms[j]);
	  }

	  // beginning of the inner loop
	  T ratio = (T) 0;
	  while (ratio < (T) 0.0001)
	  {
		// determine the levenberg-marquardt parameter.
		Vector<T> p (n);  // wa1
		lmpar (fjac, diag, fvec, maxPivot, delta, par, p);

		// store the direction p and x + p. calculate the norm of p.
		Vector<T> xp = point - p;  // p is actually negative
		T pnorm = 0;
		for (int j = 0; j < n; j++)
		{
		  T temp = diag[j] * p[j];
		  pnorm += temp * temp;
		}
		pnorm = std::sqrt (pnorm);

		// on the first iteration, adjust the initial step bound
		if (iter == 1)
		{
		  delta = std::min (delta, pnorm);
		}

		// evaluate the function at x + p and calculate its norm
		Vector<T> tempFvec;
		searchable.value (xp, tempFvec);
		T fnorm1 = tempFvec.norm (2);

		// compute the scaled actual reduction
		T actred = (T) -1;
		if (fnorm1 / 10 < fnorm)
		{
		  T temp = fnorm1 / fnorm;
		  actred = (T) 1 - temp * temp;
		}
std::cerr << "actred=" << actred << " " << fnorm1 << " " << fnorm << std::endl;

		// compute the scaled predicted reduction and the scaled directional derivative
		T temp1 = (fjac * p).norm (2) / fnorm;
		T temp2 = std::sqrt (par) * pnorm / fnorm;
		T prered = temp1 * temp1 + (T) 2 * temp2 * temp2;
		T dirder = -(temp1 * temp1 + temp2 * temp2);

		// compute the ratio of the actual to the predicted reduction
		ratio = (T) 0;
		if (prered != (T) 0)
		{
		  ratio = actred / prered;
		}

		// update the step bound
		if (ratio <= (T) 0.25)
		{
		  T update;
		  if (actred >= (T) 0)
		  {
			update = (T) 0.5;
		  }
		  else
		  {
			update = dirder / ((T) 2 * dirder + actred);
		  }
		  if (fnorm1 / 10 >= fnorm  ||  update < (T) 0.1)
		  {
			update = (T) 0.1;
		  }
		  delta = update * std::min (delta, pnorm * 10);
		  par /= update;
		}
		else
		{
		  if (par == (T) 0  ||  ratio >= (T) 0.75)
		  {
			delta = pnorm * 2;
			par /= 2;
		  }
		}

		// test for successful iteration.
		if (ratio >= (T) 0.0001)
		{
		  // successful iteration. update x, fvec, and their norms.
		  point = xp;
		  fvec = tempFvec;

		  xnorm = 0;
		  for (int j = 0; j < n; j++)
		  {
			T temp = diag[j] * point[j];
			xnorm += temp * temp;
		  }
		  xnorm = std::sqrt (xnorm);

		  fnorm = fnorm1;
		}
std::cerr << "ratio=" << ratio << std::endl;

		// tests for convergence
		if (   std::fabs (actred) <= toleranceF
			&& prered <= toleranceF
			&& ratio <= 2)
		{
		  // info = 1;
		  return;
		}
		if (delta <= toleranceX * xnorm)
		{
		  // info = 2;
		  return;
		}

		// tests for termination and stringent tolerances
		if (iter > maxIterations)
		{
		  throw (int) 5;
		}
		if (   std::fabs (actred) <= epsilon
			&& prered <= epsilon
			&& ratio <= 2)
		{
		  throw (int) 6;
		}
		if (delta <= epsilon * xnorm)
		{
		  throw (int) 7;
		}
		if (gnorm <= epsilon)
		{
		  throw (int) 8;
		}
	  }
	}
  }
}


#endif
