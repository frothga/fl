/*
Author: Fred Rothganger
Created 10/4/2005 to take advantage of a faster implementation of gels in
LAPACK.


Revisions 1.1 and 1.2  Copyright 2005 Sandia Corporation.
Revisions 1.3 thru 1.6 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/23 10:57:27  Fred
Use CVS Log to generate revision history.

Revision 1.5  2006/02/16 04:45:26  Fred
Change all "copy" options into "destroy".  These functions now have about the
same semantics as before the copy option was added, except now the programmer
can explicitly indicate that a parameter can be overwritten safely.

Revision 1.4  2006/02/05 22:52:00  Fred
Add a flag to control copying the "b" parameter, since there are cases where
the client program may want to reuse it.

Use pointer pushing to make copying more efficient.  We could cut down on the
amount of copying if we stored a leading dimension (ala LAPACK) in the Matrix
classes.

Revision 1.3  2006/01/29 03:25:29  Fred
Add copy option.

Revision 1.2  2005/10/15 22:24:24  Fred
Include assert.h.  It as only a fluke that this worked before.

Force type in log() expressin to be double.

Revision 1.1  2005/10/13 03:50:48  Fred
Create bridge for gelsd, which LAPACK documentation claims is faster than
gelss.
-------------------------------------------------------------------------------
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"
#include "fl/lapackproto.h"

#include <assert.h>


using namespace std;


namespace fl
{
  template<>
  void
  gelsd (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & B, double * residual, bool destroyA, bool destroyB)
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = min (m, n);
	int nrhs = B.columns ();
	int ldx = max (m, n);
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

	if (destroyB  &&  ldx == m  &&  (p = dynamic_cast<const Matrix<double> *> (&B)))
	{
	  x = *p;
	}
	else
	{
	  x.resize (ldx, nrhs);
	  double * xp = & x(0,0);
	  double * bp = & B(0,0);
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

	int smlsiz = ilaenv (9, "DGELSD", " ", 0, 0, 0, 0);
	int nlvl = max (0, (int) ceil (log (minmn / (smlsiz + 1.0)) / log (2.0)));

	double * s = (double *) malloc (minmn * sizeof (double));
	int liwork = 3 * minmn * nlvl + 11 * minmn;
	int * iwork = (int *) malloc (liwork * sizeof (int));

	int rank;
    int lwork = -1;
	double optimalSize;
	int info = 0;

	// Do space query first
	dgelsd_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 m,
			 & x[0],
			 ldx,
			 s,
			 -1.0,  // use machine precision
			 rank,
			 &optimalSize,
			 lwork,
			 iwork,
			 info);

	lwork = (int) optimalSize;
    double * work = (double *) malloc (lwork * sizeof (double));

	// And then the actual computation
	dgelsd_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 m,
			 & x[0],
			 ldx,
			 s,
			 -1.0,  // use machine precision
			 rank,
			 work,
			 lwork,
			 iwork,
			 info);

	free (work);
	free (s);
	free (iwork);

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
