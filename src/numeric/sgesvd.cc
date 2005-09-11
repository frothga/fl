/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapacks.h into separate file.  Changed
        1.0 in pinv() to 1.0f.
*/


#include "fl/lapack.h"
#include "fl/lapackprotos.h"

#include <float.h>


namespace fl
{
  template<>
  void
  gesvd (const MatrixAbstract<float> & A, Matrix<float> & U, Matrix<float> & S, Matrix<float> & VT, char jobu, char jobvt)
  {
	int m = A.rows ();
	int n = A.columns ();
	int minmn = std::min (m, n);

	Matrix<float> tempA;
	tempA.copyFrom (A);

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
  pinv (const Matrix<float> & A, float tolerance, float epsilon)
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
  rank (const Matrix<float> & A, float threshold, float epsilon)
  {
	Matrix<float> U;
	Matrix<float> S;
	Matrix<float> VT;
	gesvd (A, U, S, VT);

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
