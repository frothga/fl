/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
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
#include "fl/lapackprotos.h"


namespace fl
{
  template<>
  void
  gelss (const MatrixAbstract<float> & A, Matrix<float> & x, const MatrixAbstract<float> & b, float * residual)
  {
	int m = A.rows ();
	int n = A.columns ();
	int nrhs = b.columns ();
	int ldx = std::max (m, n);
	assert (b.rows () == m);

	Matrix<float> tempA;
	const Matrix<float> * p;
	if (p = dynamic_cast<const Matrix<float> *> (&A))
	{
	  tempA = *p;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	if (ldx == m  &&  (p = dynamic_cast<const Matrix<float> *> (&b)))
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
	  for (int c = 0; c < nrhs; c++)
	  {
		for (int r = 0; r < n; r++)
		{
		  tempX(r,c) = x(r,c);
		}
	  }

	  if (residual)
	  {
		float total = 0;
		for (int c = 0; c < nrhs; c++)
		{
		  for (int r = n; r < ldx; r++)
		  {
			float e = x(r,c);
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
