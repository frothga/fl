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
  syev (const MatrixPacked<float> & A, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors)
  {
	int n = A.rows ();

	MatrixPacked<float> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	eigenvectors.resize (n, n);

	float * work = (float *) malloc (3 * n * sizeof (float));
	int info = 0;

	sspev_ ('V',
			'U',
			n,
			& tempA(0,0),
			& eigenvalues[0],
			& eigenvectors(0,0),
			n,
			work,
			info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }
}
