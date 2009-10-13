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

#include <assert.h>


namespace fl
{
  template<>
  void
  gelsd (const MatrixAbstract<float> & A, Matrix<float> & x, const MatrixAbstract<float> & B, float * residual, bool destroyA, bool destroyB)
  {
	const int m    = std::min (A.rows (), B.rows ());
	const int n    = A.columns ();
	const int nrhs = B.columns ();
	const int mn   = std::max (m, n);  // the minimum allowable leading dimension (stride) of B

	Matrix<float> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<float> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	const Matrix<float> * p = 0;
	if (B.classID () & MatrixID) p = (const Matrix<float> *) &B;
	if (destroyB  &&  p  &&  p->strideC >= mn)
	{
	  x = *p;
	}
	else  // must copy the elements of B into X
	{
	  x.resize (mn, nrhs);
	  float * xp = & x(0,0);
	  const int xstep = x.strideC - m;
	  if (p)
	  {
		float * bp = & B(0,0);
		float * end = bp + p->strideC * nrhs;
		const int bstep = p->strideC - m;
		while (bp < end)
		{
		  float * columnEnd = bp + m;
		  while (bp < columnEnd)
		  {
			*xp++ = *bp++;
		  }
		  xp += xstep;
		  bp += bstep;
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
		  xp += xstep;
		}
	  }
	}

	Vector<float> s (std::min (m, n));

	int rank;
    int lwork = -1;  // space query
	float optimalLwork;
	int   optimalLiwork;
	int info = 0;

	// Do space query first
	sgelsd_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 tempA.strideC,
			 & x[0],
			 x.strideC,
			 & s[0],
			 -1.0,  // use machine precision
			 rank,
			 &optimalLwork,
			 lwork,
			 &optimalLiwork,
			 info);

	if (info) throw info;
	lwork      = (int) optimalLwork;
	int liwork =       optimalLiwork;
    float * work  = (float *) malloc (lwork  * sizeof (float));
	int *   iwork = (int *)   malloc (liwork * sizeof (int));

	// And then the actual computation
	sgelsd_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 tempA.strideC,
			 & x[0],
			 x.strideC,
			 & s[0],
			 -1.0,  // use machine precision
			 rank,
			 work,
			 lwork,
			 iwork,
			 info);

	free (work);
	free (iwork);

	if (info) throw info;

	x.rows_ = n;
	if (residual)
	{
	  register float total = 0;
	  const int rows = m - n;
	  if (rows > 0)
	  {
		float * xp  = (float *) x.data;
		float * end = xp + x.strideC * nrhs;
		const int step = x.strideC - rows;
		xp += n;
		while (xp < end)
		{
		  float * columnEnd = xp + rows;
		  while (xp < columnEnd) total += *xp * *xp++;
		  xp += step;
		}
	  }
	  *residual = total;
	}
  }
}
