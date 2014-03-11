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


#ifndef fl_levenberg_marquardt_sparse_tcc
#define fl_levenberg_marquardt_sparse_tcc


#include "fl/search.h"
#include "fl/math.h"

#include <float.h>
#include <algorithm>
#include <limits>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


#undef small  // Somehow, this gets defined (as char) under MSVC x64


namespace fl
{
  template<class T>
  class SHARED FactorizationBKsparse : public Factorization<T>
  {
  public:
	FactorizationBKsparse ()
	{
	  maxPivot = INT_MAX;
	}

	int maxPivot;
	MatrixSparse<T> A;
	Vector<int> pivots;

	/// Factorize A as U*D*U' using the upper triangle of A
	virtual void factorize (const MatrixAbstract<T> & inputA, bool destroyA = false)
	{
	  const T alpha = (1.0 + std::sqrt (17.0)) / 8.0;
	  int n = inputA.columns ();
	  pivots.resize (n);

	  // Only copy the upper triangular region
	  A.clear ();
	  A.resize (n, n);
	  for (int c = 0; c < n; c++)
	  {
		for (int r = 0; r <= c; r++)
		{
		  T element = inputA(r,c);
		  if (element) A.set (r, c, element);
		}
	  }

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
		this->colmax (k, imax, colmax);

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
			this->colmax (imax, jmax, rowmax);
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
			swap (kk, kp, kp - 1);  // exchange columns kk and kp up to row kp-1
			for (int j = kp + 1; j < kk; j++)
			{
			  swap (j, kk, kp, j);  // exchange elements (j,kk) and (kp,j)
			}
			swap (kk, kk, kp, kp);
			if (kstep == 2)
			{
			  swap (k-1, k, kp, k);
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
			updateRank1 (k);
		  }
		  else
		  {
			// 2-by-2 pivot block D(k): columns k and k-1 now hold
			// ( W(k-1) W(k) ) = ( U(k-1) U(k) )*D(k)
			// where U(k) and U(k-1) are the k-th and (k-1)-th columns of U

			// Perform a rank-2 update of A(1:k-2,1:k-2) as
			// A := A - ( U(k-1) U(k) )*D(k)*( U(k-1) U(k) )'
			//    = A - ( W(k-1) W(k) )*inv(D(k))*( W(k-1) W(k) )'
			updateRank2 (k);
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

	/// Solve A*X = B, where A = U*D*U'.
	virtual MatrixResult<T> solve (const MatrixAbstract<T> & B, bool destroyB = false)
	{
	  Matrix<T> * X = new Matrix<T>;
	  X->copyFrom (B);

	  int n = A.columns ();

	  for (int c = 0; c < X->columns (); c++)
	  {
		Vector<T> x (&(*X)(0,c), X->rows ());

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
			minus (k, k - 1, x);

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
			minus (k,     k - 2, x);
			minus (k - 1, k - 2, x);

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
			x[k] -= dot (k, k - 1, x);

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
			x[k]   -= dot (k,     k - 1, x);
			x[k+1] -= dot (k + 1, k - 1, x);

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

	  return X;
	}

	virtual MatrixResult<T> invert ()
	{
	  throw "FactorizationBKsparse::inverse() not implemented yet";
	}

	// Determines largest off-diagonal value in given column
	// Does not clear return values before search, so this can update results
	// of a previous search.
	void colmax (const int column, int & row, T & value) const
	{
	  std::map<int,T> & C = (*A.data)[column];
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
	  std::map<int,T> & C1 = (*A.data)[column1];
	  std::map<int,T> & C2 = (*A.data)[column2];
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
	  std::map<int,T> & C1 = (*A.data)[column1];
	  std::map<int,T> & C2 = (*A.data)[column2];
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
	  std::map<int,T> & Ck = (*A.data)[column];
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

		std::map<int,T> & Cj = (*A.data)[j->first];
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
	  std::map<int,T> & Ck  = (*A.data)[column];
	  std::map<int,T> & Ck1 = (*A.data)[column - 1];
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

		std::map<int,T> & Cj = (*A.data)[j];
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
	  std::map<int,T> & C = (*A.data)[column];
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
	  std::map<int,T> & C = (*A.data)[column];
	  typename std::map<int,T>::iterator i = C.begin ();
	  while (i != C.end ()  &&  i->first <= lastRow)
	  {
		result += x[i->first] * i->second;
		i++;
	  }
	  return result;
	}

	void addDiagonal (const T alpha, const Vector<T> & x)
	{
	  int n = A.data->size ();

	  for (int j = 0; j < n; j++)
	  {
		T value = alpha * x[j] * x[j];

		std::map<int,T> & C = (*A.data)[j];
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
  };


  // class LevenbergMarquardtSparse -------------------------------------------

  template<class T>
  LevenbergMarquardtSparse<T>::LevenbergMarquardtSparse (T toleranceF, T toleranceX, int maxIterations)
  {
	if (toleranceF < (T) 0) toleranceF = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceF = toleranceF;

	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceX = toleranceX;

	this->maxIterations = maxIterations;

#   ifdef HAVE_LAPACK
	this->method = new FactorizationSymmetric<T>;  // Better stability, with little extra overhead (the squared Jacobian scales according to number of variables to solve, rather than number of equations).
#   else
	this->method = new FactorizationBKsparse<T>;
#   endif
  }

  template<class T>
  LevenbergMarquardtSparse<T>::~LevenbergMarquardtSparse ()
  {
	delete method;
  }

  /**
	 A loose paraphrase the MINPACK function lmdif
  **/
  template<class T>
  void
  LevenbergMarquardtSparse<T>::search (Searchable<T> & searchable, Vector<T> & x)
  {
	const T toleranceG = (T) 0;
	const T epsilon = std::numeric_limits<T>::epsilon ();

	// Variables that persist between iterations
	Vector<T> y;
	int oldm = -1;
	const int n = x.rows ();
	Vector<T> scales (n);
	T par = (T) 0;  // levenberg-marquardt parameter
	T ynorm;
	T xnorm;
	T delta;

	for (int iteration = 0; iteration < maxIterations; iteration++)
	{
	  const int m = searchable.dimension (x);
	  if (m != oldm)  // dimension has changed, so get fresh value of y
	  {
		y = searchable.value (x);
		ynorm = y.norm (2);
		oldm = m;
	  }

	  MatrixResult<T> J = searchable.jacobian (x, &y);
	  Vector<T> jacobianNorms (n);
	  for (int j = 0; j < n; j++) jacobianNorms[j] = J.column (j).norm (2);

	  // On the first iteration ...
	  if (iteration == 0)
	  {
		// Scale according to the norms of the columns of the initial jacobian.
		for (int j = 0; j < n; j++)
		{
		  scales[j] = jacobianNorms[j];
		  if (scales[j] == (T) 0) scales[j] = (T) 1;
		}

		// Calculate the norm of the scaled x and initialize the step bound delta.
		xnorm = (x & scales).norm (2);
		delta = (xnorm * (T) 1) == (T) 0 ? (T) 1 : xnorm;  // Does the multiplication by 1 here make a difference?  This seems to be a test of floating-point representation.
	  }

	  // compute the norm of the scaled gradient
	  T gnorm = (T) 0;
	  if (ynorm != (T) 0)
	  {
		MatrixResult<T> Jy = J.transposeTimes (y);
		for (int j = 0; j < n; j++)
		{
		  T jnorm = jacobianNorms[j];
		  if (jnorm != (T) 0) gnorm = std::max (gnorm, std::fabs (Jy[j] / (ynorm * jnorm)));
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
		scales[j] = std::max (scales[j], jacobianNorms[j]);
	  }

	  // beginning of the inner loop
	  T ratio = (T) 0;
	  while (ratio < (T) 0.0001)
	  {
		// determine the levenberg-marquardt parameter.
		Vector<T> p (n);
		lmpar (J, scales, y, delta, par, p);

		// store the direction p and x + p. calculate the norm of p.
		Vector<T> xp = x - p;  // p is actually negative
		T pnorm = (p & scales).norm (2);

		// on the first iteration, adjust the initial step bound
		if (iteration == 0)
		{
		  delta = std::min (delta, pnorm);
		}

		// evaluate the function at x + p and calculate its norm
		Vector<T> tempY = searchable.value (xp);
		T ynorm1 = tempY.norm (2);

		// compute the scaled actual reduction
		T reductionActual = (T) -1;
		if (ynorm1 / 10 < ynorm)
		{
		  T temp = ynorm1 / ynorm;
		  reductionActual = 1 - temp * temp;
		}
std::cerr << "reductionActual=" << reductionActual << " " << ynorm1 << " " << ynorm << std::endl;

		// compute the scaled predicted reduction and the scaled directional derivative
		T temp1 = (J * p).norm (2) / ynorm;
		T temp2 = std::sqrt (par) * pnorm / ynorm;
		T reductionPredicted = temp1 * temp1 + 2 * temp2 * temp2;
		T dirder = -(temp1 * temp1 + temp2 * temp2);

		// compute the ratio of the actual to the predicted reduction
		ratio = (T) 0;
		if (reductionPredicted != (T) 0)
		{
		  ratio = reductionActual / reductionPredicted;
		}

		// update the step bound
		if (ratio <= (T) 0.25)
		{
		  T update;
		  if (reductionActual >= (T) 0)
		  {
			update = (T) 0.5;
		  }
		  else
		  {
			update = dirder / (2 * dirder + reductionActual);
		  }
		  if (ynorm1 / 10 >= ynorm  ||  update < (T) 0.1)
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

		if (ratio >= (T) 0.0001)  // successful iteration.
		{
		  // update x, y, and their norms
		  x     = xp;
		  y     = tempY;
		  xnorm = (x & scales).norm (2);
		  ynorm = ynorm1;
		}
std::cerr << "ratio=" << ratio << std::endl;

		// tests for convergence
		if (   std::fabs (reductionActual) <= toleranceF
			&& reductionPredicted <= toleranceF
			&& ratio <= (T) 2)
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
		if (   std::fabs (reductionActual) <= epsilon
			&& reductionPredicted <= epsilon
			&& ratio <= (T) 2)
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

	throw (int) 5;  // exceeded maximum iterations
  }

  template<class T>
  void
  LevenbergMarquardtSparse<T>::lmpar (const MatrixAbstract<T> & J, const Vector<T> & scales, const Vector<T> & y, T delta, T & par, Vector<T> & x)
  {
	const T minimum = std::numeric_limits<T>::min ();
	const int n = J.columns ();

	// Compute and store in x the gauss-newton direction.
	// ~J * J * x = ~J * y
	MatrixResult<T> Jy = J.transposeTimes (y);
	MatrixResult<T> JJ = J.transposeSquare ();
	method->factorize (JJ);
	x = method->solve (Jy);

	// Evaluate the function at the origin, and test
	// for acceptance of the gauss-newton direction.
	Vector<T> dx = x & scales;
	T dxnorm = dx.norm (2);
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
	Vector<T> wa1 = dx & scales / dxnorm;
	Vector<T> wa2 = method->solve (wa1);
	T parl = std::max ((T) 0, fp / (delta * wa1.dot (wa2)));

	// Calculate an upper bound, paru, for the zero of the function.
	wa1 = Jy / scales;
	T gnorm = wa1.norm (2);
	T paru = gnorm / delta;
	if (paru == (T) 0)
	{
	  paru = minimum / std::min (delta, (T) 0.1);
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
		par = std::min (minimum, (T) 0.001 * paru);
	  }
	  Matrix<T> temp (JJ);
	  for (int i = 0; i < n; i++) temp(i,i) += scales[i] * scales[i] * par;
	  method->factorize (temp);
	  x = method->solve (Jy);

	  dx = x & scales;
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
	  wa1 = dx & scales / dxnorm;
	  wa2 = method->solve (wa1);
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
}


#endif
