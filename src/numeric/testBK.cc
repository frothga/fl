#include "fl/matrix.h"
#include "fl/random.h"
#include "fl/search.h"
#include "fl/lapackd.h"


#include <iostream>
#include <fstream>


using namespace std;
using namespace fl;


/*  This class structure demonstrates the overloaded function resolution
	"problem".  If you write:
	B a;
	B b;
	a * b;
	You will get a compile error.

class A
{
public:
  void operator * (const A & b) const;
};

class B : public A
{
public:
  void operator * (float f) const;
};
*/



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



extern "C"
{
  void dsytf2_ (const char & uplo,
				const int & n,
				double a[],
				const int & lda,
				int ipiv[],
				int & info);

  void dsytrs_ (const char & uplo,
				const int & n,
				const int & nrhs,
				double a[],
				const int & lda,
				int ipiv[],
				double b[],
				const int & ldb,
				int & info);
}




static void
factorize (Matrix<double> & A, Vector<int> & pivots)
{
  cerr << "factorize " << A.rows () << " " << A.columns () << endl;
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
	for (int i = 0; i < k; i++)
	{
	  double value = fabs (A(i,k));
	  if (value > colmax)
	  {
		imax = i;
		colmax = value;
	  }
	}

	int kp;
	if (max (absakk, colmax) == 0)
	{
	  // Error condition.  info = k.  However, plow on.
	  kp = k;
	}
	else
	{
	  if (absakk >= alpha * colmax)
	  {
		// no interchange, use 1-by-1 pivot block
		kp = k;
	  }
	  else
	  {
		// JMAX is the column-index of the largest off-diagonal
		// element in row IMAX, and ROWMAX is its absolute value
		// (jmax isn't used in this C++ adaptation)
		double rowmax = 0;
		for (int j = imax + 1; j <= k; j++)
		{
		  rowmax = max (rowmax, fabs (A(imax,j)));
		}
		for (int j = 0; j < imax; j++)
		{
		  rowmax = max (rowmax, fabs (A(j,imax)));
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
		for (int j = 0; j < kp; j++)
		{
		  swap (A(j,kk), A(j,kp));
		}
		for (int j = kp + 1; j < kk; j++)
		{
		  swap (A(j,kk), A(kp,j));
		}
		swap (A(kk,kk), A(kp,kp));
		if (kstep == 2)
		{
		  swap (A(k-1,k), A(kp,k));
		}
	  }

	  // Update the leading submatrix
	  if (kstep == 1)
	  {
		// 1-by-1 pivot block D(k): column k now holds W(k) = U(k)*D(k)
		// where U(k) is the k-th column of U

		// Perform a rank-1 update of A(1:k-1,1:k-1) as
		// A := A - U(k)*D(k)*U(k)' = A - W(k)*1/D(k)*W(k)'
		double dk = A(k,k);
		for (int j = 0; j < k; j++)
		{
		  if (A(j,k) != 0)
		  {
			double temp = -A(j,k) / dk;
			for (int i = 0; i <= j; i++)
			{
			  A(i,j) += A(i,k) * temp;
			}
		  }
		}

		// Store U(k) in column k
		MatrixRegion<double> (A, 0, k, k-1, k) /= dk;
	  }
	  else
	  {
		// 2-by-2 pivot block D(k): columns k and k-1 now hold
		// ( W(k-1) W(k) ) = ( U(k-1) U(k) )*D(k)
		// where U(k) and U(k-1) are the k-th and (k-1)-th columns of U

		// Perform a rank-2 update of A(1:k-2,1:k-2) as
		// A := A - ( U(k-1) U(k) )*D(k)*( U(k-1) U(k) )'
		//    = A - ( W(k-1) W(k) )*inv(D(k))*( W(k-1) W(k) )'
		double d12 = A(k-1,k);
		double d22 = A(k-1,k-1) / d12;
		double d11 = A(k,k) / d12;
		d12 = 1 / ((d11 * d22 - 1) * d12);

		for (int j = k - 2; j >= 0; j--)
		{
		  double wkm1 = d12 * (d11 * A(j,k-1) - A(j,k));
		  double wk   = d12 * (d22 * A(j,k)   - A(j,k-1));
		  for (int i = j; i >= 0; i--)
		  {
			A(i,j) -= A(i,k) * wk + A(i,k-1) * wkm1;
		  }
		  A(j,k)   = wk;
		  A(j,k-1) = wkm1;
		}
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
solve (const Matrix<double> & A, const Vector<int> & pivots, Vector<double> & x, const Vector<double> & b)
{
  cerr << "solve " << A.rows () << " " << A.columns () << endl;
  // Solve A*X = B, where A = U*D*U'.

  int n = A.columns ();

  x.copyFrom (b);

  // First solve U*D*X = B
  // K is the main loop index, decreasing from N to 1 in steps of
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
	  for (int i = 0; i < k; i++)
	  {
		x[i] -= A(i,k) * x[k];
	  }

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
	  for (int i = 0; i < k - 1; i++)
	  {
		x[i] -= A(i,k) * x[k];
	  }
	  for (int i = 0; i < k - 1; i++)
	  {
		x[i] -= A(i,k-1) * x[k-1];
	  }

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
  // K is the main loop index, increasing from 1 to N in steps of
  // 1 or 2, depending on the size of the diagonal blocks.
  k = 0;
  while (k < n)
  {
	if (pivots[k] > 0)
	{
	  // 1 x 1 diagonal block

	  // Multiply by inv(U'(K)), where U(K) is the transformation
	  // stored in column K of A.
	  for (int i = 0; i < k; i++)
	  {
		x[k] -= A(i,k) * x[i];
	  }

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
	  for (int i = 0; i < k; i++)
	  {
		x[k] -= A(i,k) * x[i];
	  }
	  for (int i = 0; i < k; i++)
	  {
		x[k+1] -= A(i,k+1) * x[i];
	  }

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







inline double
randbad ()
{
  double sign = randfb ();
  double a = fabs (sign);
  sign /= a;
  double b = 5 * randfb ();
  double result = sign * pow (a, b);
  return result;
}




int
main (int argc, char * argv[])
{
  #define parmFloat(n,d) (argc > n ? atof (argv[n]) : (d))
  #define parmInt(n,d) (argc > n ? atoi (argv[n]) : (d))

  int seed = parmInt (3, time (NULL));
  srand (seed);
  cerr << "Random seed = " << seed << endl;

  int m = parmInt (1, 4);  // rows
  int n = parmInt (2, 4);  // columns


  Matrix<double> A (m, n);
  for (int r = 0; r < m; r++)
  {
	for (int c = 0; c < n; c++)
	{
	  A(r,c) = randbad ();
	  //A(r,c) = randfb ();
	}
  }
  //cerr << A << endl << endl;

  /*
  Matrix<double> A (2*m, 2*n);
  A.clear ();
  for (int r = 0; r < min (m, n); r++)
  {
	int r2 = 2 * r;
	//int c = 2 * n - 2;
	int c = 0;

	A(r2,c) = randbad ();
	A(r2,c+1) = randbad ();
	A(r2+1,c) = randbad ();
	A(r2+1,c+1) = randbad ();

	A(r2,r2) = randbad ();
	A(r2,r2+1) = randbad ();
	A(r2+1,r2) = randbad ();
	A(r2+1,r2+1) = randbad ();
  }
  //cerr << A << endl << endl;
  */

  /*
  Matrix<double> A (m, n);
  A.clear ();
  for (int r = 0; r < min (m, n); r++)
  {
	A(r,0) = randbad ();
	A(r,r) = randbad ();
  }
  //cerr << A << endl << endl;
  */


  Matrix<double> B;
  B = ~A * A;
  n = B.columns ();
  int r = n / 2;
  B(r,n-1) = B.column (n-1).frob (2) * 2;
  B(n-1,r) = B(r,n-1);
  B(r,r) = B(r,n-1) / 3;

  Matrix<double> T;
  T.copyFrom (B);

  for (int c = 0; c < n; c++)
  {
	for (int r = c + 1; r < n; r++)
	{
	  B(r,c) = 0;
	}
  }
  cerr << B << endl << endl;
  //cerr << T << endl << endl;

  Matrix<double> Tu;
  Tu.copyFrom (B);

  //SparseLU C = B;
  Matrix<double> C;
  C.copyFrom (B);


  Vector<int> ipiv;
  factorize (C, ipiv);
  cerr << ipiv << endl;
  cerr << C << endl << endl;
  reconstruct (C, ipiv, T);

  /*
  Vector<int> ipiv2 (B.columns ());
  factorize (B, ipiv2);
  cerr << ipiv2 << endl;
  cerr << B << endl << endl;
  reconstruct (B, ipiv2, T);
  */

  Vector<int> ipiv2 (B.columns ());
  int info;
  dsytf2_ ('U', B.columns (), &B(0,0), B.rows (), &ipiv2[0], info);
  cerr << ipiv2 << endl;
  cerr << B << endl << endl;
  reconstruct (B, ipiv2, T);


  cerr << "------------------------------------------------------------" << endl;
  Vector<double> b (C.rows ());
  for (int i = 0; i < b.rows (); i++)
  {
	b[i] = randbad ();
  }
  Vector<double> x;
  solve (C, ipiv, x, b);
  cerr << "b=" << b << endl;
  //cerr << "x=" << x << endl;
  cerr << "Ax=" << T * x << endl;
  //cerr << (T * x - b) << endl;
  cerr << (T * x - b).frob (2) << endl;

  /*
  cerr << "------------------------------------------------------------" << endl;
  solve (B, ipiv, x, b);
  cerr << "b=" << b << endl;
  cerr << "Ax=" << T * x << endl;
  cerr << (T * x - b).frob (2) << endl;
  */

  cerr << "------------------------------------------------------------" << endl;
  x.copyFrom (b);
  dsytrs_ ('U', B.columns (), 1, &B(0,0), B.rows (), &ipiv2[0], &x[0], x.rows (), info);
  //cerr << "x=" << x << endl;
  cerr << "Ax=" << T * x << endl;
  //cerr << (T * x - b) << endl;
  cerr << (T * x - b).frob (2) << endl;



  /*
  //AnnealingAdaptive method;
  //LevenbergMarquardt method;
  LevenbergMarquardtSparse method;
  Test t;
  Vector<double> point (3);
  point[0] = 1;
  point[1] = 1;
  point[2] = 1;
  method.search (t, point);
  cerr << point << endl;
  */
}
