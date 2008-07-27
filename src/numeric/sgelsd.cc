/*
Author: Fred Rothganger
Created 10/4/2005 to take advantage of a faster implementation of gels in
LAPACK.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
