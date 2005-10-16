/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapackd.h into separate file.  Allow
        overwriting of input.  Compute residual.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"

#include <assert.h>


namespace fl
{
  template<>
  void
  gelss (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & b, double * residual)
  {
	int m = A.rows ();
	int n = A.columns ();
	int nrhs = b.columns ();
	int ldx = std::max (m, n);
	assert (b.rows () == m);

	Matrix<double> tempA;
	const Matrix<double> * p;
	if (p = dynamic_cast<const Matrix<double> *> (&A))
	{
	  tempA = *p;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	if (ldx == m  &&  (p = dynamic_cast<const Matrix<double> *> (&b)))
	{
	  x = *p;
	}
	else
	{
	  x.resize (ldx, nrhs);
	  for (int c = 0; c < nrhs; c++)
	  {
		for (int r = 0; r < m; r++)
		{
		  x(r,c) = b(r,c);
		}
	  }
	}

	Matrix<double> s (std::min (m, n), 1);

	int rank;
	int ldwork = 5 * std::max (ldx, nrhs);
	double * work = (double *) malloc (ldwork * sizeof (double));
	int info = 0;

	dgelss_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 m,
			 & x[0],
			 ldx,
			 & s[0],
			 -1.0,  // use machine precision
			 rank,
			 work,
			 ldwork,
			 info);

	free (work);

	if (info)
	{
	  throw info;
	}

	if (ldx > n)
	{
	  Matrix<double> tempX (n, nrhs);
	  for (int c = 0; c < nrhs; c++)
	  {
		for (int r = 0; r < n; r++)
		{
		  tempX(r,c) = x(r,c);
		}
	  }

	  if (residual)
	  {
		double total = 0;
		for (int c = 0; c < nrhs; c++)
		{
		  for (int r = n; r < ldx; r++)
		  {
			double e = x(r,c);
			total += e * e;
		  }
		}
		*residual = total;
	  }

	  x = tempX;
	}
	else
	{
	  if (residual)
	  {
		*residual = 0;
	  }
	}
  }
}
