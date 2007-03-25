/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.1 thru 1.3 Copyright 2005 Sandia Corporation.
Revisions 1.5 thru 1.7 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/25 14:38:39  Fred
Don't pass destroyA flag in pinv() or rank().  Instead, assume the default is
to preserve the matrix.  This could change again in the future, but the present
code was broken since the meaning of the flag in that position changed.

Revision 1.6  2007/03/23 10:57:27  Fred
Use CVS Log to generate revision history.

Revision 1.5  2006/02/16 04:45:26  Fred
Change all "copy" options into "destroy".  These functions now have about the
same semantics as before the copy option was added, except now the programmer
can explicitly indicate that a parameter can be overwritten safely.

Revision 1.4  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/10/13 04:01:11  Fred
Allow overwriting of input matrix.  Add option to force preservation of input
matrix.

Change interface to pinv() and rank() to take a MatrixAbstract.  Use the "copy"
option when calling gesvd().

Add Sandia copyright notice.

Revision 1.2  2005/09/11 15:26:28  Fred
Changed 1.0 in pinv() to 1.0f.

Revision 1.1  2005/09/10 17:40:49  Fred
Create templates for LAPACK bridge, rather than using type specific inlines. 
Break lapackd.h and lapacks.h into implementation files.
-------------------------------------------------------------------------------
*/


#include "fl/lapack.h"
#include "fl/lapackprotos.h"

#include <float.h>


namespace fl
{
  template<>
  void
  gesvd (const MatrixAbstract<float> & A, Matrix<float> & U, Matrix<float> & S, Matrix<float> & VT, char jobu, char jobvt, bool destroyA)
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = std::min (m, n);

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
	float optimalSize;
	int info = 0;

	// Do space query first
	sgesvd_ (jobu,
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
    float * work = (float *) malloc (lwork * sizeof (float));

	// Now for the real thing
	sgesvd_ (jobu,
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
  Matrix<float>
  pinv (const MatrixAbstract<float> & A, float tolerance, float epsilon)
  {
	Matrix<float> U;
	Vector<float> D;
	Matrix<float> VT;
	gesvd (A, U, D, VT);

	if (tolerance < 0)
	{
	  if (epsilon < 0)
	  {
		epsilon = FLT_EPSILON;
	  }
	  tolerance = std::max (A.rows (), A.columns ()) * D[0] * epsilon;
	}

	for (int i = 0; i < D.rows (); i++)
	{
	  if (D[i] > tolerance)
	  {
		D[i] = 1.0f / D[i];
	  }
	  else
	  {
		D[i] = 0;
	  }
	}
	MatrixDiagonal<float> DD = D;

	return ~VT * DD * ~U;
  }

  template<>
  int
  rank (const MatrixAbstract<float> & A, float threshold, float epsilon)
  {
	Matrix<float> U;
	Matrix<float> S;
	Matrix<float> VT;
	gesvd (A, U, S, VT, 'N', 'N');

	if (threshold < 0)
	{
	  if (epsilon < 0)
	  {
		epsilon = FLT_EPSILON;
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
