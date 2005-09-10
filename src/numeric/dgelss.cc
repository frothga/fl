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
  int
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
	int ldwork = 5 * std::max (ldb, nrhs);
    double * work = (double *) malloc (ldwork * sizeof (double));
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

	free (work);

	if (info)
	{
	  throw info;
	}

	return rank;
  }
}
