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
#include "fl/lapackprotos.h"


namespace fl
{
  template<>
  void
  sygv (const MatrixAbstract<float> & A, const MatrixAbstract<float> & B, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors, bool destroyA, bool destroyB)
  {
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  eigenvectors = (const Matrix<float> &) A;
	}
	else
	{
	  eigenvectors.copyFrom (A);
	}

	Matrix<float> tempB;
	if (destroyB  &&  (B.classID () & MatrixID))
	{
	  tempB = (const Matrix<float> &) B;
	}
	else
	{
	  tempB.copyFrom (B);
	}

	int n = eigenvectors.rows ();  // or eigenvectors.columns (); they should be equal
	eigenvalues.resize (n);

	int lwork = -1;
	float optimalSize = 0;
	int info = 0;

	// Space query
	ssygv_ (1,
			'V',
			'U',
			n,
			& eigenvectors[0],
			eigenvectors.strideC,
			& tempB[0],
			tempB.strideC,
			& eigenvalues[0],
			& optimalSize,
			lwork,
			info);

	if (info) throw info;
	lwork = (int) optimalSize;
	float * work = (float *) malloc (lwork * sizeof (float));

	// Actual computation, using optimal work space.
	ssygv_ (1,
			'V',
			'U',
			n,
			& eigenvectors[0],
			eigenvectors.strideC,
			& tempB[0],
			tempB.strideC,
			& eigenvalues[0],
			work,
			lwork,
			info);

	free (work);

	if (info) throw info;
  }
}
