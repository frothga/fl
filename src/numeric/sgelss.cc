/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


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
  gelss (const MatrixAbstract<float> & A, Matrix<float> & x, const MatrixAbstract<float> & B, float * residual, bool destroyA, bool destroyB)
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
	float optimalSize;
	int lwork = -1;
	int info = 0;

	sgelss_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 tempA.strideC,
			 & x[0],
			 x.strideC,
			 & s[0],
			 -1.0,  // use machine precision
			 rank,
			 & optimalSize,
			 lwork,
			 info);

	if (info) throw info;
	lwork = (int) optimalSize;
	float * work = (float *) malloc (lwork * sizeof (float));

	sgelss_ (m,
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
			 info);

	free (work);
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
