/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapacks.h into separate file.
*/


#include "fl/lapack.h"
#include "fl/lapackprotos.h"


namespace fl
{
  template<>
  Matrix<float>
  operator ! (const Matrix<float> & A)
  {
	int m = A.rows ();
	int n = A.columns ();
	Matrix<float> tempA;
	tempA.copyFrom (A);

	int * ipiv = (int *) malloc (std::min (m, n) * sizeof (int));

	int info = 0;

	sgetrf_ (m,
			 n,
			 & tempA[0],
			 m,
			 ipiv,
			 info);

	if (info == 0)
	{
	  int lwork = -1;
	  float optimalSize;

	  sgetri_ (n,
			   & tempA[0],
			   m,
			   ipiv,
			   & optimalSize,
			   lwork,
			   info);

	  lwork = (int) optimalSize;
	  float * work = (float *) malloc (lwork * sizeof (float));

	  sgetri_ (n,
			   & tempA[0],
			   m,
			   ipiv,
			   work,
			   lwork,
			   info);

	  free (work);
	}

	free (ipiv);

	if (info != 0)
	{
	  throw info;
	}

	return tempA;
  }

  template<>
  float
  det (const Matrix<float> & A)
  {
	int m = A.rows ();
	if (m != A.columns ())
	{
	  throw "det only works on square matrices";
	}

	Matrix<float> tempA;
	tempA.copyFrom (A);

	int * ipiv = (int *) malloc (m * sizeof (int));

	int info = 0;

	sgetrf_ (m,
			 m,
			 & tempA[0],
			 m,
			 ipiv,
			 info);

	int exchanges = 0;
	float result = 1;
	for (int i = 0; i < m; i++)
	{
	  result *= tempA(i,i);
	  if (ipiv[i] != i + 1)
	  {
		exchanges++;
	  }
	}
	if (exchanges % 2)
	{
	  result *= -1;
	}

	free (ipiv);

	if (info != 0)
	{
	  throw info;
	}

	return result;
  }
}
