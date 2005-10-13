/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
09/2005 Fred Rothganger -- Moved from lapackd.h into separate file.
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"


namespace fl
{
  template<>
  void
  syev (const MatrixPacked<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	int n = A.rows ();

	MatrixPacked<double> tempA;
	tempA.copyFrom (A);

	eigenvalues.resize (n);
	eigenvectors.resize (n, n);

	double * work = (double *) malloc (3 * n * sizeof (double));
	int info = 0;

	dspev_ ('V',
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
