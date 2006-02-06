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


01/2006 Fred Rothganger -- Add "copy" option.
02/2006 Fred Rothganger -- Use pointer pushing for more efficient copies.
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"

#include <assert.h>


namespace fl
{
  template<>
  void
  gelss (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & b, double * residual, bool copyA, bool copyb)
  {
	int m = A.rows ();
	int n = A.columns ();
	int nrhs = b.columns ();
	int ldx = std::max (m, n);
	assert (b.rows () == m);

	Matrix<double> tempA;
	const Matrix<double> * p;
	if (! copyA  &&  (p = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  tempA = *p;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	if (! copyb  &&  ldx == m  &&  (p = dynamic_cast<const Matrix<double> *> (&b)))
	{
	  x = *p;
	}
	else
	{
	  x.resize (ldx, nrhs);
	  double * xp = & x(0,0);
	  double * bp = & b(0,0);
	  double * end = bp + m * nrhs;
	  int step = ldx - m;
	  while (bp < end)
	  {
		double * rowEnd = bp + m;
		while (bp < rowEnd)
		{
		  *xp++ = *bp++;
		}
		xp += step;
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
	  double * xp = & x(0,0);
	  double * tp = & tempX(0,0);
	  double * end = tp + n * nrhs;
	  int step = ldx - n;
	  while (tp < end)
	  {
		double * rowEnd = tp + n;
		while (tp < rowEnd)
		{
		  *tp++ = *xp++;
		}
		xp += step;
	  }

	  if (residual)
	  {
		double total = 0;
		double * xp = & x(0,0);
		double * end = xp + ldx * nrhs;
		while (xp < end)
		{
		  double * rowEnd = xp + ldx;
		  xp += n;
		  while (xp < rowEnd)
		  {
			total += *xp * *xp++;
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
