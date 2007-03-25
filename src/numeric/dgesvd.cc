/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.1 and 1.2  Copyright 2005 Sandia Corporation.
Revisions 1.4 thru 1.6 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/25 14:38:39  Fred
Don't pass destroyA flag in pinv() or rank().  Instead, assume the default is
to preserve the matrix.  This could change again in the future, but the present
code was broken since the meaning of the flag in that position changed.

Revision 1.5  2007/03/23 10:57:28  Fred
Use CVS Log to generate revision history.

Revision 1.4  2006/02/16 04:45:26  Fred
Change all "copy" options into "destroy".  These functions now have about the
same semantics as before the copy option was added, except now the programmer
can explicitly indicate that a parameter can be overwritten safely.

Revision 1.3  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/10/13 04:01:11  Fred
Allow overwriting of input matrix.  Add option to force preservation of input
matrix.

Change interface to pinv() and rank() to take a MatrixAbstract.  Use the "copy"
option when calling gesvd().

Add Sandia copyright notice.

Revision 1.1  2005/09/10 17:40:49  Fred
Create templates for LAPACK bridge, rather than using type specific inlines. 
Break lapackd.h and lapacks.h into implementation files.
-------------------------------------------------------------------------------
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"

#include <float.h>


namespace fl
{
  template<>
  void
  gesvd (const MatrixAbstract<double> & A, Matrix<double> & U, Matrix<double> & S, Matrix<double> & VT, char jobu, char jobvt, bool destroyA)
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = std::min (m, n);

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

	S.resize (minmn);

	switch (jobu)
	{
	  case 'A':
		U.resize (m, m);
		break;
	  case 'N':
		if (U.columns () < 1)
		{
		  U.resize (1, 1);
		}
		break;
	  default:
		jobu = 'S';
		U.resize (m, minmn);
	}

	switch (jobvt)
	{
	  case 'A':
		VT.resize (n, n);
		break;
	  case 'N':
		if (VT.columns () < 1)
		{
		  VT.resize (1, 1);
		}
		break;
	  default:
		jobvt = 'S';
		VT.resize (minmn, n);
	}

    int lwork = -1;
	double optimalSize;
	int info = 0;

	// Do space query first
	dgesvd_ (jobu,
			 jobvt,
			 m,
			 n,
			 & tempA[0],
			 m,
			 & S[0],
			 & U[0],
			 U.rows (),
			 & VT[0],
			 VT.rows (),
			 &optimalSize,
			 lwork,
			 info);

	lwork = (int) optimalSize;
    double * work = (double *) malloc (lwork * sizeof (double));

	// Now for the real thing
	dgesvd_ (jobu,
			 jobvt,
			 m,
			 n,
			 & tempA[0],
			 m,
			 & S[0],
			 & U[0],
			 U.rows (),
			 & VT[0],
			 VT.rows (),
			 work,
			 lwork,
			 info);

    free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  Matrix<double>
  pinv (const MatrixAbstract<double> & A, double tolerance, double epsilon)
  {
	Matrix<double> U;
	Vector<double> D;
	Matrix<double> VT;
	gesvd (A, U, D, VT);

	if (tolerance < 0)
	{
	  if (epsilon < 0)
	  {
		epsilon = DBL_EPSILON;
	  }
	  tolerance = std::max (A.rows (), A.columns ()) * D[0] * epsilon;
	}

	for (int i = 0; i < D.rows (); i++)
	{
	  if (D[i] > tolerance)
	  {
		D[i] = 1.0 / D[i];
	  }
	  else
	  {
		D[i] = 0;
	  }
	}
	MatrixDiagonal<double> DD = D;

	return ~VT * DD * ~U;
  }

  template<>
  int
  rank (const MatrixAbstract<double> & A, double threshold, double epsilon)
  {
	Matrix<double> U;
	Matrix<double> S;
	Matrix<double> VT;
	gesvd (A, U, S, VT, 'N', 'N');

	if (threshold < 0)
	{
	  if (epsilon < 0)
	  {
		epsilon = DBL_EPSILON;
	  }
	  threshold = std::max (A.rows (), A.columns ()) * S[0] * epsilon;
	}

	int result = 0;
	while (result < S.rows ()  &&  S[result] > threshold)
	{
	  result++;
	}

	return result;
  }
}
