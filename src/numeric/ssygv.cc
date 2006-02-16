/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapacks.h into separate file.  Update
        interface for B to match A, and allow overwriting of input.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.

02/2006 Fred Rothganger -- Add "destroy" option.
*/


#include "fl/lapack.h"
#include "fl/lapackprotos.h"


namespace fl
{
  template<>
  void
  sygv (const MatrixAbstract<float> & A, const MatrixAbstract<float> & B, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors, bool destroyA, bool destroyB)
  {
	const Matrix<float> * p;
	if (destroyA  &&  (p = dynamic_cast<const Matrix<float> *> (&A)))
	{
	  eigenvectors = *p;
	}
	else
	{
	  eigenvectors.copyFrom (A);
	}

	Matrix<float> tempB;
	if (destroyB  &&  (p = dynamic_cast<const Matrix<float> *> (&B)))
	{
	  tempB = *p;
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
			n,
			& tempB[0],
			n,
			& eigenvalues[0],
			& optimalSize,
			lwork,
			info);

	lwork = (int) optimalSize;
	float * work = (float *) malloc (lwork * sizeof (float));

	// Actual computation, using optimal work space.
	ssygv_ (1,
			'V',
			'U',
			n,
			& eigenvectors[0],
			n,
			& tempB[0],
			n,
			& eigenvalues[0],
			work,
			lwork,
			info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }
}
