/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.1 and 1.2  Copyright 2005 Sandia Corporation.
Revisions 1.4 thru 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/25 14:34:42  Fred
This code was assuming that it always received dense matrices, and so was
copying it using a direct pointer to memory.  Fixed this latent bug by using
the element accessor when the matrix is not dense.

Revision 1.8  2007/03/23 10:57:28  Fred
Use CVS Log to generate revision history.

Revision 1.7  2006/02/16 04:45:26  Fred
Change all "copy" options into "destroy".  These functions now have about the
same semantics as before the copy option was added, except now the programmer
can explicitly indicate that a parameter can be overwritten safely.

Revision 1.6  2006/02/05 22:52:00  Fred
Add a flag to control copying the "b" parameter, since there are cases where
the client program may want to reuse it.

Use pointer pushing to make copying more efficient.  We could cut down on the
amount of copying if we stored a leading dimension (ala LAPACK) in the Matrix
classes.

Revision 1.5  2006/01/29 03:25:29  Fred
Add copy option.

Revision 1.4  2005/10/15 22:23:15  Fred
Include assert.h.  It as only a fluke that this worked before.

Revision 1.3  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/10/13 03:57:16  Fred
Allow overwriting of input matrix.  Fix bug introduced by compilability fixes
for MSVC.  Optionally compute residual.  Changed interface to get rid of SVD
related return values.

Add Sandia copyright notice.

Revision 1.1  2005/09/10 17:40:49  Fred
Create templates for LAPACK bridge, rather than using type specific inlines. 
Break lapackd.h and lapacks.h into implementation files.
-------------------------------------------------------------------------------
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"

#include <assert.h>


namespace fl
{
  template<>
  void
  gelss (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & B, double * residual, bool destroyA, bool destroyB)
  {
	int m = A.rows ();
	int n = A.columns ();
	int nrhs = B.columns ();
	int ldx = std::max (m, n);
	assert (B.rows () == m);

	Matrix<double> tempA;
	const Matrix<double> * p;
	if (destroyA  &&  (p = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  tempA = *p;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	p = dynamic_cast<const Matrix<double> *> (&B);
	if (destroyB  &&  ldx == m  &&  p)
	{
	  x = *p;
	}
	else
	{
	  x.resize (ldx, nrhs);
	  double * xp = & x(0,0);
	  int step = ldx - m;
	  if (p)
	  {
		double * bp = & B(0,0);
		double * end = bp + m * nrhs;
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
	  else
	  {
		for (int c = 0; c < nrhs; c++)
		{
		  for (int r = 0; r < m; r++)
		  {
			*xp++ = B(r,c);
		  }
		  xp += step;
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
