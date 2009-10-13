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
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<double> &) A;
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
			 tempA.strideC,
			 & S[0],
			 & U[0],
			 U.strideC,
			 & VT[0],
			 VT.strideC,
			 &optimalSize,
			 lwork,
			 info);

	if (info) throw info;
	lwork = (int) optimalSize;
    double * work = (double *) malloc (lwork * sizeof (double));

	// Now for the real thing
	dgesvd_ (jobu,
			 jobvt,
			 m,
			 n,
			 & tempA[0],
			 tempA.strideC,
			 & S[0],
			 & U[0],
			 U.strideC,
			 & VT[0],
			 VT.strideC,
			 work,
			 lwork,
			 info);

    free (work);

	if (info) throw info;
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
