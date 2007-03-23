/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.1, 1.2 and 1.4 Copyright 2005 Sandia Corporation.
Revisions 1.5 thru 1.8     Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 10:57:27  Fred
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
#include "fl/lapackprotos.h"

#include <assert.h>


namespace fl
{
  template<>
  void
  gelss (const MatrixAbstract<float> & A, Matrix<float> & x, const MatrixAbstract<float> & B, float * residual, bool destroyA, bool destroyB)
  {
	int m = A.rows ();
	int n = A.columns ();
	int nrhs = B.columns ();
	int ldx = std::max (m, n);
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

	if (destroyB  &&  ldx == m  &&  (p = dynamic_cast<const Matrix<float> *> (&B)))
	{
	  x = *p;
	}
	else
	{
	  x.resize (ldx, nrhs);
	  float * xp = & x(0,0);
	  float * bp = & B(0,0);
	  float * end = bp + m * nrhs;
	  int step = ldx - m;
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

	Matrix<float> s (std::min (m, n), 1);

	int rank;
	int ldwork = 5 * std::max (ldx, nrhs);
	float * work = (float *) malloc (ldwork * sizeof (float));
	int info = 0;

	sgelss_ (m,
			 n,
			 nrhs,
			 & tempA[0],
			 m,
			 & x[0],
			 ldx,
			 & s[0],
			 -1.0f,  // use machine precision
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
