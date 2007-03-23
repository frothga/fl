/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.1 thru 1.3 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 10:57:28  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/10/13 04:03:10  Fred
Change interface to det() to take a MatrixAbstract.

Add Sandia copyright notice.

Revision 1.2  2005/09/11 23:01:53  Fred
Make operator ! a member of Matrix and add the operator to MatrixAbstract and
Matrix3x3 as well.

Revision 1.1  2005/09/10 17:40:49  Fred
Create templates for LAPACK bridge, rather than using type specific inlines. 
Break lapackd.h and lapacks.h into implementation files.
-------------------------------------------------------------------------------
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
