/*
Author: Fred Rothganger
Created 10/4/2005 to take advantage of a faster implementation of gels in
LAPACK.


Revisions 1.1 and 1.2  Copyright 2005 Sandia Corporation.
Revisions 1.3 thru 1.7 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/25 14:34:42  Fred
This code was assuming that it always received dense matrices, and so was
copying it using a direct pointer to memory.  Fixed this latent bug by using
the element accessor when the matrix is not dense.

Revision 1.6  2007/03/23 10:57:30  Fred
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
#include "fl/lapackprotos.h"
#include "fl/lapackproto.h"

#include <assert.h>


using namespace std;


namespace fl
{
  template<>
  void
  gelsd (const MatrixAbstract<float> & A, Matrix<float> & x, const MatrixAbstract<float> & B, float * residual, bool destroyA, bool destroyB)
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = min (m, n);
	int nrhs = B.columns ();
	int ldx = max (m, n);
	assert (B.rows () == m);

	Matrix<float> tempA;
	const Matrix<float> * p;
	if (destroyA  &&  (p = dynamic_cast<const Matrix<float> *> (&A)))
	{
	  tempA = *p;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	p = dynamic_cast<const Matrix<float> *> (&B);
	if (destroyB  &&  ldx == m  &&  p)
	{
	  x = *p;
	}
	else
	{
	  x.resize (ldx, nrhs);
	  float * xp = & x(0,0);
	  int step = ldx - m;
	  if (p)
	  {
		float * bp = & B(0,0);
		float * end = bp + m * nrhs;
		while (bp < end)
		{
		  float * rowEnd = bp + m;
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

	int smlsiz = ilaenv (9, "SGELSD", " ", 0, 0, 0, 0);
	int nlvl = max (0, (int) ceil (log (minmn / (smlsiz + 1.0)) / log (2.0)));

	float * s = (float *) malloc (minmn * sizeof (float));
	int liwork = 3 * minmn * nlvl + 11 * minmn;
	int * iwork = (int *) malloc (liwork * sizeof (int));

	int rank;
    int lwork = -1;
	float optimalSize;
	int info = 0;

	// Do space query first
	sgelsd_ (m,
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
    float * work = (float *) malloc (lwork * sizeof (float));

	// And then the actual computation
	sgelsd_ (m,
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
	  Matrix<float> tempX (n, nrhs);
	  float * xp = & x(0,0);
	  float * tp = & tempX(0,0);
	  float * end = tp + n * nrhs;
	  int step = ldx - n;
	  while (tp < end)
	  {
		float * rowEnd = tp + n;
		while (tp < rowEnd)
		{
		  *tp++ = *xp++;
		}
		xp += step;
	  }

	  if (residual)
	  {
		float total = 0;
		float * xp = & x(0,0);
		float * end = xp + ldx * nrhs;
		while (xp < end)
		{
		  float * rowEnd = xp + ldx;
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
