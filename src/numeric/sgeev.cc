/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapacks.h into separate file.
*/


#include "fl/lapack.h"
#include "fl/lapackprotos.h"


namespace fl
{
  template<>
  void
  geev (const MatrixAbstract<float> & A, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors)
  {
	char jobvl = 'N';
	char jobvr = 'V';

	int lda = A.rows ();
	int n = std::min (lda, A.columns ());
	Matrix<float> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<float> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	float * work = (float *) malloc (lwork * sizeof (float));
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

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  void
  geev (const MatrixAbstract<float> & A, Matrix<float> & eigenvalues)
  {
	int lda = A.rows ();
	int n = std::min (lda, A.columns ());

	Matrix<float> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<float> wi (n);

	int lwork = 5 * n;
	float * work = (float *) malloc (lwork * sizeof (float));
	int info = 0;

	sgeev_ ('N',
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
  geev (const MatrixAbstract<float> & A, Matrix<std::complex<float> > & eigenvalues, Matrix<float> & eigenvectors)
  {
	char jobvl = 'N';
	char jobvr = 'V';

	int lda = A.rows ();
	int n = std::min (lda, A.columns ());
	Matrix<float> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	Matrix<float> wr (n);
	Matrix<float> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	float * work = (float *) malloc (lwork * sizeof (float));
	int info = 0;

	sgeev_ (jobvl,
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
	  eigenvalues[i] = std::complex<float> (wr[i], wi[i]);
	}
  }
}
