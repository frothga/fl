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
  syev (const MatrixAbstract<float> & A, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors)
  {
	int n = A.rows ();
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'V';
	char uplo = 'U';

	int lwork = n * n;
	lwork = std::max (lwork, 10);  // Special case for n == 1 and n == 2;
	float * work = (float *) malloc (lwork * sizeof (float));
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

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  void
  syev (const MatrixAbstract<float> & A, Matrix<float> & eigenvalues)
  {
	int n = A.rows ();

	Matrix<float> eigenvectors;
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'N';
	char uplo = 'U';

	int lwork = n * n;
	lwork = std::max (lwork, 10);  // Special case for n == 1 and n == 2;
	float * work = (float *) malloc (lwork * sizeof (float));
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

	free (work);

	if (info)
	{
	  throw info;
	}
  }
}
