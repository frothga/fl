#ifndef fl_lapack_double_h
#define fl_lapack_double_h


#include "fl/matrix.h"
#include "fl/lapackprotod.h"
#include "fl/complex.h"

#include <algorithm>
#include <float.h>


namespace fl
{
  // Compute eigenvalues and eigenvectors for symmetric matrix
  inline void
  syev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	int n = A.rows ();
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'V';
	char uplo = 'U';

	int lwork = n * n;
	lwork = lwork >? 10;  // Special case for n == 1 and n == 2;
	double * work = new double [lwork];
	int info = 0;

	dsyev_ (jobz,
			uplo,
			n,
			& eigenvectors[0],
			n,
			& eigenvalues[0],
			work,
			lwork,
			info);

	delete work;

	if (info)
	{
	  throw info;
	}
  }

  inline void
  syev (const MatrixPacked<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	int n = A.rows ();

	MatrixPacked<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	eigenvectors.resize (n, n);

	double * work = new double [3 * n];
	int info = 0;

	dspev_ ('V',
			'U',
			n,
			& tempA(0,0),
			& eigenvalues[0],
			& eigenvectors(0,0),
			n,
			work,
			info);

	delete work;

	if (info)
	{
	  throw info;
	}
  }

  // Compute eigenvalues (only) for symmetric matrix.
  inline void
  syev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues)
  {
	int n = A.rows ();

	Matrix<double> eigenvectors;
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'N';
	char uplo = 'U';

	int lwork = n * n;
	lwork = lwork >? 10;  // Special case for n == 1 and n == 2;
	double * work = new double [lwork];
	int info = 0;

	dsyev_ (jobz,
			uplo,
			n,
			& eigenvectors[0],
			n,
			& eigenvalues[0],
			work,
			lwork,
			info);

	delete work;

	if (info)
	{
	  throw info;
	}
  }

  // Compute eigenvalues and eigenvectors for general (non-symmetric) matrix
  inline void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	char jobvl = 'N';
	char jobvr = 'V';

	int lda = A.rows ();
	int n = lda <? A.columns ();
	Matrix<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	double * work = new double [lwork];
	int info = 0;

	dgeev_ (jobvl,
			jobvr,
			n,
			& tempA[0],
			lda,
			& eigenvalues[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			& eigenvectors[0],
			n,
			work,
			lwork,
			info);

	delete work;

	if (info)
	{
	  throw info;
	}
  }

  inline void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues)
  {
	int lda = A.rows ();
	int n = std::min (lda, A.columns ());

	Matrix<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	int lwork = 5 * n;
	double * work = new double [lwork];
	int info = 0;

	dgeev_ ('N',
			'N',
			n,
			& tempA[0],
			lda,
			& eigenvalues[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			0,
			1,  // ldvr >= 1.  ditto
			work,
			lwork,
			info);

	delete work;

	if (info)
	{
	  throw info;
	}
  }

  inline void
  geev (const MatrixAbstract<double> & A, Matrix<complex double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	char jobvl = 'N';
	char jobvr = 'V';

	int lda = A.rows ();
	int n = lda <? A.columns ();
	Matrix<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<double> wr (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	double * work = new double [lwork];
	int info = 0;

	dgeev_ (jobvl,
			jobvr,
			n,
			& tempA[0],
			lda,
			& wr[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			& eigenvectors[0],
			n,
			work,
			lwork,
			info);

	delete work;

	if (info)
	{
	  throw info;
	}

	for (int i = 0; i < n; i++)
	{
	  eigenvalues[i] = wr[i] + wi[i] * I;
	}
  }

  // Solve least squares problem using SVD via QR
  inline int
  gelss (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & b, const double rcond, Matrix<double> & s)
  {
	int m = A.rows ();
	int n = A.columns ();
	int ldb = std::max (m, n);
	int nrhs = b.columns ();
	int bm = std::max (b.rows (), ldb);

	Matrix<double> tempA;
	tempA.copyFrom (A);

	x.resize (ldb, nrhs);
	for (int c = 0; c < nrhs; c++)
	{
	  for (int r = 0; r < bm; r++)
	  {
		x(r,c) = b(r,c);
	  }
	}

    s.resize (std::min (m, n), 1);

	int rank;
    int ldwork = 5 * (ldb >? nrhs);
    double * work = new double[ldwork];
	int info = 0;

    dgelss_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 m,
			 & x[0],
			 ldb,
			 & s[0],
			 rcond,
			 rank,
			 work,
			 ldwork,
			 info);

    delete work;

	if (info)
	{
	  throw info;
	}

	return rank;
  }

  inline int
  gelss (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & b, const double rcond = -1)
  {
	Matrix<double> s;
	return gelss (A, x, b, rcond, s);
  }

  inline void
  gesvd (const MatrixAbstract<double> & A, Matrix<double> & U, Matrix<double> & S, Matrix<double> & VT, char jobu = 'A', char jobvt = 'A')
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = std::min (m, n);

	Matrix<double> tempA;
	tempA.copyFrom (A);

	S.resize (minmn);

	switch (jobu)
	{
	  case 'A':
		U.resize (m, m);
		break;
	  case 'N':
		if (U.columns () < 1)
		{
		  U.resize (1, 1);
		}
		break;
	  default:
		jobu = 'S';
		U.resize (m, minmn);
	}

	switch (jobvt)
	{
	  case 'A':
		VT.resize (n, n);
		break;
	  case 'N':
		if (VT.columns () < 1)
		{
		  VT.resize (1, 1);
		}
		break;
	  default:
		jobvt = 'S';
		U.resize (minmn, n);
	}

    int lwork = -1;
	double optimalSize;
	int info = 0;

	// Do space query first
	dgesvd_ (jobu,
			 jobvt,
			 m,
			 n,
			 & tempA[0],
			 m,
			 & S[0],
			 & U[0],
			 U.rows (),
			 & VT[0],
			 VT.rows (),
			 &optimalSize,
			 lwork,
			 info);

	lwork = (int) optimalSize;
    double * work = new double[lwork];

	// Now for the real thing
	dgesvd_ (jobu,
			 jobvt,
			 m,
			 n,
			 & tempA[0],
			 m,
			 & S[0],
			 & U[0],
			 U.rows (),
			 & VT[0],
			 VT.rows (),
			 work,
			 lwork,
			 info);

    delete work;

	if (info)
	{
	  throw info;
	}
  }


  // General non LAPACK operations the depend on LAPACK -----------------------

  // Returns the pseudoinverse of any matrix A
  inline Matrix<double>
  pinv (const Matrix<double> & A, double tolerance = -1, double epsilon = DBL_EPSILON)
  {
	Matrix<double> U;
	Vector<double> D;
	Matrix<double> VT;
	gesvd (A, U, D, VT);

	if (tolerance < 0)
	{
	  tolerance = std::max (A.rows (), A.columns ()) * D[0] * epsilon;
	}

	for (int i = 0; i < D.rows (); i++)
	{
	  if (D[i] > tolerance)
	  {
		D[i] = 1.0 / D[i];
	  }
	  else
	  {
		D[i] = 0;
	  }
	}
	MatrixDiagonal<double> DD = D;

	return ~VT * DD * ~U;
  }

  // Returns the inverse of a non-singular square matrix A
  inline Matrix<double>
  operator ! (const Matrix<double> & A)
  {
	int m = A.rows ();
	int n = A.columns ();
	Matrix<double> tempA;
	tempA.copyFrom (A);

	int * ipiv = new int[std::min (m, n)];

	int info = 0;

	dgetrf_ (m,
			 n,
			 & tempA[0],
			 m,
			 ipiv,
			 info);

	if (info == 0)
	{
	  int lwork = -1;
	  double optimalSize;

	  dgetri_ (n,
			   & tempA[0],
			   m,
			   ipiv,
			   & optimalSize,
			   lwork,
			   info);

	  lwork = (int) optimalSize;
	  double * work = new double[lwork];

	  dgetri_ (n,
			   & tempA[0],
			   m,
			   ipiv,
			   work,
			   lwork,
			   info);

	  delete work;
	}

	delete ipiv;

	if (info != 0)
	{
	  throw info;
	}

	return tempA;
  }

  inline double
  det (const Matrix<double> & A)
  {
	int m = A.rows ();
	if (m != A.columns ())
	{
	  throw "det only works on square matrices";
	}

	Matrix<double> tempA;
	tempA.copyFrom (A);

	int * ipiv = new int[m];

	int info = 0;

	dgetrf_ (m,
			 m,
			 & tempA[0],
			 m,
			 ipiv,
			 info);

	int exchanges = 0;
	double result = 1;
	for (int i = 0; i < m; i++)
	{
	  result *= tempA(i,i);
	  if (ipiv[i] != i + 1)
	  {
		exchanges++;
	  }
	}
	if (exchanges % 2)
	{
	  result *= -1;
	}

	delete ipiv;

	if (info != 0)
	{
	  throw info;
	}

	return result;
  }

  inline int
  rank (const Matrix<double> & A, double threshold = -1, double eps = DBL_EPSILON)
  {
	Matrix<double> U;
	Matrix<double> S;
	Matrix<double> VT;
	gesvd (A, U, S, VT);

	if (threshold < 0)
	{
	  threshold = std::max (A.rows (), A.columns ()) * S[0] * eps;
	}

	int result = 0;
	while (result < S.rows ()  &&  S[result] > threshold)
	{
	  result++;
	}

	return result;
  }
}


#endif
