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
  void
  sygv (const Matrix<float> & A, Matrix<float> & B, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors)
  {
	int n = A.rows ();  // or A.columns (); they should be equal
	eigenvalues.resize (n);
	eigenvectors.copyFrom (A);

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
			& B[0],
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
			& B[0],
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
