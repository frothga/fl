/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapackd.h into separate file.  Allow
        overwriting of input.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"


namespace fl
{
  template<>
  void
  syev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors, bool copy)
  {
	const Matrix<double> * pA;
	if (! copy  &&  (pA = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  eigenvectors = *pA;
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
  syev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, bool copy)
  {
	Matrix<double> eigenvectors;
	const Matrix<double> * pA;
	if (! copy  &&  (pA = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  eigenvectors = *pA;
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
