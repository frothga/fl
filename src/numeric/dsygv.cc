/*
Author: Fred Rothganger
Created 9/10/2005 to complement the float version of the same routine.
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"


namespace fl
{
  template<>
  void
  sygv (const Matrix<double> & A, Matrix<double> & B, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors)
  {
	int n = A.rows ();  // or A.columns (); they should be equal
	eigenvalues.resize (n);
	eigenvectors.copyFrom (A);

	int lwork = -1;
	double optimalSize = 0;
	int info = 0;

	// Space query
	dsygv_ (1,
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
	double * work = (double *) malloc (lwork * sizeof (double));

	// Actual computation, using optimal work space.
	dsygv_ (1,
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
