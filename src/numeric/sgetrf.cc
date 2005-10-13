/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapacks.h into separate file.
10/2005 Fred Rothganger -- Make det() operate on MatrixAbstract
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
  Matrix<float>
  Matrix<float>::operator ! () const
  {
	if (rows_ != columns_) return pinv (*this);  // forces sgesvd to be linked as well

	Matrix<float> tempA;
	tempA.copyFrom (*this);

	int * ipiv = (int *) malloc (rows_ * sizeof (int));

	int info = 0;

	sgetrf_ (rows_,
			 rows_,
			 & tempA[0],
			 rows_,
			 ipiv,
			 info);

	if (info == 0)
	{
	  int lwork = -1;
	  float optimalSize;

	  sgetri_ (rows_,
			   & tempA[0],
			   rows_,
			   ipiv,
			   & optimalSize,
			   lwork,
			   info);

	  lwork = (int) optimalSize;
	  float * work = (float *) malloc (lwork * sizeof (float));

	  sgetri_ (rows_,
			   & tempA[0],
			   rows_,
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
  Matrix<float>
  MatrixAbstract<float>::operator ! () const
  {
	return ! Matrix<float> (*this);
  }

  template<>
  Matrix<float>
  Matrix3x3<float>::operator ! () const
  {
	return ! Matrix<float> (const_cast<float *> (&data[0][0]), 3, 3);
  }

  template<>
  float
  det (const MatrixAbstract<float> & A)
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
