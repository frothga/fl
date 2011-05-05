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


#include "fl/lapack.h"
#include "fl/lapackproto.h"

#include <limits>


namespace fl
{
  template<class T>
  void
  syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA)
  {
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  eigenvectors = (const Matrix<T> &) A;
	}
	else
	{
	  eigenvectors.copyFrom (A);
	}

	int n = eigenvectors.rows ();
	eigenvalues.resize (n);

	char jobz = 'V';
	char uplo = 'U';

	int lwork = n * n;
	lwork = std::max (lwork, 10);  // Special case for n == 1 and n == 2;
	T * work = (T *) malloc (lwork * sizeof (T));
	int info = 0;

	syev (jobz,
		  uplo,
		  n,
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  & eigenvalues[0],
		  work,
		  lwork,
		  info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<class T>
  void
  syev (const MatrixPacked<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA)
  {
	int n = A.rows ();

	MatrixPacked<T> tempA;
	if (destroyA)
	{
	  tempA = A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	eigenvectors.resize (n, n);

	T * work = (T *) malloc (3 * n * sizeof (T));
	int info = 0;

	spev ('V',
		  'U',
		  n,
		  & tempA(0,0),
		  & eigenvalues[0],
		  & eigenvectors(0,0),
		  eigenvectors.strideC,
		  work,
		  info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<class T>
  void
  syev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, bool destroyA)
  {
	Matrix<T> eigenvectors;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  eigenvectors = (const Matrix<T> &) A;
	}
	else
	{
	  eigenvectors.copyFrom (A);
	}

	int n = A.rows ();
	eigenvalues.resize (n);

	char jobz = 'N';
	char uplo = 'U';

	int lwork = n * n;
	lwork = std::max (lwork, 10);  // Special case for n == 1 and n == 2;
	T * work = (T *) malloc (lwork * sizeof (T));
	int info = 0;

	syev (jobz,
		  uplo,
		  n,
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  & eigenvalues[0],
		  work,
		  lwork,
		  info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<class T>
  void
  sygv (const MatrixAbstract<T> & A, const MatrixAbstract<T> & B, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA, bool destroyB)
  {
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  eigenvectors = (const Matrix<T> &) A;
	}
	else
	{
	  eigenvectors.copyFrom (A);
	}

	Matrix<T> tempB;
	if (destroyB  &&  (B.classID () & MatrixID))
	{
	  tempB = (const Matrix<T> &) B;
	}
	else
	{
	  tempB.copyFrom (B);
	}

	int n = eigenvectors.rows ();  // or eigenvectors.columns (); they should be equal
	eigenvalues.resize (n);

	int lwork = -1;
	T optimalSize = 0;
	int info = 0;

	// Space query
	sygv (1,
		  'V',
		  'U',
		  n,
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  & tempB[0],
		  tempB.strideC,
		  & eigenvalues[0],
		  & optimalSize,
		  lwork,
		  info);

	if (info) throw info;
	lwork = (int) optimalSize;
	T * work = (T *) malloc (lwork * sizeof (T));

	// Actual computation, using optimal work space.
	sygv (1,
		  'V',
		  'U',
		  n,
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  & tempB[0],
		  tempB.strideC,
		  & eigenvalues[0],
		  work,
		  lwork,
		  info);

	free (work);

	if (info) throw info;
  }

  template<class T>
  void
  geev (const MatrixAbstract<T> & A, Matrix<std::complex<T> > & eigenvalues, Matrix<T> & eigenvectors, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<T> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<T> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<T> wr (n);
	Matrix<T> wi (n);

	eigenvectors.resize (n, n);

	int lwork = -1;
	T optimalLwork;
	int info = 0;

	geev ('N',
		  'V',
		  n,
		  & tempA[0],
		  tempA.strideC,
		  & wr[0],
		  & wi[0],
		  0,
		  1,  // ldvl >= 1.  A lie to make lapack happy.
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  &optimalLwork,
		  lwork,
		  info);

	if (info) throw info;
	lwork = (int) optimalLwork;
	T * work = (T *) malloc (lwork * sizeof (T));

	geev ('N',
		  'V',
		  n,
		  & tempA[0],
		  tempA.strideC,
		  & wr[0],
		  & wi[0],
		  0,
		  1,
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  work,
		  lwork,
		  info);

	free (work);

	if (info) throw info;

	for (int i = 0; i < n; i++)
	{
	  eigenvalues[i] = std::complex<T> (wr[i], wi[i]);
	}
  }

  template<class T>
  void
  geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, Matrix<T> & eigenvectors, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<T> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<T> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<T> wi (n);

	eigenvectors.resize (n, n);

	int lwork = -1;
	T optimalLwork;
	int info = 0;

	geev ('N',
		  'V',
		  n,
		  & tempA[0],
		  tempA.strideC,
		  & eigenvalues[0],
		  & wi[0],
		  0,
		  1,  // ldvl >= 1.  A lie to make lapack happy.
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  &optimalLwork,
		  lwork,
		  info);

	if (info) throw info;
	lwork = (int) optimalLwork;
	T * work = (T *) malloc (lwork * sizeof (T));

	geev ('N',
		  'V',
		  n,
		  & tempA[0],
		  tempA.strideC,
		  & eigenvalues[0],
		  & wi[0],
		  0,
		  1,
		  & eigenvectors[0],
		  eigenvectors.strideC,
		  work,
		  lwork,
		  info);

	free (work);

	if (info) throw info;
  }

  template<class T>
  void
  geev (const MatrixAbstract<T> & A, Matrix<T> & eigenvalues, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<T> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<T> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<T> wi (n);

	int lwork = -1;
	T optimalLwork;
	int info = 0;

	geev ('N',
		  'N',
		  n,
		  & tempA[0],
		  tempA.strideC,
		  & eigenvalues[0],
		  & wi[0],
		  0,
		  1,  // ldvl >= 1.  A lie to make lapack happy.
		  0,
		  1,  // ditto
		  &optimalLwork,
		  lwork,
		  info);

	if (info) throw info;
	lwork = (int) optimalLwork;
	T * work = (T *) malloc (lwork * sizeof (T));

	geev ('N',
		  'N',
		  n,
		  & tempA[0],
		  tempA.strideC,
		  & eigenvalues[0],
		  & wi[0],
		  0,
		  1,
		  0,
		  1,
		  work,
		  lwork,
		  info);

	free (work);

	if (info) throw info;
  }

  template<class T>
  void
  gelss (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & B, T * residual, bool destroyA, bool destroyB)
  {
	const int m    = std::min (A.rows (), B.rows ());
	const int n    = A.columns ();
	const int nrhs = B.columns ();
	const int mn   = std::max (m, n);  // the minimum allowable leading dimension (stride) of B

	Matrix<T> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<T> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	const Matrix<T> * p = 0;
	if (B.classID () & MatrixID) p = (const Matrix<T> *) &B;
	if (destroyB  &&  p  &&  p->strideC >= mn)
	{
	  x = *p;
	}
	else  // must copy the elements of B into X
	{
	  x.resize (mn, nrhs);
	  T * xp = & x(0,0);
	  const int xstep = x.strideC - m;
	  if (p)
	  {
		T * bp = & B(0,0);
		T * end = bp + p->strideC * nrhs;
		const int bstep = p->strideC - m;
		while (bp < end)
		{
		  T * columnEnd = bp + m;
		  while (bp < columnEnd)
		  {
			*xp++ = *bp++;
		  }
		  xp += xstep;
		  bp += bstep;
		}
	  }
	  else
	  {
		for (int c = 0; c < nrhs; c++)
		{
		  for (int r = 0; r < m; r++)
		  {
			*xp++ = B(r,c);
		  }
		  xp += xstep;
		}
	  }
	}

	Vector<T> s (std::min (m, n));

	int rank;
	T optimalSize;
	int lwork = -1;
	int info = 0;

	gelss (m,
		   n,
		   nrhs,
		   & tempA[0],
		   tempA.strideC,
		   & x[0],
		   x.strideC,
		   & s[0],
		   -1.0,  // use machine precision
		   rank,
		   & optimalSize,
		   lwork,
		   info);

	if (info) throw info;
	lwork = (int) optimalSize;
	T * work = (T *) malloc (lwork * sizeof (T));

	gelss (m,
		   n,
		   nrhs,
		   & tempA[0],
		   tempA.strideC,
		   & x[0],
		   x.strideC,
		   & s[0],
		   -1.0,  // use machine precision
		   rank,
		   work,
		   lwork,
		   info);

	free (work);
	if (info) throw info;

	x.rows_ = n;
	if (residual)
	{
	  register T total = 0;
	  const int rows = m - n;
	  if (rows > 0)
	  {
		T * xp  = (T *) x.data;
		T * end = xp + x.strideC * nrhs;
		const int step = x.strideC - rows;
		xp += n;
		while (xp < end)
		{
		  T * columnEnd = xp + rows;
		  while (xp < columnEnd) total += *xp * *xp++;
		  xp += step;
		}
	  }
	  *residual = total;
	}
  }

  template<class T>
  void
  gelsd (const MatrixAbstract<T> & A, Matrix<T> & x, const MatrixAbstract<T> & B, T * residual, bool destroyA, bool destroyB)
  {
	const int m    = std::min (A.rows (), B.rows ());
	const int n    = A.columns ();
	const int nrhs = B.columns ();
	const int mn   = std::max (m, n);  // the minimum allowable leading dimension (stride) of B

	Matrix<T> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<T> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	const Matrix<T> * p = 0;
	if (B.classID () & MatrixID) p = (const Matrix<T> *) &B;
	if (destroyB  &&  p  &&  p->strideC >= mn)
	{
	  x = *p;
	}
	else  // must copy the elements of B into X
	{
	  x.resize (mn, nrhs);
	  T * xp = & x(0,0);
	  const int xstep = x.strideC - m;
	  if (p)
	  {
		T * bp = & B(0,0);
		T * end = bp + p->strideC * nrhs;
		const int bstep = p->strideC - m;
		while (bp < end)
		{
		  T * columnEnd = bp + m;
		  while (bp < columnEnd)
		  {
			*xp++ = *bp++;
		  }
		  xp += xstep;
		  bp += bstep;
		}
	  }
	  else
	  {
		for (int c = 0; c < nrhs; c++)
		{
		  for (int r = 0; r < m; r++)
		  {
			*xp++ = B(r,c);
		  }
		  xp += xstep;
		}
	  }
	}

	Vector<T> s (std::min (m, n));

	int rank;
    int lwork = -1;  // space query
	T optimalLwork;
	int   optimalLiwork;
	int info = 0;

	// Do space query first
	gelsd (m,
		   n,
		   nrhs,
		   & tempA[0],
		   tempA.strideC,
		   & x[0],
		   x.strideC,
		   & s[0],
		   -1.0,  // use machine precision
		   rank,
		   &optimalLwork,
		   lwork,
		   &optimalLiwork,
		   info);

	if (info) throw info;
	lwork      = (int) optimalLwork;
	int liwork =       optimalLiwork;
    T * work  = (T *) malloc (lwork  * sizeof (T));
	int *   iwork = (int *)   malloc (liwork * sizeof (int));

	// And then the actual computation
	gelsd (m,
		   n,
		   nrhs,
		   & tempA[0],
		   tempA.strideC,
		   & x[0],
		   x.strideC,
		   & s[0],
		   -1.0,  // use machine precision
		   rank,
		   work,
		   lwork,
		   iwork,
		   info);

	free (work);
	free (iwork);

	if (info) throw info;

	x.rows_ = n;
	if (residual)
	{
	  register T total = 0;
	  const int rows = m - n;
	  if (rows > 0)
	  {
		T * xp  = (T *) x.data;
		T * end = xp + x.strideC * nrhs;
		const int step = x.strideC - rows;
		xp += n;
		while (xp < end)
		{
		  T * columnEnd = xp + rows;
		  while (xp < columnEnd) total += *xp * *xp++;
		  xp += step;
		}
	  }
	  *residual = total;
	}
  }

  template<class T>
  void
  gesvd (const MatrixAbstract<T> & A, Matrix<T> & U, Matrix<T> & S, Matrix<T> & VT, char jobu, char jobvt, bool destroyA)
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = std::min (m, n);

	Matrix<T> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<double> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

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
		VT.resize (minmn, n);
	}

    int lwork = -1;
	T optimalSize;
	int info = 0;

	// Do space query first
	gesvd (jobu,
		   jobvt,
		   m,
		   n,
		   & tempA[0],
		   tempA.strideC,
		   & S[0],
		   & U[0],
		   U.strideC,
		   & VT[0],
		   VT.strideC,
		   &optimalSize,
		   lwork,
		   info);

	if (info) throw info;
	lwork = (int) optimalSize;
    T * work = (T *) malloc (lwork * sizeof (T));

	// Now for the real thing
	gesvd (jobu,
		   jobvt,
		   m,
		   n,
		   & tempA[0],
		   tempA.strideC,
		   & S[0],
		   & U[0],
		   U.strideC,
		   & VT[0],
		   VT.strideC,
		   work,
		   lwork,
		   info);

    free (work);

	if (info) throw info;
  }

  template<class T>
  Matrix<T>
  pinv (const MatrixAbstract<T> & A, T tolerance, T epsilon)
  {
	Matrix<T> U;
	Vector<T> D;
	Matrix<T> VT;
	gesvd (A, U, D, VT);

	if (tolerance < 0)
	{
	  if (epsilon < 0)
	  {
		epsilon = std::numeric_limits<T>::epsilon ();
	  }
	  tolerance = std::max (A.rows (), A.columns ()) * D[0] * epsilon;
	}

	for (int i = 0; i < D.rows (); i++)
	{
	  if (D[i] > tolerance)
	  {
		D[i] = 1.0f / D[i];
	  }
	  else
	  {
		D[i] = 0;
	  }
	}
	MatrixDiagonal<T> DD = D;

	return ~VT * DD * ~U;
  }

  template<class T>
  int
  rank (const MatrixAbstract<T> & A, T threshold, T epsilon)
  {
	Matrix<T> U;
	Matrix<T> S;
	Matrix<T> VT;
	gesvd (A, U, S, VT, 'N', 'N');

	if (threshold < 0)
	{
	  if (epsilon < 0)
	  {
		epsilon = std::numeric_limits<T>::epsilon ();
	  }
	  threshold = std::max (A.rows (), A.columns ()) * S[0] * epsilon;
	}

	int result = 0;
	while (result < S.rows ()  &&  S[result] > threshold)
	{
	  result++;
	}

	return result;
  }

  template<class T>
  Matrix<T>
  MatrixAbstract<T>::operator ! () const
  {
	int h = rows ();
	int w = columns ();
	if (w != h) return pinv (*this);  // forces dgesvd to be linked as well

	Matrix<T> result;
	result.copyFrom (*this);

	int * ipiv = (int *) malloc (h * sizeof (int));

	int info = 0;

	getrf (h,
		   h,
		   & result[0],
		   result.strideC,
		   ipiv,
		   info);

	if (info == 0)
	{
	  int lwork = -1;
	  T optimalSize;

	  getri (h,
			 & result[0],
			 result.strideC,
			 ipiv,
			 & optimalSize,
			 lwork,
			 info);

	  lwork = (int) optimalSize;
	  T * work = (T *) malloc (lwork * sizeof (T));

	  getri (h,
			 & result[0],
			 result.strideC,
			 ipiv,
			 work,
			 lwork,
			 info);

	  free (work);
	}

	free (ipiv);

	if (info != 0)
	{
	  throw info;
	}

	return result;
  }

  template<class T>
  T
  det (const MatrixAbstract<T> & A)
  {
	int m = A.rows ();
	if (m != A.columns ())
	{
	  throw "det only works on square matrices";
	}

	Matrix<T> tempA;
	tempA.copyFrom (A);

	int * ipiv = (int *) malloc (m * sizeof (int));

	int info = 0;

	getrf (m,
		   m,
		   & tempA[0],
		   tempA.strideC,
		   ipiv,
		   info);

	int exchanges = 0;
	T result = 1;
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

	free (ipiv);

	if (info != 0)
	{
	  throw info;
	}

	return result;
  }
}
