#ifndef fl_lapack_single_h
#define fl_lapack_single_h


#include "fl/matrix.h"
#include "fl/lapackprotos.h"

#include <algorithm>
#include <complex>
#include <float.h>


namespace fl
{
  // Compute eigenvalues and eigenvectors for symmetric matrix
  inline void
  syev (const Matrix<float> & A, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors)
  {
	int n = A.rows ();
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'V';
	char uplo = 'U';

	int lwork = n * n;
	lwork = lwork >? 10;  // Special case for n == 1 and n == 2;
	float * work = new float [lwork];
	int info = 0;

	ssyev_ (jobz,
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

  // Compute eigenvalues (only) for symmetric matrix.
  inline void
  syev (const Matrix<float> & A, Matrix<float> & eigenvalues)
  {
	int n = A.rows ();

	Matrix<float> eigenvectors;
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'N';
	char uplo = 'U';

	int lwork = n * n;
	lwork = lwork >? 10;  // Special case for n == 1 and n == 2;
	float * work = new float [lwork];
	int info = 0;

	ssyev_ (jobz,
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
  // eigenvalues should really be Matrix< complex<float> >.  Fix this some time.
  inline void
  geev (const Matrix<float> & A, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors)
  {
	char jobvl = 'N';
	char jobvr = 'V';

	int lda = A.rows ();
	int n = lda <? A.columns ();
	Matrix<float> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<float> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	float * work = new float [lwork];
	int info = 0;

	sgeev_ (jobvl,
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

  // Solve least squares problem using SVD via QR
  inline void
  gelss (Matrix<float> & A, Matrix<float> & x, const Matrix<float> & b)
  {
	int m = A.rows ();
	int n = A.columns ();
	int ldb = m >? n;
	int bn = b.columns ();

	Matrix<float> tempA;
	tempA.copyFrom (A);

	x.resize (ldb, bn);
	x.copyFrom (b);
	x.rows_ = ldb;  // Size of answer x, not necessarily size of b.

	float rcond = -1;
	int rank;
    int ldwork = 5 * (ldb >? bn);
    float * work = new float[ldwork];
    float * s = new float[m <? n];
	int info = 0;

    sgelss_ (m,
			 n,
			 bn,
			 & tempA[0],
			 m,
			 & x[0],
			 ldb,
			 s,
			 rcond,
			 rank,
			 work,
			 ldwork,
			 info);

    delete work;
    delete s;

	if (info)
	{
	  throw info;
	}
  }

  inline void
  gesvd (const Matrix<float> & A, Matrix<float> & U, Matrix<float> & S, Matrix<float> & VT)
  {
	char jobu = 'A';
	char jobvt = 'A';
	int m = A.rows ();
	int n = A.columns ();

	Matrix<float> tempA;
	tempA.copyFrom (A);

	U.resize (m, m);
	S.resize (m <? n);
	VT.resize (n, n);

    int lwork = -1;
	float optimalSize;
	int info = 0;

	// Do space query first
	sgesvd_ (jobu,
			 jobvt,
			 m,
			 n,
			 & tempA[0],
			 m,
			 & S[0],
			 & U[0],
			 m,
			 & VT[0],
			 n,
			 &optimalSize,
			 lwork,
			 info);

	lwork = (int) optimalSize;
    float * work = new float[lwork];

	sgesvd_ (jobu,
			 jobvt,
			 m,
			 n,
			 & tempA[0],
			 m,
			 & S[0],
			 & U[0],
			 m,
			 & VT[0],
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


  // General non LAPACK operations the depend on LAPACK -----------------------

  // Returns the pseudoinverse of any matrix A
  inline Matrix<float>
  pinv (const Matrix<float> & A, float tolerance = -1, float epsilon = FLT_EPSILON)
  {
	Matrix<float> U;
	Vector<float> D;
	Matrix<float> VT;
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
	MatrixDiagonal<float> DD = D;

	return ~VT * DD * ~U;
  }

  // Returns the inverse of a non-singular square matrix A
  inline Matrix<float>
  operator ! (const Matrix<float> & A)
  {
	int m = A.rows ();
	int n = A.columns ();
	Matrix<float> tempA;
	tempA.copyFrom (A);

	int * ipiv = new int[std::min (m, n)];

	int info = 0;

	sgetrf_ (m,
			 n,
			 & tempA[0],
			 m,
			 ipiv,
			 info);

	if (info == 0)
	{
	  int lwork = -1;
	  float optimalSize;

	  sgetri_ (n,
			   & tempA[0],
			   m,
			   ipiv,
			   & optimalSize,
			   lwork,
			   info);

	  lwork = (int) optimalSize;
	  float * work = new float[lwork];

	  sgetri_ (n,
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

  inline float
  det (const Matrix<float> & A)
  {
	int m = A.rows ();
	if (m != A.columns ())
	{
	  throw "det only works on square matrices";
	}

	Matrix<float> tempA;
	tempA.copyFrom (A);

	int * ipiv = new int[m];

	int info = 0;

	sgetrf_ (m,
			 m,
			 & tempA[0],
			 m,
			 ipiv,
			 info);

	int exchanges = 0;
	float result = 1;
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
  rank (Matrix<float> & A, float threshold = -1, float eps = FLT_EPSILON)
  {
	Matrix<float> U;
	Matrix<float> S;
	Matrix<float> VT;
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
