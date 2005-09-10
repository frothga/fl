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
  syev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	int n = A.rows ();
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'V';
	char uplo = 'U';

	int lwork = n * n;
	lwork = std::max (lwork, 10);  // Special case for n == 1 and n == 2;
	double * work = (double *) malloc (lwork * sizeof (double));
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

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  void
  syev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues)
  {
	int n = A.rows ();

	Matrix<double> eigenvectors;
	eigenvectors.copyFrom (A);
	eigenvalues.resize (n);

	char jobz = 'N';
	char uplo = 'U';

	int lwork = n * n;
	lwork = std::max (lwork, 10);  // Special case for n == 1 and n == 2;
	double * work = (double *) malloc (lwork * sizeof (double));
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

	free (work);

	if (info)
	{
	  throw info;
	}
  }
}
