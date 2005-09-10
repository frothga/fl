/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapackd.h into separate file.
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"


namespace fl
{
  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	char jobvl = 'N';
	char jobvr = 'V';

	int lda = A.rows ();
	int n = std::min (lda, A.columns ());
	Matrix<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	double * work = (double *) malloc (lwork * sizeof (double));
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

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues)
  {
	int lda = A.rows ();
	int n = std::min (lda, A.columns ());

	Matrix<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	int lwork = 5 * n;
	double * work = (double *) malloc (lwork * sizeof (double));
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

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<std::complex<double> > & eigenvalues, Matrix<double> & eigenvectors)
  {
	char jobvl = 'N';
	char jobvr = 'V';

	int lda = A.rows ();
	int n = std::min (lda, A.columns ());
	Matrix<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<double> wr (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	double * work = (double *) malloc (lwork * sizeof (double));
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

	free (work);

	if (info)
	{
	  throw info;
	}

	for (int i = 0; i < n; i++)
	{
	  eigenvalues[i] = std::complex<double> (wr[i], wi[i]);
	}
  }
}
