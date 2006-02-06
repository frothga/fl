/*
Author: Fred Rothganger
Created 10/4/2005 to take advantage of a faster implementation of gels in
LAPACK.
Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


01/2006 Fred Rothganger -- Add "copy" option.
02/2006 Fred Rothganger -- Use pointer pushing for more efficient copies.
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
  gelsd (const MatrixAbstract<double> & A, Matrix<double> & x, const MatrixAbstract<double> & b, double * residual, bool copyA, bool copyb)
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = min (m, n);
	int nrhs = b.columns ();
	int ldx = max (m, n);
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
