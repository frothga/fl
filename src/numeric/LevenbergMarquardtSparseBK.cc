#include "fl/search.h"

#include <float.h>
#include <algorithm>


// For debugging
//#include <iostream>


using namespace fl;
using namespace std;


// Support functions ----------------------------------------------------------

/*  For testing only.  Recreates original matrix from factored form.
void
reconstruct (MatrixAbstract<double> & A, Vector<int> & ipiv, Matrix<double> & B)
{
  int n = A.rows ();
  Matrix<double> U (n, n);
  U.identity ();
  Matrix<double> D (n, n);
  D.clear ();
  int k = n - 1;
  while (k >= 0)
  {
	Matrix<double> PUk (n, n);
	PUk.identity ();
	if (ipiv[k] > 0)
	{
	  int j = ipiv[k] - 1;
	  MatrixRegion<double> (PUk, 0, k, k-1, k) <<= MatrixRegion<double> (A, 0, k, k-1, k);
	  if (j != k)
	  {
		Matrix<double> temp;
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
      MatrixRegion<double> (PUk, 0, k-1, k-2, k) <<= MatrixRegion<double> (A, 0, k-1, k-2, k);
	  if (j != k-1)
	  {
		Matrix<double> temp;
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
  cerr << (B - U * D * ~U).frob (2) << endl;
}
*/

class SparseBK : public MatrixSparse<double>
{
public:
  SparseBK () : MatrixSparse<double> () {}
  SparseBK (int rows, int columns) : MatrixSparse<double> (rows, columns) {}

  // Determines largest off-diagonal value in given column
  // Does not clear return values before search, so this can update results
  // of a previous search.
  void colmax (const int column, int & row, double & value) const
  {
	map<int,double> & C = (*data)[column];
	map<int,double>::iterator i;
	for (i = C.begin (); i != C.end ()  &&  i->first < column; i++)
	{
	  double temp = fabs (i->second);
	  if (temp > value)
	  {
		row = i->first;
		value = temp;
	  }
	}
  }

  void swap (const int row1, const int column1, const int row2, const int column2)
  {
	map<int,double> & C1 = (*data)[column1];
	map<int,double> & C2 = (*data)[column2];
	pair< map<int,double>::iterator, bool > r1 = C1.insert (make_pair (row1, 0));
	pair< map<int,double>::iterator, bool > r2 = C2.insert (make_pair (row2, 0));
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
	map<int,double> & C1 = (*data)[column1];
	map<int,double> & C2 = (*data)[column2];
	map<int,double>::iterator i1 = C1.begin ();
	map<int,double>::iterator i2 = C2.begin ();
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
		map<int,double>::iterator t = i2;
		i2++;
		C2.erase (t);
	  }
	  else  // i1->first < i2->first
	  {
		// Transfer element from C1 to C2
		C2.insert (i2, *i1);
		map<int,double>::iterator t = i1;
		i1++;
		C1.erase (t);
	  }
	}
  }

  void updateRank1 (const int column)
  {
	map<int,double> & Ck = (*data)[column];
	map<int,double>::reverse_iterator j = Ck.rbegin ();
	map<int,double>::reverse_iterator CkEnd = Ck.rend ();
	if (j == CkEnd  ||  j->first != column)
	{
	  throw "SparseBK::updateRank1: diagonal element is zero";
	}
	double alpha = j->second;
	j++;
	while (j != CkEnd)
	{
	  double temp = - j->second / alpha;

	  map<int,double> & Cj = (*data)[j->first];
	  map<int,double>::reverse_iterator ij = Cj.rbegin ();
	  map<int,double>::reverse_iterator ik = j;
	  map<int,double>::reverse_iterator CjEnd = Cj.rend ();
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
	map<int,double> & Ck  = (*data)[column];
	map<int,double> & Ck1 = (*data)[column - 1];
	map<int,double>::reverse_iterator jk  = Ck.rbegin ();
	map<int,double>::reverse_iterator jk1 = Ck1.rbegin ();
	map<int,double>::reverse_iterator CkEnd  = Ck.rend ();
	map<int,double>::reverse_iterator Ck1End = Ck1.rend ();

	if (jk == CkEnd  ||  jk1 == Ck1End  ||  jk->first != column  ||  jk1->first != column - 1)
	{
	  throw "SparseBK::updateRank2: diagonal element is zero";
	}

	double d11 = (jk++)->second;

	double d12 = 0;
	if (jk->first == column - 1)
	{
	  d12 = jk->second;
	  jk++;
	}

	double d22 = (jk1++)->second;

	double temp = d12;
	d12 = d11 * d22 / d12 - d12;
	d22 /= temp;
	d11 /= temp;

	while (jk != CkEnd  ||  jk1 != Ck1End)
	{
	  int jkRow  = jk  == CkEnd  ? -1 : jk->first;
	  int jk1Row = jk1 == Ck1End ? -1 : jk1->first;
	  int j;
	  double Ajk;
	  double Ajk1;
	  if (jkRow == jk1Row)
	  {
		j = jkRow;  // or jk1Row
		Ajk  = jk ->second;
		Ajk1 = jk1->second;
	  }
	  else if (jkRow < jk1Row)
	  {
		j = jk1Row;
		Ajk  = 0;
		Ajk1 = jk1->second;
	  }
	  else  // jkRow > jk1Row
	  {
		j = jkRow;
		Ajk  = jk ->second;
		Ajk1 = 0;
	  }
	  double wk1 = (d11 * Ajk1 - Ajk)  / d12;
	  double wk  = (d22 * Ajk  - Ajk1) / d12;

	  map<int,double> & Cj = (*data)[j];
	  map<int,double>::reverse_iterator ij = Cj.rbegin ();
	  map<int,double>::reverse_iterator ik = jk;
	  map<int,double>::reverse_iterator ik1 = jk1;
	  map<int,double>::reverse_iterator CjEnd = Cj.rend ();
	  while (ij != CjEnd  ||  ik != CkEnd  ||  ik1 != Ck1End)
	  {
		int ikRow  = ik  == CkEnd  ? -1 : ik->first;
		int ik1Row = ik1 == Ck1End ? -1 : ik1->first;
		int i;
		double Aik;
		double Aik1;
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
		  Aik  = 0;
		  Aik1 = ik1->second;
		  ik1++;
		}
		else  // ikRow > ik1Row
		{
		  i = ikRow;
		  Aik  = ik ->second;
		  Aik1 = 0;
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
		  if (temp != 0)
		  {
			// A(i,k) != 0, so must create element in A(i,j)
			Cj.insert (ij.base (), make_pair (i, -temp));
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
		if (wk != 0)
		{
		  Ck.insert (jk.base (), make_pair (j, wk));
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
		if (wk1 != 0)
		{
		  Ck1.insert (jk1.base (), make_pair (j, wk1));
		  if (jk1 != Ck1End)
		  {
			jk1++;
		  }
		}
		jk++;
	  }
	}
  }

  void minus (const int column, const int lastRow, Vector<double> & x) const
  {
	double alpha = x[column];
	if (alpha == 0)
	{
	  return;
	}
	map<int,double> & C = (*data)[column];
	map<int,double>::iterator i = C.begin ();
	while (i->first <= lastRow  &&  i != C.end ())
	{
	  x[i->first] -= i->second * alpha;
	  i++;
	}
  }

  double dot (const int column, const int lastRow, const Vector<double> & x) const
  {
	double result = 0;
	map<int,double> & C = (*data)[column];
	map<int,double>::iterator i = C.begin ();
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
	int n = data->size ();
	SparseBK result (n, n);

	for (int c = 0; c < n; c++)
	{
	  map<int,double> & C = (*result.data)[c];
	  map<int,double>::iterator i = C.begin ();
	  for (int r = 0; r <= c; r++)
	  {
		double t = 0;
		map<int,double> & C1 = (*data)[r];
		map<int,double> & C2 = (*data)[c];
		map<int,double>::iterator i1 = C1.begin ();
		map<int,double>::iterator i2 = C2.begin ();
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
		if (t != 0)
		{
		  i = C.insert (i, make_pair (r, t));
		}
	  }
	}

	return result;
  }

  Vector<double> transposeMult (const Vector<double> & x) const
  {
	int n = data->size ();
	Vector<double> result (n);

	for (int c = 0; c < n; c++)
	{
	  result[c] = 0;
	  map<int,double> & C = (*data)[c];
	  map<int,double>::iterator i = C.begin ();
	  while (i != C.end ())
	  {
		result[c] += x[i->first] * i->second;
		i++;
	  }
	}

	return result;
  }

  Vector<double> operator * (const Vector<double> & x) const
  {
	int n = data->size ();

	Vector<double> result (rows_);
	result.clear ();

	for (int c = 0; c < n; c++)
	{
	  map<int,double> & C = (*data)[c];
	  map<int,double>::iterator i = C.begin ();
	  while (i != C.end ())
	  {
		result[i->first] += i->second * x[c];
		i++;
	  }
	}

	return result;
  }

  /*  For testing only.  Multiplies x by full matrix, even though stored only as upper triangle.
  Vector<double> trimult (const Vector<double> & x) const
  {
	const int n = data->size ();

	Vector<double> result (rows_);
	result.clear ();

	for (int c = 0; c < n; c++)
	{
	  map<int,double> & C = (*data)[c];
	  map<int,double>::iterator i = C.begin ();
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

  void addDiagonal (const double alpha, const Vector<double> & x)
  {
	int n = data->size ();

	for (int j = 0; j < n; j++)
	{
	  double value = alpha * x[j] * x[j];

	  map<int,double> & C = (*data)[j];
	  map<int,double>::reverse_iterator i = C.rbegin ();
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
		C.insert (i.base (), make_pair (j, value));
	  }
	}
  }

  double frob2 (const int column)
  {
	double result = 0;
	map<int,double> & C = (*data)[column];
	map<int,double>::iterator i = C.begin ();
	while (i != C.end ())
	{
	  result += i->second * i->second;
	  i++;
	}
	return sqrt (result);
  }
};

static void
factorize (const int maxPivot, SparseBK & A, Vector<int> & pivots)
{
  // Factorize A as U*D*U' using the upper triangle of A

  const double alpha = (1.0 + sqrt (17.0)) / 8.0;
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

	double absakk = fabs (A(k,k));

	// IMAX is the row-index of the largest off-diagonal element in
	// column K, and COLMAX is its absolute value
	int imax = 0;
	double colmax = 0;
	A.colmax (k, imax, colmax);

	int kp;
	if (! (max (absakk, colmax) > 0))
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
		double rowmax = 0;
		A.colmax (imax, jmax, rowmax);
		for (int j = imax + 1; j <= k; j++)
		{
		  // no need to update jmax, because it is not used below
		  rowmax = max (rowmax, fabs (A(imax,j)));
		}

		if (absakk >= alpha * colmax * colmax / rowmax)
		{
		  // no interchange, use 1-by-1 pivot block
		  kp = k;
		}
		else if (fabs (A(imax,imax)) >= alpha * rowmax)
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

static void
solve (const SparseBK & A, const Vector<int> & pivots, Vector<double> & x, const Vector<double> & b)
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
		swap (x[k], x[kp]);
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
		swap (x[k-1], x[kp]);
	  }

	  // Multiply by inv(U(K)), where U(K) is the transformation
	  // stored in columns K-1 and K of A.
	  A.minus (k,     k - 2, x);
	  A.minus (k - 1, k - 2, x);

	  // Multiply by the inverse of the diagonal block.
	  double akm1k = A(k-1,k);
	  double akm1  = A(k-1,k-1) / akm1k;
	  double ak    = A(k,k)     / akm1k;
	  double denom = akm1 * ak - 1;
	  double bkm1  = x[k-1] / akm1k;
	  double bk    = x[k]   / akm1k;
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
		swap (x[k], x[kp]);
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
		swap (x[k], x[kp]);
	  }

	  k += 2;
	}
  }
}

static double
enorm (Vector<double> & x)
{
  // Use special method from MINPACK to handle wide range of magnitudes

  const double rdwarf = 3.834e-20;
  const double rgiant = 1.304e19;
  const double agiant = rgiant / x.rows ();

  double large = 0;
  double intermediate = 0;
  double small = 0;
  double largeMax = 0;
  double smallMax = 0;

  for (int i = 0; i < x.rows (); i++)
  {
	double xabs = fabs (x[i]);
	if (xabs <= rdwarf)
	{
	  // sum for small components.
	  if (xabs > smallMax)
	  {
		double t = smallMax / xabs;
		small = 1.0 + small * t * t;
		smallMax = xabs;
	  }
	  else if (xabs != 0)
	  {
		double t = xabs / smallMax;
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
		double t = largeMax / xabs;
		large = 1.0 + large * t * t;
		largeMax = xabs;
	  }
	  else
	  {
		double t = xabs / largeMax;
		large += t * t;
	  }
	}
  }

  // calculation of norm.
  if (large != 0)
  {
	return largeMax * sqrt (large + (intermediate / largeMax) / largeMax);
  }
  else if (intermediate != 0)
  {
	if (intermediate >= smallMax)
	{
	  return sqrt (intermediate * (1.0 + (smallMax / intermediate) * (smallMax * small)));
	}
	else
	{
	  return sqrt (smallMax * ((intermediate / smallMax) + (smallMax * small)));
	}
  }
  else
  {
	return smallMax * sqrt (small);
  }
}

static void
lmpar (const SparseBK & fjac, const Vector<double> & diag, const Vector<double> & fvec, const int maxPivot, double delta, double & par, Vector<double> & x)
{
  int n = fjac.columns ();

  // Compute and store in x the gauss-newton direction.
  // ~fjac * fjac * x = ~fjac * fvec
  Vector<double> Jf = fjac.transposeMult (fvec);
  SparseBK JJ = fjac.transposeSquare ();
  SparseBK factoredJJ;
  factoredJJ.copyFrom (JJ);
  Vector<int> ipvt;
  factorize (maxPivot, factoredJJ, ipvt);
  solve (factoredJJ, ipvt, x, Jf);

  // Evaluate the function at the origin, and test
  // for acceptance of the gauss-newton direction.
  Vector<double> dx (n);
  for (int j = 0; j < n; j++)
  {
	dx[j] = diag[j] * x[j];
  }
  double dxnorm = enorm (dx);
  double fp = dxnorm - delta;
cerr << "fp=" << fp << " " << dxnorm << " " << delta << endl;
  if (fp <= 0.1 * delta)
  {
	par = 0;
	return;
  }

  // The jacobian is required to have full rank, so the newton
  // step provides a lower bound, parl, for the zero of
  // the function.
  Vector<double> wa1 (n);
  for (int j = 0; j < n; j++)
  {
	wa1[j] = diag[j] * dx[j] / dxnorm;
  }
  Vector<double> wa2;
  solve (factoredJJ, ipvt, wa2, wa1);
  double parl = max (0.0, fp / (delta * wa1.dot (wa2)));

  // Calculate an upper bound, paru, for the zero of the function.
  for (int j = 0; j < n; j++)
  {
	wa1[j] = Jf[j] / diag[j];
  }
  double gnorm = wa1.frob (2);
  double paru = gnorm / delta;
  if (paru == 0)
  {
	paru = DBL_MIN / min (delta, 0.1);
  }

  // If the input par lies outside of the interval (parl,paru),
  // set par to the closer endpoint.
  par = max (par, parl);
  par = min (par, paru);
  if (par == 0)
  {
	par = gnorm / dxnorm;
  }

  int iter = 0;
  while (true)
  {
	iter++;

	// Evaluate the function at the current value of par.
	if (par == 0)
	{
	  par = max (DBL_MIN, 0.001 * paru);
	}
	factoredJJ.copyFrom (JJ);
	factoredJJ.addDiagonal (par, diag);
	factorize (maxPivot, factoredJJ, ipvt);
	solve (factoredJJ, ipvt, x, Jf);

	for (int j = 0; j < n; j++)
	{
	  dx[j] = diag[j] * x[j];
	}
	dxnorm = dx.frob (2);
	double oldFp = fp;
	fp = dxnorm - delta;

cerr << "par=" << par << " " << parl << " " << paru << " " << fp << " " << delta << endl;
	// If the function is small enough, accept the current value
	// of par.  Also test for the exceptional cases where parl
	// is zero or the number of iterations has reached 10.
	if (   fabs (fp) <= 0.1 * delta
	    || (parl == 0  &&  fp <= oldFp  &&  oldFp < 0)
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
	double parc = fp / (delta * wa1.dot (wa2));

	// Depending on the sign of the function, update parl or paru.
	if (fp > 0)
	{
	  parl = max (parl, par);
	}
	if (fp < 0)
	{
	  paru = min (paru, par);
	}
	// Compute an improved estimate for par.
	par = max (parl, par + parc);
  }
}


// class LevenbergMarquardtSparseBK -------------------------------------------

LevenbergMarquardtSparseBK::LevenbergMarquardtSparseBK (double toleranceF, double toleranceX, int maxIterations, int maxPivot)
{
  if (toleranceF < 0)
  {
	toleranceF = sqrt (DBL_EPSILON);
  }
  this->toleranceF = toleranceF;

  if (toleranceX < 0)
  {
	toleranceX = sqrt (DBL_EPSILON);
  }
  this->toleranceX = toleranceX;

  this->maxIterations = maxIterations;
  this->maxPivot = maxPivot;
}

void
LevenbergMarquardtSparseBK::search (Searchable & searchable, Vector<double> & point)
{
  // The following is a loose paraphrase the MINPACK function lmdif

  const double toleranceG = 0;

  // Evaluate the function at the starting point and calculate its norm.
  Vector<double> fvec;
  searchable.value (point, fvec);

  int m = fvec.rows ();
  int n = point.rows ();

  SparseBK fjac (m, n);
  Vector<double> diag (n);  // scales
  double par = 0;  // levenberg-marquardt parameter
  double fnorm = fvec.frob (2);
  double xnorm;
  double delta;

  // outer loop
  int iter = 0;
  while (true)
  {
	iter++;

	// calculate the jacobian matrix
	searchable.jacobian (point, fjac, &fvec);

	// compute the qr factorization of the jacobian.
	Vector<double> jacobianNorms (n);
	for (int j = 0; j < n; j++)
	{
	  jacobianNorms[j] = fjac.frob2 (j);
	}

	// On the first iteration ...
	if (iter == 1)
	{
	  // Scale according to the norms of the columns of the initial jacobian.
	  for (int j = 0; j < n; j++)
	  {
		diag[j] = jacobianNorms[j];
		if (diag[j] == 0)
		{
		  diag[j] = 1;
		}
	  }

	  // Calculate the norm of the scaled x and initialize the step bound delta.
	  xnorm = 0;
	  for (int j = 0; j < n; j++)
	  {
		double temp = diag[j] * point[j];
		xnorm += temp * temp;
	  }
	  xnorm = sqrt (xnorm);

	  const double factor = 1;
	  delta = factor * xnorm;
	  if (delta == 0)
	  {
		delta = factor;
	  }
	}

	// compute the norm of the scaled gradient
	double gnorm = 0;
	if (fnorm != 0)
	{
	  for (int j = 0; j < n; j++)
	  {
		if (jacobianNorms[j] != 0)
		{
		  double value = fjac.dot (j, m - 1, fvec);
		  gnorm = max (gnorm, fabs (value / (fnorm * jacobianNorms[j])));
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
	  diag[j] = max (diag[j], jacobianNorms[j]);
	}

	// beginning of the inner loop
	double ratio = 0;
	while (ratio < 0.0001)
	{
	  // determine the levenberg-marquardt parameter.
	  Vector<double> p (n);  // wa1
	  lmpar (fjac, diag, fvec, maxPivot, delta, par, p);

	  // store the direction p and x + p. calculate the norm of p.
	  Vector<double> xp = point - p;  // p is actually negative
	  double pnorm = 0;
	  for (int j = 0; j < n; j++)
	  {
		double temp = diag[j] * p[j];
		pnorm += temp * temp;
	  }
	  pnorm = sqrt (pnorm);

	  // on the first iteration, adjust the initial step bound
	  if (iter == 1)
	  {
		delta = min (delta, pnorm);
	  }

	  // evaluate the function at x + p and calculate its norm
	  Vector<double> tempFvec;
	  searchable.value (xp, tempFvec);
	  double fnorm1 = tempFvec.frob (2);

	  // compute the scaled actual reduction
	  double actred = -1;
	  if (fnorm1 / 10 < fnorm)
	  {
		double temp = fnorm1 / fnorm;
		actred = 1 - temp * temp;
	  }
cerr << "actred=" << actred << " " << fnorm1 << " " << fnorm << endl;

	  // compute the scaled predicted reduction and the scaled directional derivative
	  double temp1 = (fjac * p).frob (2) / fnorm;
	  double temp2 = sqrt (par) * pnorm / fnorm;
	  double prered = temp1 * temp1 + 2 * temp2 * temp2;
	  double dirder = -(temp1 * temp1 + temp2 * temp2);

	  // compute the ratio of the actual to the predicted reduction
	  ratio = 0;
	  if (prered != 0)
	  {
		ratio = actred / prered;
	  }

	  // update the step bound
	  if (ratio <= 0.25)
	  {
		double update;
		if (actred >= 0)
		{
		  update = 0.5;
		}
		else
		{
		  update = dirder / (2 * dirder + actred);
		}
		if (fnorm1 / 10 >= fnorm  ||  update < 0.1)
		{
		  update = 0.1;
		}
		delta = update * min (delta, pnorm * 10);
		par /= update;
	  }
	  else
	  {
		if (par == 0  ||  ratio >= 0.75)
		{
		  delta = pnorm * 2;
		  par /= 2;
		}
	  }

	  // test for successful iteration.
	  if (ratio >= 0.0001)
	  {
	    // successful iteration. update x, fvec, and their norms.
		point = xp;
		fvec = tempFvec;

		xnorm = 0;
		for (int j = 0; j < n; j++)
		{
		  double temp = diag[j] * point[j];
		  xnorm += temp * temp;
		}
		xnorm = sqrt (xnorm);

		fnorm = fnorm1;
	  }
cerr << "ratio=" << ratio << endl;

	  // tests for convergence
	  if (   fabs (actred) <= toleranceF
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
	  if (   fabs (actred) <= DBL_EPSILON
		  && prered <= DBL_EPSILON
		  && ratio <= 2)
	  {
		throw (int) 6;
	  }
	  if (delta <= DBL_EPSILON * xnorm)
	  {
		throw (int) 7;
	  }
	  if (gnorm <= DBL_EPSILON)
	  {
		throw (int) 8;
	  }
	}
  }
}
