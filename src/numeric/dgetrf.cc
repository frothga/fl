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


namespace fl
{
  template<>
  Matrix<double>
  MatrixAbstract<double>::operator ! () const
  {
	int h = rows ();
	int w = columns ();
	if (w != h) return pinv (*this);  // forces dgesvd to be linked as well

	Matrix<double> result;
	result.copyFrom (*this);

	int * ipiv = (int *) malloc (h * sizeof (int));

	int info = 0;

	dgetrf_ (h,
			 h,
			 & result[0],
			 h,
			 ipiv,
			 info);

	if (info == 0)
	{
	  int lwork = -1;
	  double optimalSize;

	  dgetri_ (h,
			   & result[0],
			   h,
			   ipiv,
			   & optimalSize,
			   lwork,
			   info);

	  lwork = (int) optimalSize;
	  double * work = (double *) malloc (lwork * sizeof (double));

	  dgetri_ (h,
			   & result[0],
			   h,
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

	return result;
  }

  template<>
  double
  det (const MatrixAbstract<double> & A)
  {
	int m = A.rows ();
	if (m != A.columns ())
	{
	  throw "det only works on square matrices";
	}

	Matrix<double> tempA;
	tempA.copyFrom (A);

	int * ipiv = (int *) malloc (m * sizeof (int));

	int info = 0;

	dgetrf_ (m,
			 m,
			 & tempA[0],
			 m,
			 ipiv,
			 info);

	int exchanges = 0;
	double result = 1;
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
