/*
Author: Fred Rothganger
Created 9/10/2005 to complement the float version of the same routine.
Copyright 2005 Sandia Corporation.
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
  void
  sygv (const MatrixAbstract<double> & A, const MatrixAbstract<double> & B, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors, bool copy)
  {
	const Matrix<double> * p;
	if (! copy  &&  (p = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  eigenvectors = *p;
	}
	else
	{
	  eigenvectors.copyFrom (A);
	}

	Matrix<double> tempB;
	if (! copy  &&  (p = dynamic_cast<const Matrix<double> *> (&B)))
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
	double optimalSize = 0;
	int info = 0;

	// Space query
	dsygv_ (1,
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
	double * work = (double *) malloc (lwork * sizeof (double));

	// Actual computation, using optimal work space.
	dsygv_ (1,
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
